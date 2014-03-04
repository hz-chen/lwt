/*******************************************
*
* Author: Hongzhou Chen - hzchen_cs@gwmail.gwu.edu
*
*
* Last modified:	12-08-2013 22:36
*
* Filename:		lwt_types.h
*
* Description: 		contains all specific data types used by lwt library.
*
*
******************************************/

#ifndef __LWT_TYPE_H__
#define __LWT_TYPE_H__

#define _MULTI_THREADED

#include <pthread.h>
#include "atomic.h"

#define likely(x)       __builtin_expect(!!(x), 1)
#define unlikely(x)     __builtin_expect(!!(x), 0)

//lwt rdy queue size
#define _LWT_RQUEUE_MAX	2048

//thread status
typedef enum{
	_LWT_STAT_UNINIT,		//0
	_LWT_STAT_RUN,			//1
	_LWT_STAT_WAIT,			//2
	_LWT_STAT_RDY,			//3
	_LWT_STAT_DEAD,			//4
	_LWT_STAT_ZOMB,			//5
}_lwt_stat_t;

//who calls scheduler
typedef enum{
	_LWT_JOIN,	//0
	_LWT_DIE,	//1
	_LWT_YIELD,	//2
	_LWT_CHAN,	//3
}_lwt_sche_caller_t;

//control token between kthds
typedef enum{
	_LWT_KTHDT_NOTHING,			//0: by default
	_LWT_KTHDT_UNSET_TYPE,		//1: unset wait_type of target thread and put it into rdyq
	_LWT_KTHDT_SCHD,			//2: put target tcb into rdyq
	_LWT_KTHDT_DESTORY,			//3: end the whole target kthd
}_lwt_kthd_ctrl_t;

//current thread is blocking because of?
//each blocking function should have corresponding
//value here.
//after the event happens, we must reset the wait_type.
typedef enum{
	_LWT_WAIT_NOTHING,	//0: I'm not blocking for anything
	_LWT_WAIT_JOIN,		//1: bloced bacause tried to join other
	_LWT_WAIT_CHAN_RCV,	//2: blocked because tried to rcv from chan		
	_LWT_WAIT_CGRP_RCV,	//3: blocked because tried to get a rcv evnt from cgrp
	_LWT_WAIT_CHAN_SND,	//4: blocked because tried to send to chan
	_LWT_WAIT_CGRP_SND,	//5: blocked because tried to get a snd evnt from cgrp
}_lwt_wait_type_t;

//if some reg value of current tcb is not set, 
//then set the reg into _LWT_REG_NSET
//This can be set to a routing, says if 
//target reg has not been set, pop an error
//so we can handle it gracefully.
#define _LWT_REG_NSET	0


#define _LWT_MAX_EVNT_SZ	1024

//single linked list buffer for data and blocked senders.
//support FIFO
typedef struct node_t{
	void* self;
	struct node_t *next;
}linked_buf;

__thread linked_buf* nil;

typedef struct _lwt_tcb_t* lwt_t;
typedef int kthd_t;
typedef struct _lwt_tcb_t _lwt_tcb;

__thread _lwt_tcb* nil_tcb;

#define LWT_NULL	nil_tcb

struct lwt_channel{
	/*Sender data*/
	/**buf sz will be 0 in current stage**/
	void* data_buf;			//ring buffer. 
	int	buf_len;				//buf length, how many data packet in queue
	int buf_sz;					//buf size, defined at init time.
	int prod_p, cons_p;		//producer pointer & consumer pointer.

	/****/
	linked_buf* blocked_senders;	//when buf_len == buf_sz, the newly incoming 
									//senders will be put here
	linked_buf* blocked_senders_last;	//pointer points to the last one in the blocked_senders.
	int blocked_len;			//how many senders are blocked?
	linked_buf* senders;		//senders to this channel.
	int snd_cnt;				//# of senders in this channel?
//	struct lwt_channel *cgrp_prev, *cgrp_next;	//prev/next on channel group list
	struct lwt_cgrp* cgrp_snd;	//this channel can be only reached as a snder in a cgrp
	int in_snd_evnt_lst;		//this channel is in snd_evnt_lst
	struct lwt_cgrp* cgrp_rcv;	//this channel can be only reached as a rcver in a cgrp

	void* mark;
	/*receiver data*/
	_lwt_tcb* receiver;				//owner & receiver

	//pthread_id_np_t kthd_id;
//	kthd_t kthd_index;	//which kthd does the receiver of the channel belongs to?
};

typedef struct lwt_channel * lwt_chan_t;

typedef void* (*lwt_fn_t)(void*, lwt_chan_t);
typedef void* lwt_reg_t;

//this is because of a page size of 4096,
//got by #getconf PAGESIZE
#define _LWT_STACK_SIZE		16368	

#define _LWT_KTHD_EVNT_BUF	2048
#define _LWT_KTHD_MAX_NUM	2048

#define LWT_FLAG_NOJOIN	1


//total number of lwt
#define _LWT_SIZE 2048


/*
typedef struct _lwt_chan_data_t{
	//headers
	_lwt_channel_data_type_t type;	//what type this data packet contains?
	int length;	//length of the data segment of this channel data packet
	//data
	void* data;	//a pointer points to data.
}_lwt_chan_data;

*/

/**
 * This is the tcb structure. 
 * our tcb structure is an array
 * The lwt_id is the globle unique id of current thread;
 * The lwt_ip is the %ip of the current thread, actually the
 * next %ip going to be executed.
 * The lwt_ebp and the tcb_esp marks the range of stack.
 * The rdyq_next marks the next lwt_tcb in the run queue.
 * The waitq_next marks the next lwt_tcb in the wait queue.
 */
struct _lwt_tcb_t{
	int lwt_id;		//offset	
	_lwt_tcb* rdyq_prev;		//offset+4		0x4
	_lwt_tcb* rdyq_next;		//offset+8		0x8
	lwt_reg_t lwt_ebp;	//offset+12		0xc
	lwt_reg_t lwt_esp;	//offset+16		0x10
	lwt_reg_t lwt_ip;	//offset+20		0x14
	_lwt_stat_t lwt_status;		//offset+24	0x18
	//we have a dead queue as linked list
	_lwt_tcb* deadq_prev;		//offset+28		0x20
	_lwt_tcb* deadq_next;		//offset+32		0x1c
	_lwt_tcb* parent;			//offset+36		0x24
	//parent thread
	//am I been joined?
	_lwt_tcb* joined;			//offset+40	0x28

	void* ret_val;		//offset 44		0x2c
	lwt_reg_t ebp_base;	//offset 48		0x30
//	lwt_t sndq_prev;		//offset+52		0x34
//	lwt_t sndq_next;		//offset+56		0x38
//	lwt_t cgrp_wait_next;
	int flag;			//offset + 52
	_lwt_wait_type_t wait_type;	//offset + 56
	int chan_data_useful;
	void* chan_data;	//offset+60		0x3c
	pthread_t kthd_t;	//offset + 64
	kthd_t kthd_index;	//offset + 64
	int padding[13];
};		//total size: 128


/*****************channel data types*****************/
//channel direction
typedef enum{
	LWT_CHAN_NULL,
	LWT_CHAN_SND,
	LWT_CHAN_RCV,
}lwt_chan_dir_t;

struct lwt_cgrp{
	int cl_len;				//channel list length
	lwt_t snd_waiter;			//thread waiting on snd event
	lwt_t rcv_waiter;			//thread waiting on rcv event

	int snd_evnt_arr[_LWT_MAX_EVNT_SZ];
	int snd_evnt_cnt;		//length of event.
	int snd_prod_p, snd_cons_p;	//producer pointer & consumer pointer.

	int rcv_evnt_arr[_LWT_MAX_EVNT_SZ];
	int rcv_evnt_cnt;		//length of event.
	int rcv_prod_p, rcv_cons_p;	//producer pointer & consumer pointer.
};

typedef struct lwt_cgrp * lwt_cgrp_t;

struct lwt_kthd_evnt{
	_lwt_kthd_ctrl_t token;
	int target_lwt;
};


struct lwt_kthd_tcb{
	kthd_t kthd_index;
	pthread_t kthd_id;
	struct lwt_kthd_evnt (*rbuf)[_LWT_KTHD_EVNT_BUF];
	int *kthd_rbuf_cons_p;
	int *kthd_rbuf_prod_p;
	int *kthd_rbuf_len_p;
	int done_init;
	kthd_t prev;
	kthd_t next;
};

typedef struct lwt_kthd_tcb * lwt_kthd_t;


#endif


