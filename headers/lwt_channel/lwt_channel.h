/*******************************************
*
* Author: Hongzhou Chen - hzchen_cs@gwmail.gwu.edu
*
*
* Last modified:	12-08-2013 22:35
*
* Filename:		lwt_channel.h
*
* Description: 		contains all functions about channel function
*
* functions:	lwt_chan
*				lwt_chan_deref
*				lwt_snd
*				lwt_rcv
*				lwt_snd_chan
*				lwt_rcv_chan
*				lwt_cgrp
*				lwt_cgrp_free
*				lwt_cgrp_add
*				lwt_cgrp_rem
*				lwt_cgrp_wait
*				lwt_chan_mark_set
*				lwt_chan_mark_get
*				lwt_snd_cdeleg
*				lwt_rcv_cdeleg
*
*
******************************************/
/**
 *
 *	lwt_channel.h: contains all functions about channel function
 *
 */

#ifndef __LWT_CHAN_H__
#define __LWT_CHAN_H__


#include <stdlib.h>
#include "../lwt_core/lwt_core.h"
#include "../lwt_types/lwt_types.h"


/**
 *	this function creates a channel referenced by the return value.
 *	The memory will be freed by function lwt_chan_deref.
 */
	inline lwt_chan_t 
lwt_chan(int sz)
{
	if(unlikely(NULL == nil))
	{
		nil = (linked_buf*)malloc(sizeof(linked_buf));
		nil->self = nil;
		nil->next = nil;
	}
	lwt_chan_t c = (lwt_chan_t)malloc(sizeof(struct lwt_channel));
	if(sz != 0)
		c->data_buf = (void*)malloc(sizeof(void*) * sz);
	else
		c->data_buf = nil;
	c->buf_len = 0;
	c->buf_sz = sz;
	c->prod_p = 0;
	c->cons_p = 0;
	c->blocked_senders = nil_tcb;
	c->blocked_senders_last = nil_tcb;
	c->blocked_len = 0;
	c->snd_cnt = 0;
	c->cgrp_snd = NULL;
	c->cgrp_rcv = NULL;
	c->receiver = curr_tcb;

#ifdef DBG
	printf("thread %d in kthd %d: channel 0x%08x constructed.\n", curr_tcb->lwt_id, kthd_index, (unsigned int)c);
#endif

	return c;
}


/**
 *	Dereferencing channel. If the receiver calls this function, set
 *	c->receiver to nil_tcb.
 *	When the c->snd_cnt == 0 && c->receiver == nil_tcb, do the following:
 *	1: recursively free data buf;
 *	2: recursively free senders;
 *	3: free itself.
 *
 */
	inline void 
lwt_chan_deref(lwt_chan_t c)
{
#ifdef DBG
	printf("thread %d in kthd %d: derefing channel 0x%08x.\n", curr_tcb->lwt_id, kthd_index, (unsigned int)c);
#endif
	if(unlikely(c==NULL))
		return;

	if(c->receiver != nil_tcb && curr_tcb == c->receiver)
	{
#ifdef DBG
		printf("thread %d in kthd %d: channel 0x%08x receiver derefed.\n",
				curr_tcb->lwt_id, kthd_index, (unsigned int)c);
#endif
		c->receiver = nil_tcb;
	}else
	{
#ifdef DBG
		printf("thread %d in kthd %d: channel sender derefed.\n", curr_tcb->lwt_id, (unsigned int)kthd_self);
#endif
		//fo through the dlinked buffer
		linked_buf* curr = c->senders;
		linked_buf* prev = c->senders;
		while(NULL != curr)
		{
			if(((lwt_t)(curr->self))== curr_tcb)
			{
				prev->next = curr->next;
				c->snd_cnt--;
				break;
			}
			prev = curr;
			curr = curr->next;
		}
	}
	if(c->snd_cnt == 0 && c->receiver == nil_tcb)
	{

#ifdef DBG
		printf("thread %d in kthd %d: channel 0x%08x deref accepted.\n",
				curr_tcb->lwt_id, kthd_index, (unsigned int)c);
#endif
	//	assert(c->buf_len == 0);		//no data_buf
		if(c->buf_len != 0)
			free(c->data_buf);
//TODO: sometimes sender dead, but len will still be 1
//assert(c->blocked_len == 0);	//no blocked_senders
//		assert(c->blocked_senders_last == nil);	//no blocked_senders_last
		//		printf("free c: 0x%08x\n", c);

		free(c);		
		c=NULL;
	}
}


/**
 *	create a data packet, then:
 *	-0.5: assert: data shouldn't be NULL!
 *	1: check channel alive or not 
 *		if receiver == nil_tcb
 *		1.1: return -1
 *	-1.5: construct data packet.
 *	1.6: put data packet into tcb
 *	2: call __lwt_chan_append_into_data_buf.
 *	2.1: after back here, free data packet.
 *	2.2: return.
 *	 
 */
	inline int
lwt_snd(lwt_chan_t c, void* data)
{
	//0.5
	//assert( NULL != data);

	//1
	if (unlikely(c->receiver == nil_tcb))
		return -1;

#ifdef DBG
	printf("thread %d in kthd %d: sending data %d to thread %d by channel 0x%08x\n", 
			curr_tcb->lwt_id, kthd_index, (int)data, c->receiver->lwt_id, (unsigned int)c);
#endif

	while(unlikely(curr_tcb->chan_data_useful))
	{
		//while my data is still avaliable, schedule else where
		lwt_t op_tcb = lwt_rdyq_head;

		curr_tcb->lwt_status = _LWT_STAT_RDY;
		__lwt_append_into_rdyq(curr_tcb);
		__lwt_remove_from_rdyq(op_tcb);
		if(op_tcb->lwt_status != _LWT_STAT_ZOMB)
			op_tcb->lwt_status = _LWT_STAT_RUN;
		__lwt_schedule(op_tcb);
	}

	curr_tcb->chan_data = data;
	curr_tcb->chan_data_useful = 1;
#ifdef DBG
	printf("thread %d in kthd %d: mark self as wait type %d: _LWT_WAIT_CHAN_SND\n",
			curr_tcb->lwt_id, kthd_index, _LWT_WAIT_CHAN_SND);
#endif
	curr_tcb->wait_type = _LWT_WAIT_CHAN_SND;

	//2
	return __lwt_chan_append_into_data_buf(c, data);
	
}


/**
 *	receive a data packet from channel, decrypt it and returns 
 *	curresponding content
 *	1: if curr_tcb != c->receiver, return nil
 *	2: call __lwt_chan_remove_from_data_buf
 *
 */

	inline void* 
lwt_rcv(lwt_chan_t c)
{
	//1
	if(unlikely(curr_tcb != c->receiver))
	{
		printf("thread %d in kthd %d: current thread %d is not the receiver of channel 0x%08x\n",
				curr_tcb->lwt_id, kthd_index, curr_tcb->lwt_id, (unsigned int)c);
		return nil;
	}
#ifdef DBG
	printf("thread %d in kthd %d: attempting to rcv on channel 0x%08x\n",
			curr_tcb->lwt_id, kthd_index, (unsigned int)c);
	printf("thread %d in kthd %d: mark self as wait type %d: _LWT_WAIT_CHAN_RCV\n",
			curr_tcb->lwt_id, kthd_index, _LWT_WAIT_CHAN_RCV);
#endif

	curr_tcb->wait_type = _LWT_WAIT_CHAN_RCV;
	void* data = __lwt_chan_remove_from_data_buf(c);

	curr_tcb->wait_type = _LWT_WAIT_NOTHING;

#ifdef DBG
	printf("thread %d in kthd %d: rcved data %d from channel 0x%08x.\n",
			curr_tcb->lwt_id, kthd_index, (int)data, (unsigned int)c);
#endif

	return data;
}

	inline void
lwt_snd_chan(lwt_chan_t sender, lwt_chan_t sendee)
{
	assert(sendee && sender);

	if(unlikely(sender->receiver == nil_tcb))
		return;

#ifdef DBG
	printf("thread %d in kthd %d: sending channel 0x%08x to thread %d by channel 0x%08x\n",
			curr_tcb->lwt_id, kthd_index, (unsigned int)sendee, sender->receiver->lwt_id, (unsigned int)sender);
	printf("thread %d in kthd %d: mark self as wait type %d: _LWT_WAIT_CHAN_SND\n",
			curr_tcb->lwt_id, kthd_index, _LWT_WAIT_CHAN_SND);
#endif

	while(unlikely(curr_tcb->chan_data_useful))
	{
		//while my data is still avaliable, schedule else where
		//lwt_t op_tcb = lwt_rdyq_head;
		lwt_t op_tcb = lwt_rdyq_head;

		curr_tcb->lwt_status = _LWT_STAT_RDY;
		__lwt_append_into_rdyq(curr_tcb);
		__lwt_remove_from_rdyq(op_tcb);
		if(op_tcb->lwt_status != _LWT_STAT_ZOMB)
			op_tcb->lwt_status = _LWT_STAT_RUN;
		__lwt_schedule(op_tcb);
	}


	curr_tcb->chan_data = (void*)sendee;
	curr_tcb->wait_type = _LWT_WAIT_CHAN_SND;
	curr_tcb->chan_data_useful = 1;


	__lwt_chan_append_into_sndr_buf(sendee, sender->receiver);
	__lwt_chan_append_into_data_buf(sender, (void*)sendee);

	return;

}

	inline lwt_chan_t 
lwt_rcv_chan(lwt_chan_t c)
{
	if(unlikely(curr_tcb != c->receiver))
	{
		printf("thread %d in kthd %d: current thread %d is not the receiver of channel 0x%08x\n",
				curr_tcb->lwt_id, kthd_index, curr_tcb->lwt_id, (unsigned int)c);
		return NULL;
	}
#ifdef DBG
	printf("thread %d in kthd %d: rcving channel from channel 0x%08x\n",
			curr_tcb->lwt_id, kthd_index, (unsigned int)c);
	printf("thread %d in kthd %d: mark self as wait type %d: _LWT_WAIT_CHAN_RCV\n",
			curr_tcb->lwt_id, kthd_index, _LWT_WAIT_CHAN_RCV);
#endif

	curr_tcb->wait_type = _LWT_WAIT_CHAN_RCV;

	lwt_chan_t  data_pkt = (lwt_chan_t)__lwt_chan_remove_from_data_buf(c);
#ifdef DBG
	printf("thread %d in kthd %d: channel 0x%08x was received through channel 0x%08x\n",
			curr_tcb->lwt_id, kthd_index, (unsigned int) data_pkt, (unsigned int)c);
#endif
	return data_pkt;
}

/***********	channel group support	*********/

	inline lwt_cgrp_t 
lwt_cgrp(void)
{
	lwt_cgrp_t cg = (lwt_cgrp_t)calloc(sizeof(struct lwt_cgrp), sizeof(char));
	cg->cl_len = 0;
	cg->snd_waiter = nil_tcb;
	cg->rcv_waiter = nil_tcb;

	cg->snd_prod_p = 0;
	cg->snd_cons_p = 0;
	cg->snd_evnt_cnt = 0;

	cg->rcv_prod_p = 0;
	cg->rcv_cons_p = 0;
	cg->rcv_evnt_cnt = 0;

	return cg;
}


/**
 * 	Free a channel group and return 0 only if there are no pending
 * 	events. If there are pending events, return -1, and do not delete 
 * 	the channel.
 * 	Do not free the associated	channels.
 */
	inline int
lwt_cgrp_free(lwt_cgrp_t cg)
{
	//if there are pending events return 1
//	if(unlikely(cg->evnt_cnt != 0))
//		return -1;

#ifdef DBG
	printf("thread %d in kthd %d: attempting to free cgrp 0x%08x\n",
			curr_tcb->lwt_id, kthd_index, (unsigned int)cg);
#endif

	//TODO: This is actually checking the channel length, not the event length. Fix it!
	if(cg->cl_len != 0)
		return -1;
	
	lwt_t waiter = cg->snd_waiter;

	//TODO: need to check what the waiter is waiting for. Fix it!
	if((waiter != nil_tcb) && (waiter->lwt_status == _LWT_STAT_WAIT))
	{
		waiter->lwt_status = _LWT_STAT_RDY;
		__lwt_append_into_rdyq(waiter);
	}	

	
	waiter = cg->rcv_waiter;

	if((waiter != nil_tcb) && (waiter->lwt_status == _LWT_STAT_WAIT))
	{
		waiter->lwt_status = _LWT_STAT_RDY;
		__lwt_append_into_rdyq(waiter);
	}

#ifdef DBG
	printf("thread %d in kthd %d: will free cg 0x%08x now\n",
			curr_tcb->lwt_id, kthd_index, (unsigned int)cg);
#endif

	free(cg);
	cg = NULL;

	return 0;
}


	inline int
lwt_cgrp_add(lwt_cgrp_t cg, lwt_chan_t c, lwt_chan_dir_t type)
{
#ifdef DBG
	printf("thread %d in kthd %d: channel 0x%08x added to cgrp 0x%08x on direction %d\n",
			curr_tcb->lwt_id, kthd_index, (unsigned int)c, (unsigned int)cg, type);
#endif
	switch(type){
		case LWT_CHAN_SND:
			if (unlikely (c->cgrp_snd != NULL))
				return -1;
			c->cgrp_snd = cg;
			if(c->blocked_len == 0)	//no one blocking, sendable
				__lwt_chan_triger_evnt(c, LWT_CHAN_SND);
			break;
		case LWT_CHAN_RCV:
			if (unlikely (c->cgrp_rcv != NULL))
				return -1;

			c->cgrp_rcv = cg;
			if(c->buf_len > 0 || c->blocked_len > 0)	//has data/ has blocking, recvable
				__lwt_chan_triger_evnt(c, LWT_CHAN_RCV);
			break;
		default:
			perror("undefined lwt_chan_dir_t received, exiting.\n");
			exit(0);
	}
	cg->cl_len ++;
	return 0;
}



	inline int
lwt_cgrp_rem(lwt_cgrp_t cg, lwt_chan_t c)
{
#ifdef DBG
	printf("thread %d in kthd %d: trying to remove channel 0x%08x from cgrp 0x%08x\n",
			curr_tcb->lwt_id, kthd_index, (unsigned int)c, (unsigned int)cg);
#endif
	if(c->cgrp_snd == cg)
		c->cgrp_snd = NULL;
	else if(c->cgrp_rcv == cg)
		c->cgrp_rcv = NULL;
	else
		return -1;

	cg->cl_len --;
	return 0;
}
/**
 *
 *	This is the function for which the others exist. This is
 *	a blocking function. The calling thread blocks unless there 
 *	is a pending event on one of the channels.
 *	A blocking thread will block until one of the channels has an event.
 *
 */
inline lwt_chan_t
lwt_cgrp_wait(lwt_cgrp_t cg, lwt_chan_dir_t direction)
{
	//if current thread is already in the list, then it must have been
	//blocked. So logically, it's impossible to have the same thread 
	//call this function with same cg twice.

	//simply add current thread into the wait list of group, and block it.

#ifdef DBG
	printf("thread %d in kthd %d: waiting for cgrp 0x%08x on direction %d\n",
			curr_tcb->lwt_id, kthd_index, (unsigned int)cg, direction);
#endif

	int* evnt_cnt = NULL;

	if(direction == LWT_CHAN_SND)
	{
#ifdef DBG
		printf("thread %d in kthd %d: mark self as wait type %d: _LWT_WAIT_CGRP_SND\n",
				curr_tcb->lwt_id, kthd_index, _LWT_WAIT_CGRP_SND);
#endif
		cg->snd_waiter = curr_tcb;
		evnt_cnt = &(cg->snd_evnt_cnt);
		curr_tcb->wait_type = _LWT_WAIT_CGRP_SND;
	}else if (direction == LWT_CHAN_RCV){
#ifdef DBG
		printf("thread %d in kthd %d: mark self as wait type %d: _LWT_WAIT_CGRP_RCV\n",
				curr_tcb->lwt_id, kthd_index, _LWT_WAIT_CGRP_RCV);
#endif
		cg->rcv_waiter = curr_tcb;
		evnt_cnt = &(cg->rcv_evnt_cnt);
		curr_tcb->wait_type = _LWT_WAIT_CGRP_RCV;

	}else if (direction == LWT_CHAN_NULL){
		if(cg->rcv_evnt_cnt > 0){
#ifdef DBG
			printf("thread %d in kthd %d: wait on no direction, mark self as wait type %d: _LWT_WAIT_CGRP_RCV\n",
					curr_tcb->lwt_id, kthd_index, _LWT_WAIT_CGRP_RCV);
#endif
			cg->rcv_waiter = curr_tcb;
			evnt_cnt = &(cg->rcv_evnt_cnt);
			curr_tcb->wait_type = _LWT_WAIT_CGRP_RCV;
		}else
		{
#ifdef DBG
			printf("thread %d in kthd %d: wait on no direction, mark self as wait type %d: _LWT_WAIT_CGRP_SND by default\n",
					curr_tcb->lwt_id, kthd_index, _LWT_WAIT_CGRP_SND);
#endif
			cg->snd_waiter = curr_tcb;
			evnt_cnt = &(cg->snd_evnt_cnt);
			curr_tcb->wait_type = _LWT_WAIT_CGRP_SND;
		}
	}
	


	while(*evnt_cnt == 0)
	{
		lwt_t op_tcb = lwt_rdyq_head;
		__lwt_remove_from_rdyq(op_tcb);
		curr_tcb->lwt_status = _LWT_STAT_RDY;
		__lwt_append_into_rdyq(curr_tcb);
		op_tcb->lwt_status = _LWT_STAT_RUN;

		__lwt_schedule(op_tcb);
	}
	assert(*evnt_cnt > 0);
	
#ifdef DBG
	printf("thread %d in kthd %d: wait for cgrp 0x%08x returns with evnt_cnt %d\n",
			curr_tcb->lwt_id, kthd_index, (unsigned int)cg, *evnt_cnt);
#endif

	return __lwt_chan_consume_evnt(cg, direction);

}

inline void
lwt_chan_mark_set(lwt_chan_t c, void* mark)
{
#ifdef DBG
	printf("thread %d in kthd %d: set mark %d to channel 0x%08x\n",
			curr_tcb->lwt_id, kthd_index, (unsigned int) mark, (unsigned int)c);
#endif
	c->mark = mark;
	
	return;
}


static inline void*
lwt_chan_mark_get(lwt_chan_t c)
{
#ifdef DBG
	printf("thread %d in kthd %d: get mark %d from channel 0x%08x\n",
			curr_tcb->lwt_id, kthd_index, (unsigned int) c->mark, (unsigned int)c);
#endif
	return c->mark;
}

/*********channel delegation*************/

/**
 * Send channel delegating to channel c;
 * Increase the sender counter of channel c;
 * change the receiver of channel delegation into the receiver of channel c.
 */
static inline void
lwt_snd_cdeleg(lwt_chan_t c, lwt_chan_t delegating)
{
	assert(c && delegating);

	if(unlikely(c->receiver == nil_tcb))
		return;

#ifdef DBG
	printf("thread %d in kthd %d: delegating channel 0x%08x to thread %d by channel 0x%08x\n",
			curr_tcb->lwt_id, kthd_index, (unsigned int)delegating, c->receiver->lwt_id, (unsigned int)c);
	printf("thread %d in kthd %d: mark self as wait type %d: _LWT_WAIT_CHAN_SND\n",
			curr_tcb->lwt_id, kthd_index, _LWT_WAIT_CHAN_SND);
#endif

	while(unlikely(curr_tcb->chan_data_useful))
	{
		//while my data is still avaliable, schedule else where
		lwt_t op_tcb = lwt_rdyq_head;

		curr_tcb->lwt_status = _LWT_STAT_RDY;
		__lwt_append_into_rdyq(curr_tcb);
		__lwt_remove_from_rdyq(op_tcb);
		if(op_tcb->lwt_status != _LWT_STAT_ZOMB)
			op_tcb->lwt_status = _LWT_STAT_RUN;
		__lwt_schedule(op_tcb);
	}
	curr_tcb->chan_data = (void*)delegating;
	curr_tcb->chan_data_useful = 1;

	delegating->receiver = c->receiver;
	curr_tcb->wait_type = _LWT_WAIT_CHAN_SND;


	__lwt_chan_append_into_sndr_buf(delegating, c->receiver);
	__lwt_chan_append_into_data_buf(c, (void*)delegating);

	return;

}

static inline lwt_chan_t
lwt_rcv_cdeleg(lwt_chan_t c)
{
	if(unlikely(curr_tcb != c->receiver))
		return NULL;
#ifdef DBG
	printf("thread %d in kthd %d: rcving delegating channel from channel 0x%08x\n",
			curr_tcb->lwt_id, kthd_index, (unsigned int)c);
	printf("thread %d in kthd %d: mark self as wait type %d: _LWT_WAIT_CHAN_RCV\n",
			curr_tcb->lwt_id, kthd_index, _LWT_WAIT_CHAN_RCV);
#endif

	curr_tcb->wait_type = _LWT_WAIT_CHAN_RCV;

	lwt_chan_t  data = (lwt_chan_t)__lwt_chan_remove_from_data_buf(c);


#ifdef DBG
	printf("thread %d in kthd %d: channel 0x%08x was received and delegated through channel 0x%08x\n",
			curr_tcb->lwt_id, kthd_index, (unsigned int) data, (unsigned int)c);
#endif
	return data;

}


#endif
