/*******************************************
 *
 * Author: Hongzhou Chen - hongzhoc@amazon.com
 * Filename:		lwt.h
 *
 * Description: 		contains general user level interfaces
 *
 * functions: 
 * 				lwt_t lwt_create(lwt_fn_t fn, void* data, int flag, lwt_chan_t c)
 * 				int lwt_yield(lwt_t)
 * 				void* lwt_join(lwt_t)
 * 				void lwt_die()
 * 				lwt_t lwt_current(void)
 * 				int lwt_info(lwt_info_t);
 *
 *
 *
 ******************************************/

#ifndef __LWT_H__
#define __LWT_H__

//#include <lwt/lwt_types.h>
//#include <lwt/lwt_core.h>
//#include <lwt/lwt_channel.h>

#include "lwt_types/lwt_types.h"
#include "lwt_core/lwt_core.h"
#include "lwt_channel/lwt_channel.h"

//auxiliary functions and definitions:
typedef enum {
	LWT_INFO_NTHD_RUNNABLE,
	LWT_INFO_NTHD_ZOMBIES,
	LWT_INFO_NTHD_BLOCKED
} lwt_info_t;

__thread int num_of_zombie=0;
__thread int num_of_block=0;

int lwt_info(lwt_info_t info);

static inline lwt_t
lwt_create(lwt_fn_t fn, void* data, int flag, lwt_chan_t c)
{
	_lwt_tcb* target_tcb;
	//if the list is empty, then we init the root first.
	assert(curr_tcb);

	int id = __lwt_get_target();
	target_tcb = __init_tcb(id);
	
	void* bp = target_tcb->lwt_ebp;
	target_tcb->lwt_esp = target_tcb->lwt_ebp - 12;
	if(c)
		*(int*)(bp-4) = (int)c;
	if(data)
		*(int*)(bp-8) = (int)data;
	*(lwt_fn_t*)(bp-12) = (lwt_fn_t)fn;
	target_tcb->flag = flag;
	target_tcb->lwt_ip =(void*)__lwt_trampoline;


	return target_tcb;
}

static inline
void  lwt_yield(lwt_t target)
{
	_lwt_tcb* op_tcb = target;
	if(target == nil_tcb)
	{
		if(unlikely(length_of_rdyq == 0))
			return;
		else
			op_tcb = lwt_rdyq_head;

	}

	curr_tcb->lwt_status = _LWT_STAT_RDY;
	__lwt_append_into_rdyq(curr_tcb);
	__lwt_remove_from_rdyq(op_tcb);
	if(op_tcb->lwt_status != _LWT_STAT_ZOMB)
		op_tcb->lwt_status = _LWT_STAT_RUN;
	__lwt_schedule(op_tcb);
}


static inline
void* lwt_join(lwt_t target)
{
#ifdef DBG
	printf("thread %d in kthd %d: trying to join thread %d\n",
			curr_tcb->lwt_id, kthd_index, target->lwt_id);
#endif


	_lwt_tcb* target_tcb = target;
	_lwt_tcb* op_tcb = target_tcb;

	if(unlikely(curr_tcb != target_tcb->parent) ||
		(target_tcb->flag == LWT_FLAG_NOJOIN))
	{
#ifdef DBG
		printf("thread %d in kthd %d: trying to thread %d from non-parent thread, yield\n",
				curr_tcb->lwt_id, kthd_index, target->lwt_id);
#endif
		lwt_yield(target);
		return NULL;
	}


	num_of_block++;
	//lwt_lst_root[target].joined = curr_tcb;
	target_tcb->joined = curr_tcb;

	do{
		op_tcb = target_tcb;
		curr_tcb->lwt_status = _LWT_STAT_WAIT;
		curr_tcb->wait_type = _LWT_WAIT_JOIN;

	//	if (unlikely(lwt_lst_root[target].lwt_status == _LWT_STAT_WAIT))
		if ((target_tcb->lwt_status == _LWT_STAT_WAIT))
			op_tcb = lwt_rdyq_head;

#ifdef DBG
		printf("thread %d in kthd %d: target thread %d is in status %d, pick thread %d as operand.\n",
				curr_tcb->lwt_id, kthd_index, target_tcb->lwt_id, target_tcb->lwt_status, op_tcb->lwt_id);
#endif


		__lwt_remove_from_rdyq(op_tcb);

		if(op_tcb->lwt_status == _LWT_STAT_RDY)
			op_tcb->lwt_status = _LWT_STAT_RUN;

		__lwt_schedule(op_tcb);
	}while(target_tcb->lwt_status != _LWT_STAT_ZOMB);
#ifdef DBG
	printf("thread %d in kthd %d: killing thread %d\n", curr_tcb->lwt_id, kthd_index, target_tcb->lwt_id);
#endif


	curr_tcb->lwt_status = _LWT_STAT_RUN;
	op_tcb->lwt_status = _LWT_STAT_DEAD;
	num_of_block--;
	void* ret = target_tcb->ret_val;
	__lwt_append_into_deadq(target);
	__lwt_free(target);
	return ret;
}


void lwt_die(void* retVal)
{
	_lwt_tcb* op_tcb;
	curr_tcb->ret_val = retVal;
	curr_tcb->lwt_status = _LWT_STAT_ZOMB;
#ifdef DBG
	printf("thread %d in kthd %d: Now I'm a ZOMBIE!!!\n", curr_tcb->lwt_id, kthd_index);
#endif
	num_of_zombie++;

	while(1)
	{

		//if someone joined me and that one is waiting for me
		if(curr_tcb->joined != nil_tcb&&
				curr_tcb->joined->wait_type == _LWT_WAIT_JOIN)
		{
			num_of_zombie--;
#ifdef DBG
			printf("thread %d in kthd %d: I gonna die!!\n", curr_tcb->lwt_id, kthd_index);
#endif
			//let parent go
			op_tcb = curr_tcb->parent;

			if(op_tcb->lwt_status == _LWT_STAT_RDY)
				__lwt_remove_from_rdyq(op_tcb);

			op_tcb->lwt_status = _LWT_STAT_RUN;

			__lwt_schedule(op_tcb);
		}else
		{
			if(unlikely(curr_tcb->flag == LWT_FLAG_NOJOIN))
			{

#ifdef DBG
			printf("thread %d in kthd %d: I've been flagged with NOJOIN.\n",
					curr_tcb->lwt_id, kthd_index);
#endif

				curr_tcb->lwt_status = _LWT_STAT_DEAD;
				__lwt_append_into_deadq(curr_tcb);
				num_of_zombie--;
				__lwt_free(curr_tcb);
			}
			else
			{
#ifdef DBG
			printf("thread %d in kthd %d: no one joins me.\n",
					curr_tcb->lwt_id, kthd_index);
#endif
				curr_tcb->lwt_status = _LWT_STAT_ZOMB;
				__lwt_append_into_rdyq(curr_tcb);
			}

			op_tcb = lwt_rdyq_head;
			__lwt_remove_from_rdyq(op_tcb);

			op_tcb->lwt_status = _LWT_STAT_RUN;

			__lwt_schedule(op_tcb);
		}
	}
}


lwt_t lwt_current(void)
{
	assert(curr_tcb);
	return curr_tcb;
}

int lwt_info(lwt_info_t info)
{
	int ret=0;
	switch(info){
		case LWT_INFO_NTHD_RUNNABLE:
			//we have to add one, for the currently 
			//running thread is not in rdyq
			return __lwt_get_rdyq_len() +1;
			break;
		case LWT_INFO_NTHD_ZOMBIES:
			ret = num_of_zombie;
			return ret;
			break;
		case LWT_INFO_NTHD_BLOCKED:
			ret = num_of_block;
			return ret;
			break;
	}
	return 0;
}


/***
 * This function creates both a kernel-scheduled thread 
 * (a pthread in our case) and a lwt that is created to run on that
 * kthd. That lwt thread on the kthd calls the fn as with 
 * the normal lwt create API sending the data and delegated 
 * channel as normal. The difference here is that the lwt thread 
 * is executing on a separate kthd. Note that we no longer return
 * a lwt_t. Instead, return 0 if we successfully create the
 * thread, and -1 otherwise.
 *
 * This function will also create a monitor lwt thread keep
 * monitoring the ring_buffer so active with others.
 */
int lwt_kthd_create(lwt_fn_t fn, void *data, lwt_chan_t c)
{


	if(kthds[0].kthd_id == 0)
	{
		kthds[0].kthd_id = pthread_self();
		kthd_ring_buf = (struct lwt_kthd_evnt (*)[_LWT_KTHD_EVNT_BUF])malloc(sizeof(struct lwt_kthd_evnt) * _LWT_KTHD_EVNT_BUF);
		kthds[0].rbuf = kthd_ring_buf;
		kthds[0].kthd_rbuf_cons_p = &kthd_rbuf_cons;
		kthds[0].kthd_rbuf_prod_p = &kthd_rbuf_prod;
		kthds[0].kthd_rbuf_len_p = &kthd_rbuf_len;
		kthds[0].done_init = 1;
	}


	//******************create monitor thread***********/
	assert(curr_tcb);

	int target_id= __lwt_get_target();
	_lwt_tcb* target_tcb = __init_tcb(target_id);
	
	void* bp = target_tcb->lwt_ebp;
	target_tcb->lwt_esp = target_tcb->lwt_ebp - 12;
	/**** construct stack on args structure******/
	/*
	if(c)
		*(int*)(bp-4) = (int)c;
	if(data)
		*(int*)(bp-8) = (int)data;
	*/
	/**********/
	*(lwt_fn_t*)(bp-12) = (lwt_fn_t)__lwt_kthd_rbuf_monitor;
	target_tcb->flag = LWT_FLAG_NOJOIN;
	target_tcb->lwt_ip =(void*)__lwt_trampoline;


	/*****************create pthread*************************/

	int tmp_num;
	int new_num;

	do{
		tmp_num = kthd_num;
		new_num = tmp_num + 1;
	}while(!( __cas(&kthd_num, tmp_num, new_num)));

	lwt_args_t* args = (lwt_args_t*)malloc(sizeof(lwt_args_t));
	args->data = data;
	if(c)
		args->c = c;
	else
		args->c = lwt_chan(2);
	args->fn = fn;
	args->kthd_index = new_num;
	pthread_create(&(kthds[new_num].kthd_id), NULL, __lwt_kthd_wrapper, args);

#ifdef DBG
	printf("create done\n");
#endif

	return 0;
}

#endif
