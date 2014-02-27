/*******************************************
*
* Author: Hongzhou Chen - hzchen_cs@gwmail.gwu.edu
*
*
* Last modified:	12-04-2013 16:04
*
* Filename:		main.c
*
* Description: 
*
* Input: 
*
* Output: 
*
*
******************************************/
#include <stdio.h>
#include <stdlib.h>

#include "lwt.h"

#define rdtscll(val) __asm__ __volatile__("rdtsc" : "=A" (val))

#define ITER 3


void* grp_wait_snder(void* param, lwt_chan_t c)
{
	lwt_chan_t cc2 = lwt_chan(10);
	lwt_snd_chan((lwt_chan_t)param, cc2);
	lwt_chan_t cg_a[ITER];

	int i=0;
	for(i=0; i<ITER; i++)
	{
		printf("in_main: thread %d: waiting incoming channel %d\n", current_tid, i);
		cg_a[i] = lwt_rcv_chan(cc2);
		printf("in_main: thread %d: channel %d rcved.\n", current_tid, i);
	}

	
	i--;
	for(;i>=0; i--)
	{
		printf("in_main: thread %d: sending %d\n", current_tid, i);
		lwt_snd(cg_a[i], (void*)i);
		printf("in_main: thread %d: %d sent\n", current_tid, i);
	}

	return NULL;

}

void* grp_wait_rcver(void* param, lwt_chan_t c)
{
	lwt_chan_t cc = lwt_chan(10);
	lwt_snd_chan((lwt_chan_t)param, cc);
	lwt_chan_t cc2 = lwt_rcv_chan(cc);
	lwt_cgrp_t cg = lwt_cgrp();
	int i=0;
	for(i=0; i < ITER; i++)
	{
		lwt_chan_t cci = lwt_chan(3);
		assert(lwt_cgrp_add(cg, cci, LWT_CHAN_RCV) == 0);
		printf("in_main: thread %d: sending channel %d\n", current_tid, i);
		lwt_snd_chan(cc2, cci);
		printf("in_main: thread %d: channel %d sent\n", current_tid, i);
	}

	while(i>0)
	{
		printf("in_main: thread %d: group waiting\n", current_tid);
		lwt_chan_t cci = lwt_cgrp_wait(cg, LWT_CHAN_RCV);
		printf("in_main: thread %d: cci rcved, start rcv data\n", current_tid);
		i = (int)lwt_rcv(cci);
		printf("in_main: thread %d: i rcved: %d\n", current_tid, i);
	}

	return NULL;

}



int test_grp_wait_with_buffer()
{

	lwt_chan_t main_c = lwt_chan(1);
	lwt_t id1 = lwt_create(grp_wait_snder, (void*)main_c, 0, 0);
	lwt_chan_t snd_c = lwt_rcv_chan(main_c);
	lwt_t id2 = lwt_create(grp_wait_rcver, (void*)main_c, 0, 0);
	lwt_chan_t rcv_c = lwt_rcv_chan(main_c);
	lwt_snd_chan(rcv_c, snd_c);

	lwt_join(id1);
	lwt_join(id2);

	return 0;
}

void* kthd_rcv_print(void* data, lwt_chan_t rcv_c)
{
	printf("in kthd: data: %d and channel 0x%08x rcvd!\n",
			(int)data, (unsigned int)rcv_c);


	lwt_chan_t cc = lwt_chan(0);
	lwt_chan_t main_c = lwt_rcv_chan(rcv_c);
	lwt_snd_chan(main_c, cc);


	/*
	   void* pkg = lwt_rcv(cc);
	   printf("in kthd: data %d rcvd!\n", (int)pkg);
	   pkg = lwt_rcv(cc);
	   printf("in kthd: data %d rcvd!\n", (int)pkg);
	   pkg = lwt_rcv(cc);
	   printf("in kthd: data %d rcvd!\n", (int)pkg);
	   pkg = lwt_rcv(cc);
	   printf("in kthd: data %d rcvd!\n", (int)pkg);
	   pkg = lwt_rcv(cc);
	   printf("in kthd: data %d rcvd!\n", (int)pkg);
	   */

	lwt_cgrp_t cg = lwt_cgrp();
	lwt_cgrp_add(cg, cc, LWT_CHAN_RCV);
	lwt_chan_t actived;
	printf("====start wait====\n");
	while((actived = lwt_cgrp_wait(cg, LWT_CHAN_RCV)) != NULL)
	{
		assert(actived == cc);
		void* pkg = lwt_rcv(actived);
		printf("in kthd: data %d rcvd!\n", (int)pkg);
	}
	printf("====end wait====\n");
	return NULL;
}

void* kthd_snd_print(void* data, lwt_chan_t snd_c)
{

	lwt_chan_t cc = lwt_rcv_chan(snd_c);
	printf("channel 0x%08x rcvd!\n", (unsigned int)cc);

	printf("====start send====\n");
	lwt_snd(cc, (void*)0x10);
	lwt_snd(cc, (void*)0x20);
	lwt_snd(cc, (void*)0x30);
	lwt_snd(cc, (void*)0x40);
	lwt_snd(cc, (void*)0x50);
	printf("====end send====\n");
	return NULL;

}

void* fn(void* data, lwt_chan_t c)
{
//	printf("===========in fn, no join===========\n");
	return NULL;
}

int test_kthd()
{
	lwt_chan_t rcv_c = lwt_chan(0);
	lwt_chan_t snd_c = lwt_chan(0);
	lwt_kthd_create(&kthd_rcv_print, (void*)0x1, rcv_c);
	lwt_kthd_create(&kthd_snd_print, (void*)0x1, snd_c);
	lwt_chan_t main_c = lwt_chan(0);
	lwt_snd_chan(rcv_c, main_c);
	lwt_chan_t cc = lwt_rcv_chan(main_c);
	lwt_snd_chan(snd_c, cc);

//	sleep(1);
	//kthd_snd_print(rcv_c);
	return 0;
}

int test_nojoin()
{

	lwt_create(fn, NULL, LWT_FLAG_NOJOIN, NULL);
	assert(num_of_threads == 2);
	lwt_yield(_LWT_NULL);
	assert(num_of_threads == 1);
	return 0;
}

int test_kthd_iter()
{
	lwt_chan_t c = lwt_chan(3);

	unsigned long long start, end;
	rdtscll(start);
	
	int i=0;
	for(i=0; i< ITER; i++)
		lwt_kthd_create(&fn, (void*)0x1, c);

	rdtscll(end);

	printf("%lld <- kthd_create\n", (end-start)/(ITER));
	return 0;
}

int main(int argc, char** argv)
{

//	printf("testing grp_wait...\n");
//	test_grp_wait_with_buffer();

//	test_nojoin();
	test_kthd();
	//test_kthd_iter();

	printf("back to main\n");
	sleep(1);
	lwt_yield(_LWT_NULL);
	return EXIT_SUCCESS;
}
