/*******************************************
*
* Author: Hongzhou Chen - hzchen_cs@gwmail.gwu.edu
*
*
* Last modified:	03-28-2014 13:21
*
* Filename:		lwt_core.h
*
* Description:  	contains core functions that are needed
* 			by the lwt library. This is the core header 
* 			file, user should not include this one.
*
* functions:
*				__lwt_get_target
*				__lwt_start
*				__lwt_remove_from_rdyq
*				__lwt_append_into_rdyq
*				__lwt_append_into_deadq
*				__lwt_remove_from_deadq
*				__init_tcb
*				__lwt_schedule
*				__lwt_free
*				__lwt_get_rdyq_len
*				__lwt_get_thd_num
*				-------lwt channel functions------
*				__lwt_chan_triger_evnt
*				__lwt_chan_consume_evnt
*				__lwt_chann_remove_from_data_buf
*				__lwt_chann_append_into_data_buf
*				__lwt_chann_append_into_sndr_buf
*
******************************************/

#ifndef __LWT_CORE_H__
#define __LWT_CORE_H__

#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

//#include <lwt/lwt_types.h>
#include "../lwt_types/lwt_types.h"

/*****public data structures*****/

struct lwt_kthd_tcb kthds[_LWT_KTHD_MAX_NUM];
int kthd_num;

/*****private data of thread*****/

//a ring buffer contains notification token from others
__thread struct lwt_kthd_evnt (*kthd_ring_buf)[_LWT_KTHD_EVNT_BUF];
__thread struct lwt_kthd_evnt nil_evnt;
__thread int kthd_rbuf_cons, kthd_rbuf_prod, kthd_rbuf_len;
__thread pthread_t kthd_self;
__thread kthd_t kthd_index;
//this is the pointer to the root node.
//we want to make the main thread tcb root.
//the lwt pool is a linked list, with max size _LWT_SIZE;
__thread _lwt_tcb lwt_lst_root[_LWT_SIZE];
//This is the pointer to the tail node.
__thread int lwt_lst_tail;
//we have a ready queue:
//the ready queue is a linklist, with max size _LWT_SIZE;
//we will not build a seperate run queue, this will simply be
//a pointer to the first lwt_tcb in the run queue.
//We currently useing the FIFO schedule strategy.
__thread lwt_t lwt_rdyq_head;
__thread lwt_t lwt_rdyq_tail;
//we have a dead queue, for reuse:
__thread lwt_t lwt_dead_head;
__thread lwt_t lwt_dead_tail;
//memory barriers;
//__thread int barrier;


//the num_of_threads is used to present the number of threads 
//currently in RDY, RUN, and WAIT status. The locked is to protect
//the num_of_threads from multi-thread issue in the next version.
//Only scheduler can access this number, no other thread shouhld be 
//able to access this number, otherwise the number might be wrong.
//Since in the very first version, it is co-operative multi-thread, we
//as long as one does not calls yield, then others will not interrupt
//the process of number modification.
__thread int num_of_threads = 0;
__thread int length_of_rdyq = 0;
__thread int length_of_deadq = 0;

__thread _lwt_tcb* curr_tcb = NULL;

//void __lwt_remove_from_rdyq_S(lwt_t target);
void __before_main(void) __attribute__((constructor));
void __lwt_trampoline(void);
void __lwt_dispatch(_lwt_tcb* next_tcb, _lwt_tcb* curr_tcb);



typedef struct lwt_start_param_struct_t{
	lwt_chan_t c;
	void* data;
	lwt_fn_t fn;
	kthd_t kthd_index;
}lwt_args_t;




//this function get the next avaliable tcb number
	static inline
int __lwt_get_target()
{
	if(unlikely(lwt_dead_head == nil_tcb))
	{
#ifdef DBG
		printf("selected %d because dead_head is nil_tcb\n", num_of_threads);
#endif
		return num_of_threads;
	}
	else
	{
#ifdef DBG
		printf("selected %d because dead_head is %d\n", lwt_dead_head->lwt_id, lwt_dead_head->lwt_id);
#endif
		return lwt_dead_head->lwt_id;
	}
}


void* __lwt_start(lwt_fn_t fn, void* data, lwt_chan_t c)
{
#ifdef DBG
	printf("thread %d in kthd %d: __lwt_start\n", curr_tcb->lwt_id, kthd_index);
#endif

	if(c)
	{
		c->receiver = curr_tcb;
	}
	return fn(data, c);
}

/***************************copied from lwt_core.S***************/

asm ("__lwt_trampoline:\n"
		"call __lwt_start\n"
				"pushl %eax\n"
				"call lwt_die\n"
				// will cause a fault...you should never get here!
				"movl $0, %eax\n"
				"movl (%eax), %ebx\n"
	);
	
/**
 *	Call this function like __lwt_dispatch(lwt_tcb* next_tcb, lwt_tcb* curr_tcb)
 */
asm ("__lwt_dispatch:\n"
				"pushl %ebp\n"
				"movl %esp, %ebp\n"
				"subl $0x18, %esp\n"
				"movl %eax, 0x14(%esp)\n"
				"movl %ebx, 0x10(%esp)\n"
				"movl %ecx, 0x0c(%esp)\n"
				"movl %edx, 0x08(%esp)\n"
				"movl %esi, 0x04(%esp)\n"
				"movl %edi, 0x00(%esp)\n"

				"movl 0x24(%esp), %eax\n"		//got curr_tcb, 8 more bytes earlier;
				"movl %esp, 0x10(%eax)\n"		//movl %esp, curr_tcb->lwt_esp
				"movl %ebp, 0xc(%eax)\n"		//movl %ebp, curr_tcb->lwt_ebp
				"movl $1f, 0x14(%eax)\n"			//movl %eip, curr_tcb->lwt_eip
				"movl 0x20(%esp), %eax\n"	//got next_tcb
				"movl 0x10(%eax), %esp\n"	//movl $next_tcb->lwt_esp, %esp
				"movl 0xc(%eax), %ebp\n" 	//movl curr_tcb->lwt_ebp, %ebp
				"movl 0x14(%eax), %ebx\n"	//movl $next_tcb->lwt_eip, %ebx
				"jmp *%ebx\n"
				"1:"

				"movl 0x00(%esp), %edi\n"
				"movl 0x04(%esp), %esi\n"
				"movl 0x08(%esp), %edx\n"
				"movl 0x0c(%esp), %ecx\n"
				"movl 0x10(%esp), %ebx\n"
				"movl 0x14(%esp), %eax\n"

				"movl %ebp, %esp\n"

				"leave\n"
				"ret\n"
				);

/***************************copied from lwt_core.S***************/

/**
 *	Remove a node from rdyq
 */
	static inline 
void __lwt_remove_from_rdyq(_lwt_tcb* target)
{
#ifdef DBG
	printf("thread %d in kthd %d: __lwt_rmv_rdyq:\t%d\n", curr_tcb->lwt_id, kthd_index, target->lwt_id);
#endif
	assert(length_of_rdyq>0);
//	while(length_of_rdyq == 0)
//		sleep(1);
	assert((target->lwt_status == _LWT_STAT_RDY)||(target->lwt_status == _LWT_STAT_ZOMB));
	if(target == lwt_rdyq_head)
	{
		lwt_rdyq_head = target -> rdyq_next;
		lwt_rdyq_head->rdyq_prev = nil_tcb;
	}else if (target == lwt_rdyq_tail)
	{
		lwt_rdyq_tail = target -> rdyq_prev;
		lwt_rdyq_tail->rdyq_next = nil_tcb;
	}
	else
	{
		target->rdyq_prev->rdyq_next = target->rdyq_next;
		target->rdyq_next->rdyq_prev= target->rdyq_prev;
	}
	target->rdyq_next = nil_tcb;
	target->rdyq_prev = nil_tcb;	

	length_of_rdyq--;


#ifdef DBG
	printf("thread %d in kthd %d: after rmv, length_of_rdyq:\t%d\n",curr_tcb->lwt_id, kthd_index, length_of_rdyq);
#endif
}



/**
 *	check if current thread status is _LWT_STAT_WAIT, then append it into rdyq.
 */
	static inline 
void __lwt_append_into_rdyq(lwt_t target)
{
#ifdef DBG
	printf("thread %d in kthd %d: __lwt_add_rdyq: %d\n",curr_tcb->lwt_id, kthd_index, target->lwt_id);
#endif

#ifdef DBG
	assert(target->lwt_status == _LWT_STAT_RDY ||
			target->lwt_status == _LWT_STAT_ZOMB);
#endif

	if(lwt_rdyq_head== nil_tcb)
	{
		target->rdyq_next = nil_tcb;
		target->rdyq_prev = nil_tcb;
		lwt_rdyq_head = target;
		lwt_rdyq_tail = target;
	}else
	{
		target->rdyq_next = nil_tcb;
		target->rdyq_prev = lwt_rdyq_tail;
		lwt_rdyq_tail->rdyq_next = target;
		lwt_rdyq_tail = target;
	}
	length_of_rdyq++;
#ifdef DBG
	printf("thread %d in kthd %d: after add, length_of_rdyq:\t%d\n",
			curr_tcb->lwt_id, kthd_index, length_of_rdyq);
#endif
}


/**
 *	check node status _LWT_STAT_DEAD, then append it into deadq
 */
	static inline 
void __lwt_append_into_deadq(lwt_t target)
{
#ifdef DBG
	printf("thread %d in kthd %d: __lwt_add_dq:\t%d\n",curr_tcb->lwt_id, kthd_index, target->lwt_id);
#endif
	assert(target->lwt_status==_LWT_STAT_DEAD);
	if(lwt_dead_head == nil_tcb)
	{
		target->deadq_next = nil_tcb;
		target->deadq_prev = nil_tcb;
		lwt_dead_head = target;
		lwt_dead_tail = target;
	}else
	{
		target->deadq_next = nil_tcb;
		target->deadq_prev = lwt_dead_tail;
		lwt_dead_tail->deadq_next = target;
		lwt_dead_tail = target;
	}
	length_of_deadq++;

	//char* stack = target->lwt_ebp + _LWT_STACK_SIZE;
	//free(stack);

}

/**
 *	Remove a node from deadq.
 */
	static inline 
void __lwt_remove_from_deadq()
{
#ifdef DBG
	printf("thread %d in kthd %d: __lwt_rmv_dq:\t%d\n",curr_tcb->lwt_id, kthd_index, lwt_dead_head->lwt_id);
#endif
	assert(length_of_deadq > 0);
	//this is the default behavior
	_lwt_tcb* target = lwt_dead_head;
	//1: move head pointer;
	lwt_dead_head = target->deadq_next;

	//2: manipulate pointers.
	lwt_dead_head->deadq_prev = nil_tcb;
	target->deadq_next = nil_tcb;
	target->deadq_prev = nil_tcb;

	length_of_deadq--;
	if(length_of_deadq ==0)
	{
		lwt_dead_head = nil_tcb;
		lwt_dead_tail = nil_tcb;
	}
}

	static inline void 
__lwt_kthd_rbuf_write(kthd_t index, _lwt_kthd_ctrl_t token, int target_lwt)
{

#ifdef DBG
	printf("thread %d in kthd %d: writing event %d to target lwt %d in kthd %d\n",
			curr_tcb->lwt_id, kthd_index, token, target_lwt, index);
#endif
	//make sure target thread is completely up.
	while(unlikely(kthds[index].done_init == 0));
	assert(index != kthd_index);
	int tmp_prod;
	int new_prod;
	do{
		tmp_prod = *(kthds[index].kthd_rbuf_prod_p);
		new_prod = tmp_prod + 1;
	}while(!(__cas((kthds[index].kthd_rbuf_prod_p), 
					tmp_prod, new_prod)));
	(*(kthds[index].rbuf))[tmp_prod].token = token;
	(*(kthds[index].rbuf))[tmp_prod].target_lwt = target_lwt;

	assert((*(kthds[index].rbuf))[tmp_prod].token == token);
	__faa(kthds[index].kthd_rbuf_len_p, 1);

#ifdef DBG
	printf("thread %d in kthd %d: after write, kthd_rbuf_len in kthds %d is %d\n",
			curr_tcb->lwt_id, kthd_index, index, *(kthds[index].kthd_rbuf_len_p));
#endif

}

	static inline struct lwt_kthd_evnt 
__lwt_kthd_rbuf_read()
{
	volatile unsigned int len;
	len = kthd_rbuf_len;
	if (len > 0)
	{
#ifdef DBG
		printf("thread %d in kthd %d: before read, kthd_rbuf_len in kthds %d is %d\n",
				curr_tcb->lwt_id, kthd_index, kthd_index, kthd_rbuf_len);
#endif
		//at any time under any circumstance, the kthd_rbuf_cons will
		//only been touched by the thread it belongs to.
		int tmp_cons = kthd_rbuf_cons;
		kthd_rbuf_cons ++;
#ifdef DBG
		printf("thread %d in kthd %d: after read, kthd_rbuf_len in kthds %d is %d\n",
				curr_tcb->lwt_id, kthd_index, kthd_index, kthd_rbuf_len);
#endif

		struct lwt_kthd_evnt got = (*kthd_ring_buf)[tmp_cons];
		assert(got.token != _LWT_KTHDT_NOTHING);
		return got;
	}
	return nil_evnt;
}

static inline void
__init_pool()
{
	lwt_lst_root[0] = *(_lwt_tcb*)calloc(sizeof(_lwt_tcb) * _LWT_SIZE, sizeof(_lwt_tcb));
	nil_tcb = (_lwt_tcb*)malloc(sizeof(_lwt_tcb));
	nil_channel = (lwt_chan_t)malloc(sizeof(lwt_chan_t));
	nil_channel->receiver = nil_tcb;
	lwt_rdyq_head = nil_tcb;
	lwt_rdyq_tail = nil_tcb;
	lwt_dead_head = nil_tcb;
	lwt_dead_head = nil_tcb;
	curr_tcb = &(lwt_lst_root[0]);
}


/**
 * This function will init an ampty _lwt_tcb on heap and return a
 * pointer to it.
 *
 * the new _lwt_tcb has its id as input parameter.
 *
 */
_lwt_tcb* __init_tcb(int id)
{

	if(unlikely(kthd_self == 0))
		kthd_self = pthread_self();

#ifdef DBG
	printf("thread %d in kthd %d: I'm in __init_tcb: \t%d\n", curr_tcb->lwt_id, kthd_index, id);
#endif
	_lwt_tcb* lwt_tmp = (_lwt_tcb*)&(lwt_lst_root[id]);

	/**
	 * get new thread info and store them into _lwt_tmp:
	 */

	//the main thread has thread id 0.
	lwt_tmp->lwt_id = id;
	lwt_tmp->lwt_ip = _LWT_REG_NSET;

	//set stack pointers
	//if this is main, then we don't need to manually assign a stack
	if(lwt_tmp->lwt_status == _LWT_STAT_DEAD)
	{
		//this is a reused block, just clear stack.
		//		memset(lwt_tmp->lwt_ebp, 0, _LWT_STACK_SIZE-50);
		lwt_tmp->lwt_ebp = lwt_tmp->ebp_base;
		lwt_tmp->lwt_esp = lwt_tmp->lwt_ebp;
		__lwt_remove_from_deadq(id);
		lwt_tmp->ret_val = 0;
	}else if(id != 0)	//this is a new one
	{
		//now we have the stack allocated and cleared. 
		char* stack = (void*)calloc(sizeof(char)*_LWT_STACK_SIZE, sizeof(char));
		lwt_tmp->lwt_ebp = stack +_LWT_STACK_SIZE;
		lwt_tmp->ebp_base = lwt_tmp->lwt_ebp;
		lwt_tmp->lwt_esp = lwt_tmp->lwt_ebp;
	}

	if(id == 0)
		lwt_tmp->parent = nil_tcb;
	else
		lwt_tmp->parent = curr_tcb;

	//this is to record whether this block as been joined by someone else or not
	lwt_tmp->joined = nil_tcb;

	//manipulating rdyq & deadq
	lwt_tmp->rdyq_next = nil_tcb;
	lwt_tmp->rdyq_prev = nil_tcb;
	lwt_tmp->deadq_next= nil_tcb;
	lwt_tmp->deadq_prev= nil_tcb;
	if(id==0)
	{
		lwt_tmp->lwt_status = _LWT_STAT_RUN;
	}
	else
	{
		lwt_tmp->lwt_status = _LWT_STAT_RDY;
		__lwt_append_into_rdyq(lwt_tmp);
	}

	lwt_tmp->ret_val = NULL;

	//waiting list
	//	lwt_tmp->wait_cnt = 0;		//this one is waiting for no one.

	lwt_tmp->chan_data = NULL;
	lwt_tmp->chan_data_useful = 0;
	lwt_tmp->wait_type = _LWT_WAIT_NOTHING;
	lwt_tmp->kthd_t = kthd_self;
	lwt_tmp->kthd_index = kthd_index;

	num_of_threads++;


	return lwt_tmp;
}


inline void
__before_main(){
#ifdef DBG
	printf("in kthd %d: this is the init function before main.\n",
			kthd_index);
#endif
	__init_pool();
	__init_tcb(0);
}




/**
 *	__lwt_schedule will choose the next target,
 *	save current content, 
 *	manipulate queues, then calls __lwt_dispatch
 *
 */
void  __lwt_schedule(_lwt_tcb* target)
{

#ifdef DBG
	assert(target->lwt_status == _LWT_STAT_ZOMB ||
			target->lwt_status == _LWT_STAT_RUN);
#endif

#ifdef DBG

	if(!(target->lwt_status == _LWT_STAT_ZOMB ||
				target->lwt_status == _LWT_STAT_RUN))
	{
		printf("schedule target %d with unexpected status: %d\n",
				target->lwt_id,
				target->lwt_status);
		exit(0);
	}
#endif


#ifdef DBG
	printf("thread %d in kthd %d: I'm calling __lwt_schedule:%d\n",curr_tcb->lwt_id, kthd_index, target->lwt_id);
#endif


	_lwt_tcb* curr = curr_tcb;
	curr_tcb = target;
	__lwt_dispatch(curr_tcb, curr);


#ifdef DBG
	assert(curr_tcb->lwt_status == _LWT_STAT_ZOMB ||
			curr_tcb->lwt_status == _LWT_STAT_RUN);
#endif

#ifdef DBG

	if(!(curr_tcb->lwt_status == _LWT_STAT_ZOMB ||
				curr_tcb->lwt_status == _LWT_STAT_RUN))
	{
		printf("back from schedule, thread %d with unexpected status: %d\n",
				curr_tcb->lwt_id,
				curr_tcb->lwt_status);
		exit(0);
	}
#endif

	return;

}

void __lwt_free(lwt_t target)
{
	num_of_threads--;
}

int __lwt_get_rdyq_len()
{
	return length_of_rdyq;
}

int __lwt_get_thd_num()
{
	return num_of_threads;
}

/*****************************************************************************
 ***************	Following are functions used by channel. *****************
 *****************************************************************************/

/**
 *	This function will trigger an event.
 *
 */
void __lwt_chan_triger_evnt(lwt_chan_t c, lwt_chan_dir_t type)
{

	assert(type == LWT_CHAN_SND || type == LWT_CHAN_RCV);

	int is_snd = (type == LWT_CHAN_SND);
	lwt_cgrp_t cg = is_snd? c->cgrp_snd : c->cgrp_rcv;

#ifdef DBG
	printf("thread %d in kthd %d: event trigger: %d in channel 0x%08x.\n",
			curr_tcb->lwt_id, kthd_index, type, (unsigned int)c);
#endif

	//if already in, then don't add
	if(is_snd && c->in_snd_evnt_lst)
	{
#ifdef DBG
		printf("thread %d in kthd %d: channel 0x%08x is already in send list, return.\n",
				curr_tcb->lwt_id, kthd_index, (unsigned int)c);
#endif
		return;
	}

	if(unlikely(cg == NULL))
	{

#ifdef DBG
		printf("thread %d in kthd %d: channel 0x%08x is not in any cgrp, return.\n",
				curr_tcb->lwt_id, kthd_index, (unsigned int)c);
#endif

		return;
	}


	int (*evnt_arr)[_LWT_MAX_EVNT_SZ] = NULL;
	int prod, prod_new;

	if(is_snd)	//is send event:
	{
		evnt_arr = &(cg->snd_evnt_arr);
		do{
			prod = ((cg->snd_prod_p));
			prod_new = (prod+1)%_LWT_MAX_EVNT_SZ;
		}while(!__cas(&(cg->snd_prod_p), prod, prod_new));
		(*evnt_arr)[prod] = (unsigned int) c;
		__faa(&(cg->snd_evnt_cnt), 1);

		c->in_snd_evnt_lst = 1;
	}else{
		evnt_arr = &(cg->rcv_evnt_arr);
		do{
			prod = ((cg->rcv_prod_p));
			prod_new = (prod+1)%_LWT_MAX_EVNT_SZ;
		}while(!__cas(&(cg->rcv_prod_p), prod, prod_new));
		(*evnt_arr)[prod] = (unsigned int) c;
		__faa(&(cg->rcv_evnt_cnt), 1);
	}

	return;

}



lwt_chan_t __lwt_chan_consume_evnt(lwt_cgrp_t cg, lwt_chan_dir_t type)
{
	if(unlikely(cg == NULL))
		return NULL;

	assert(type == LWT_CHAN_SND || type == LWT_CHAN_RCV);
	int is_snd = (type == LWT_CHAN_SND);


	int (*evnt_arr)[_LWT_MAX_EVNT_SZ];
	int cons, cons_new;

#ifdef DBG
	printf("thread %d in kthd %d: remove event from cg 0x%08x\n",
			curr_tcb->lwt_id, kthd_index, (unsigned int)cg);
#endif

	//TODO: concurrency problem need to solve.
	lwt_chan_t ret = NULL;
	if(!is_snd)	//rcv evnt
	{


		evnt_arr = &(cg->rcv_evnt_arr);
		do{
			cons = ((cg->rcv_cons_p));
			cons_new = (cons+1)%_LWT_MAX_EVNT_SZ;
		}while(!__cas(&(cg->rcv_cons_p), cons, cons_new));

		__faa(&(cg->rcv_evnt_cnt), -1);
		ret = (lwt_chan_t)(*evnt_arr)[cons];
		(*evnt_arr)[cons] = 0;


	}else		//snd evnt
	{
		//check if target channel buffer full

		evnt_arr = &(cg->snd_evnt_arr);
		do{
			cons = ((cg->snd_cons_p));
			cons_new = (cons+1)%_LWT_MAX_EVNT_SZ;
		}while(!__cas(&(cg->snd_cons_p), cons, cons_new));

		__faa(&(cg->snd_evnt_cnt), -1);
		ret = (lwt_chan_t)(*evnt_arr)[cons];
		(*evnt_arr)[cons] = 0;
		ret->in_snd_evnt_lst = 0;


		int can_rm = !(ret->buf_sz - ret->buf_len);
		int prod, prod_new;
		if(!can_rm)
		{	//if can't remove, add a new event at the end

			do{
				prod = ((cg->snd_prod_p));
				prod_new = (prod+1)%_LWT_MAX_EVNT_SZ;
			}while(!__cas(&(cg->snd_prod_p), prod, prod_new));
			(*evnt_arr)[prod] = (unsigned int) ret;
			ret->in_snd_evnt_lst = 1;
			__faa(&(cg->snd_evnt_cnt), 1);
		}
	}
	assert(ret);

	return ret;
}


/**
 * This function takes a lwt_chan_t as parameter, return the target data packet.
 * 1: assert: curr_tcb == c->receiver;
 * 2: see if there is any data on data buffer:
 * 		while(buf_len == 0 && blocked_len == 0)
 * 			2.1: mark myself as WAIT.
 * 			2.2: yield(nil_tcb);		//so current receiver thread will be blocked
 * 		//reach out here, means either buf_len>0 or blocked_len > 0
 * 		if buf_len > 0
 *			2.1: get data_buf[cons_p];
 *			2.2: cons_p ++;
 * 			2.3: buf_len --;
 * 			if blocked_len > 0
 * 				2.4: remove a sender from blocked senders list
 * 				2.5: blocked_len--;
 * 				2.6: put target->chan_data into data_buf[prod_p];
 * 				2.7: buf_len ++;
 * 				2.8: mark target as RDY;
 * 				2.8.5: unset target's wait type
 * 				2.9: put target into rdyq;
 * 		else	//no buf_len, but has blocked.
 * 			2.1: get the first thread from blocked_senders
 * 			2.2: get the data packet from tcb
 * 			2.3: unblock sender:
 * 				2.3.1: turn sender status into RDY
 * 				2.3.2: put sender into rdyq
 * 			2.5: continue executes, return the copied data packet.
 *
 */
	inline void*
__lwt_chan_remove_from_data_buf(lwt_chan_t c)
{
#ifdef DBG
	printf("thread %d in kthd %d: attempting to remove data from channel 0x%08x\n", 
			curr_tcb->lwt_id, kthd_index, (unsigned int) c);
#endif

	//_lwt_tcb* op_tcb = curr_tcb;
	_lwt_tcb* op_tcb = curr_tcb;
	void* data = NULL;
	lwt_t tmp = nil_tcb;
	//1
	assert(curr_tcb == (c->receiver));
	//2
	while(c->buf_len == 0 && c->blocked_len == 0)
	{
#ifdef DBG
		printf("thread %d in kthd %d: no data/blocked found in channel 0x%08x, blocking...\n",
				curr_tcb->lwt_id, kthd_index, (unsigned int)c);
#endif

//		__lwt_chan_triger_evnt(c, LWT_CHAN_SND);

//		_lwt_tcb* op_tcb;
		//2.1:
		//curr_tcb->lwt_status = _LWT_STAT_WAIT;
		curr_tcb->lwt_status = _LWT_STAT_RDY;
		__lwt_append_into_rdyq(curr_tcb);
		//2.2:

		__lwt_chan_triger_evnt(c, LWT_CHAN_SND);
		op_tcb = lwt_rdyq_head;
		__lwt_remove_from_rdyq(op_tcb);
		op_tcb->lwt_status = _LWT_STAT_RUN;
		__lwt_schedule(op_tcb);

#ifdef DBG
		printf("thread %d in kthd %d: awake and check if any data avaliable in channel 0x%08x\n",
				curr_tcb->lwt_id, kthd_index, (unsigned int)c);
#endif

	}
	//reach out here, means either buf_len>0 or blocked_len > 0

	if(c->buf_len>0)
	{

#ifdef DBG
		printf("thread %d in kthd %d: data found in channel 0x%08x\n",
				curr_tcb->lwt_id, kthd_index, (unsigned int) c);
#endif
		int cons, cons_new;
		do{
			cons = ((c->cons_p));
			cons_new = (cons+1) % (c->buf_sz);
		}while(!__cas(&(c->cons_p), cons, cons_new));
		__faa(&(c->buf_len), -1);
		data = (*(void* (*)[c->buf_sz])(c->data_buf))[cons];
		
		int blocked_len = ((c->blocked_len));

		if(blocked_len > 0)
		{
			//2.4
			tmp = c->blocked_senders;
			c->blocked_senders = c->blocked_senders->blkq_next;

			//2.7
			int prod, prod_new;
			do{
				prod = c->prod_p;
				prod_new = (prod + 1) % (c->buf_sz);
			}while(!__cas(&(c->prod_p), prod, prod_new));
			__faa(&(c->buf_len), 1);
			(*(void* (*)[c->buf_sz])(c->data_buf))[prod] = (tmp)->chan_data;


			//0.5:

			tmp->chan_data_useful = 0;
			tmp->wait_type = _LWT_WAIT_NOTHING;

			//2.5
			c->blocked_len--;
			//2.6
			if(c->blocked_len == 0)
				c->blocked_senders_last = nil_tcb;	

#ifdef DBG
			printf("thread %d in kthd %d: found blocked sender %d, data put in buffer, append into rdyq.\n",
					curr_tcb->lwt_id, kthd_index, tmp->lwt_id);
#endif

	//		assert(((_lwt_tcb*)(tmp->self))->wait_type == _LWT_WAIT_CHAN_SND ||
	//				((_lwt_tcb*)(tmp->self))->wait_type == _LWT_WAIT_CGRP_SND );
			//2.8
			tmp->lwt_status = _LWT_STAT_RDY;
			//2.9
			if(tmp->kthd_index == kthd_index)
				__lwt_append_into_rdyq(tmp);
			else
				__lwt_kthd_rbuf_write(tmp->kthd_index, 
						_LWT_KTHDT_SCHD, (tmp->lwt_id));
		}else
		{
			__lwt_chan_triger_evnt(c, LWT_CHAN_SND);
		}
	}else{
		//2.1
		tmp = c->blocked_senders;
		while(unlikely(tmp->wait_type != _LWT_WAIT_CHAN_SND &&
					tmp->wait_type != _LWT_WAIT_CGRP_SND && 
					tmp->wait_type != _LWT_WAIT_NOTHING))
		{

#ifdef DBG
			printf("thread %d in kthd %d: no data found, but find blocked sender %d, expecting waiting type %d or %d, however the sender is waiting for event %d, so yield to thread %d.\n",
					curr_tcb->lwt_id, kthd_index, tmp->lwt_id, _LWT_WAIT_CHAN_SND, _LWT_WAIT_CGRP_SND, tmp->wait_type, lwt_rdyq_head->lwt_id);
#endif
			op_tcb = lwt_rdyq_head;
			curr_tcb->lwt_status = _LWT_STAT_RDY;
			__lwt_append_into_rdyq(curr_tcb);
			__lwt_remove_from_rdyq(op_tcb);
			if(op_tcb->lwt_status != _LWT_STAT_ZOMB)
				op_tcb->lwt_status = _LWT_STAT_RUN;
			__lwt_schedule(op_tcb);

		}

#ifdef DBG
		printf("thread %d in kthd %d: no data found, but find blocked sender %d in kthd %d, append into rdyq.\n",
				curr_tcb->lwt_id, kthd_index, tmp->lwt_id, tmp->kthd_index);
#endif
		c->blocked_senders = c->blocked_senders->blkq_next;
		//2.2
		data = (tmp->chan_data);
		tmp->chan_data_useful = 0;
		if(c->blocked_len == 0)
			c->blocked_senders_last = nil_tcb;
		c->blocked_len--;

		if(tmp->kthd_index == kthd_index)
		{
			//2.3.1
			tmp->lwt_status = _LWT_STAT_RDY;
			tmp->wait_type = _LWT_WAIT_NOTHING;
			//2.3.2
			__lwt_append_into_rdyq(tmp);
		}
		else
			__lwt_kthd_rbuf_write(tmp->kthd_index, 
					_LWT_KTHDT_SCHD, (tmp->lwt_id));


		//2.5
	}

	//	if(tmp != NULL)
	//		free(tmp);


#ifdef DBG
	printf("thread %d in kthd %d: unmark myself from waiting type %d into 0\n",
			curr_tcb->lwt_id, kthd_index, curr_tcb->wait_type);
#endif
	curr_tcb->wait_type = _LWT_WAIT_NOTHING;

	return data;

}
	static inline int
__lwt_chan_append_into_blkd_buf(lwt_chan_t c, _lwt_tcb* sender)
{
	//target channel has invalid receiver
	if(unlikely((c->receiver) == nil_tcb))
		return -1;
#ifdef DBG
	printf("thread %d in kthd %d: thread %d in kthd %d append onto blocked list of channel 0x%08x\n",
			curr_tcb->lwt_id, kthd_index, sender->lwt_id, sender->kthd_index, (unsigned int) c);
#endif
	if(c->blocked_len == 0)
	{
		c->blocked_senders = sender;
		c->blocked_senders_last = sender;
		c->blocked_senders_last->blkq_next = nil_tcb;
	}else{
		c->blocked_senders_last->blkq_next = sender;
		c->blocked_senders_last = sender;
	}
	c->blocked_len++;

	return 0;
}



/**
 *	0.1: if target thread is dead or zombie, return -1;
 *	0.2: if target thread is already derefed, return -1;
 *	0.5:
 *	trigger event.
 *	1: check channel status
 *		if channel data buffer not full:
 *		2.1: append data onto data_buf
 *		2.2: returns
 *		else:	//channel data buffer is full:
 *		2.1: append self into blocked senders list
 *		-2.2: turn myself into WAIT status
 *		2.3: yield(receiver);	//this will unblock the receiver if
 *							//the receiver is waiting the channel
 *		2.4: after back, means the message has been
 *			successfully received, send succeed, means the sender's block
 *			will be nolonger needed, free it.
 *
 *	4: return.
 *
 */
	inline int 
__lwt_chan_append_into_data_buf(lwt_chan_t c, void* data_pkt)
{


	int full = ( ((c->buf_len)) == c->buf_sz);

	//0.1
	_lwt_stat_t status = ((c->receiver->lwt_status));

	//0.2
	if(unlikely(c->receiver == nil_tcb))
	{
#ifdef DBG
		printf("thread %d in kthd %d: send to channel 0x%08x failed: receiver is nil_tcb!\n",
				curr_tcb->lwt_id, kthd_index, (unsigned int)c);
#endif
		return -1;
	}



	if(unlikely((status == _LWT_STAT_DEAD ||
					status == _LWT_STAT_ZOMB)))
	{
#ifdef DBG
		printf("thread %d in kthd %d: send to channel 0x%08x failed: receiver DEAD!\n",
				curr_tcb->lwt_id, kthd_index, (unsigned int)c);
#endif
		return -1;
	}

	//0.5:
	//__lwt_chan_triger_evnt(c, LWT_CHAN_RCV);

	//1:
	if(!full)
	{
#ifdef DBG
		printf("thread %d in kthd %d: channel 0x%08x is not full\n",
				curr_tcb->lwt_id, kthd_index, (unsigned int)c);
#endif
		//2.1
		(*(void* (*)[c->buf_sz])(c->data_buf))[c->prod_p] = data_pkt;

		//we don't need to check if new prod_p is overwriting cons_p or not,
		//because we have buf_len.
		int prod, prod_new;
		do{
			prod = c->prod_p;
			prod_new = (prod + 1) % (c->buf_sz);
		}while(!__cas(&(c->prod_p), prod, prod_new));
		__faa(&(c->buf_len), 1);

		curr_tcb->chan_data_useful = 0;
		//0.5:
		__lwt_chan_triger_evnt(c, LWT_CHAN_RCV);


		//before first send, the channel is empty, so receiver is waiting.
		//wake up
	
		int status = c->receiver->lwt_status;
		int type = c->receiver->wait_type;

		if(unlikely((status == _LWT_STAT_WAIT && type == _LWT_WAIT_CHAN_RCV) ||
					(status == _LWT_STAT_WAIT && type == _LWT_WAIT_CGRP_RCV)))
		{
			if(c->receiver->kthd_index != kthd_index)
			{
				__lwt_kthd_rbuf_write(c->receiver->kthd_index, _LWT_KTHDT_UNSET_TYPE, c->receiver->lwt_id);
			}else{
				c->receiver->lwt_status = _LWT_STAT_RDY;
				c->receiver->wait_type = _LWT_WAIT_NOTHING;
				__lwt_append_into_rdyq(c->receiver);
			}
		}
	}
	else
	{

		//lwt_t target = nil_tcb;
		_lwt_tcb* target = nil_tcb;
		//if c->receiver is not waiting for receive event, call others 

		int type = c->receiver->wait_type;

		while(unlikely((type != _LWT_WAIT_CHAN_RCV &&
						type != _LWT_WAIT_CGRP_RCV && 
						type != _LWT_WAIT_NOTHING )))
		{
#ifdef DBG
			printf("thread %d in kthd %d: tried to wake thread %d, expecting event %d or %d, but target is waiting for event %d, so yield to thread %d.\n",
					curr_tcb->lwt_id, kthd_index, c->receiver->lwt_id, _LWT_WAIT_CHAN_RCV, _LWT_WAIT_CGRP_RCV, 
					c->receiver->wait_type, lwt_rdyq_head->lwt_id);
#endif
			target = lwt_rdyq_head;

			//yield
			curr_tcb->lwt_status = _LWT_STAT_RDY;
			__lwt_append_into_rdyq(curr_tcb);
			__lwt_remove_from_rdyq(target);
			if(target->lwt_status != _LWT_STAT_ZOMB)
				target->lwt_status = _LWT_STAT_RUN;
			__lwt_schedule(target);

			type = c->receiver->wait_type;
		}
		target = c->receiver;
#ifdef DBG
		printf("thread %d in kthd %d: now target %d in kthd %d is waiting for me, call it.\n",
				curr_tcb->lwt_id, kthd_index, target->lwt_id, c->receiver->kthd_index);
#endif
		//2.1:
		__lwt_chan_append_into_blkd_buf(c, curr_tcb);

		//		assert(target->lwt_status == _LWT_STAT_RDY || 
		//				target->lwt_status == _LWT_STAT_WAIT);
		curr_tcb->lwt_status = _LWT_STAT_WAIT;
		curr_tcb->wait_type = _LWT_WAIT_CHAN_SND;


		//0.5:
		__lwt_chan_triger_evnt(c, LWT_CHAN_RCV);



		if(c->receiver->kthd_index != kthd_index &&
				c->receiver->lwt_status == _LWT_STAT_WAIT)
		{
			__lwt_kthd_rbuf_write(c->receiver->kthd_index, _LWT_KTHDT_UNSET_TYPE, c->receiver->lwt_id);
			curr_tcb->lwt_status = _LWT_STAT_WAIT;
			target = lwt_rdyq_head;
		}
		//4
		if(target->lwt_status == _LWT_STAT_RDY)
			__lwt_remove_from_rdyq(target);

		target->lwt_status = _LWT_STAT_RUN;

		__lwt_schedule(target);

	}
	//4

#ifdef DBG
	printf("thread %d in kthd %d: unmark myself from waiting type %d into 0\n",
			curr_tcb->lwt_id, kthd_index, curr_tcb->wait_type);
#endif
	curr_tcb->wait_type = _LWT_WAIT_NOTHING;
	return 1;
}


	inline int 
__lwt_chan_append_into_sndr_buf(lwt_chan_t c, lwt_t sender)
{
	//target channel has invalid receiver
	if(unlikely(c->receiver == nil_tcb))
		return -1;
#ifdef DBG
	printf("thread %d in kthd %d: thread %d append onto sndr list of channel 0x%08x\n",
			curr_tcb->lwt_id, kthd_index, sender->lwt_id, (unsigned int) c);
#endif

	linked_buf* curr = c->senders;
	linked_buf* prev = nil;
	if(curr != 0)
	{
		//already in sender list
		if(unlikely(((_lwt_tcb*)(curr->self)) == sender ))
			return -1;
		prev = curr;
		curr = curr->next;
	}
	linked_buf *tmp = (linked_buf*)malloc(sizeof(linked_buf));
	tmp->self = sender;
	tmp->next = nil;

	//to here, curr will points to the last nil node.
	if(prev == nil)	//the sender is the first sender in the buffer
		c->senders = tmp;
	else
		prev->next = tmp;
	c->snd_cnt++;

	return 0;
}

void* __lwt_kthd_rbuf_monitor(void *args)
{
	//lwt_t target = nil_tcb;
	_lwt_tcb* target = nil_tcb;
	struct lwt_kthd_evnt evnt = nil_evnt;
	for(evnt = nil_evnt; ;evnt= __lwt_kthd_rbuf_read())
	{
		switch(evnt.token){
			case _LWT_KTHDT_NOTHING:

				//lwt_yield
				if(length_of_rdyq > 0)
				{
					target = lwt_rdyq_head;
					__lwt_remove_from_rdyq(target);
				}
				else 
					continue;
				break;
			case _LWT_KTHDT_UNSET_TYPE:
				//unset target wait_type:
				target = &lwt_lst_root[evnt.target_lwt];
#ifdef DBG
				printf("thread %d in kthd %d: control token _LWT_KTHDT_UNSET_TYPE rcved, for lwt_id %d, unset wait type\n",
						curr_tcb->lwt_id, kthd_index, target->lwt_id);
#endif
				assert(target != curr_tcb);
				target->wait_type = _LWT_WAIT_NOTHING;
				if(target->lwt_status == _LWT_STAT_ZOMB ||
						target->lwt_status == _LWT_STAT_RDY)
					__lwt_remove_from_rdyq(target);
				break;
			case _LWT_KTHDT_SCHD:
				target = &lwt_lst_root[evnt.target_lwt];
#ifdef DBG
				printf("thread %d in kthd %d: control token _LWT_KTHDT_SCHD rcved, for lwt_id %d, schedule into it\n",
						curr_tcb->lwt_id, kthd_index, target->lwt_id);
#endif
				if(target == curr_tcb)
					continue;
				//schedule into target
				if(target->lwt_status == _LWT_STAT_ZOMB ||
						target->lwt_status == _LWT_STAT_RDY)
					__lwt_remove_from_rdyq(target);
				break;
			case _LWT_KTHDT_DESTORY:
#ifdef DBG
				printf("thread %d in kthd %d: control token _LWT_KTHDT_DESTORY rcved, thread %d exiting.\n",
						curr_tcb->lwt_id, kthd_index, target->lwt_id);
#endif
				return NULL;
				break;
			default:
				perror("unspecified evnt rcved, ignore.\n");
				//exit(0);
				continue;

		}

		curr_tcb->lwt_status = _LWT_STAT_RDY;
		__lwt_append_into_rdyq(curr_tcb);
		if(target->lwt_status != _LWT_STAT_ZOMB)
			target->lwt_status = _LWT_STAT_RUN;
		__lwt_schedule(target);
	}
}

void* __lwt_kthd_wrapper(void *args)
{
	void* data = ((lwt_args_t *)args)->data;
	lwt_chan_t c = ((lwt_args_t *)args)->c;
	lwt_fn_t fn = ((lwt_args_t *)args)->fn;
	kthd_index = ((lwt_args_t *)args)->kthd_index;

	kthd_ring_buf = (struct lwt_kthd_evnt (*)[_LWT_KTHD_EVNT_BUF])malloc(sizeof(struct lwt_kthd_evnt) * _LWT_KTHD_EVNT_BUF);

	//init current kthd_tcb:
	kthds[kthd_index].kthd_index = kthd_index;
	assert(kthds[kthd_index].kthd_id == pthread_self());
	kthds[kthd_index].rbuf = kthd_ring_buf;
	kthds[kthd_index].kthd_rbuf_cons_p = &kthd_rbuf_cons;
	kthds[kthd_index].kthd_rbuf_prod_p = &kthd_rbuf_prod;
	kthds[kthd_index].kthd_rbuf_len_p = &kthd_rbuf_len;

#ifdef DBG
	printf("thread %d in kthd %d: in kthd_wrapper\n",curr_tcb->lwt_id, kthd_index);
#endif

	//same as lwt_create

	_lwt_tcb* target;
	//if the list is empty, then we init the root first.

	if(curr_tcb == NULL)
	{
		__before_main();
	}


	int id = __lwt_get_target();
	 target = __init_tcb(id);

	void* bp = target->lwt_ebp;
	target->lwt_esp = target->lwt_ebp - 12;
	if(c)
		*(int*)(bp-4) = (int)c;
	if(data)
		*(int*)(bp-8) = (int)data;
	*(lwt_fn_t*)(bp-12) = (lwt_fn_t)fn;
	target->flag = LWT_FLAG_NOJOIN;
	target->lwt_ip =(void*)__lwt_trampoline;


	kthds[kthd_index].done_init = 1;
#ifdef DBG
	printf("thread %d in kthd %d: lwt init done, start scheduling\n", 
			curr_tcb->lwt_id, kthd_index);
#endif



	//channel delegation
	c->receiver = target;

	//thread 0 will keep checking the kthd_rbuf to get control tokens.
	__lwt_kthd_rbuf_monitor((void*)NULL);


#ifdef DBG
	printf("thread %d in kthd %d: child kthd done\n",curr_tcb->lwt_id, kthd_index);
#endif

	return NULL;
}







#endif
