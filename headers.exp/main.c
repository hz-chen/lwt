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


#define IS_RESET()						\
        assert( lwt_info(LWT_INFO_NTHD_RUNNABLE) == 1 &&	\
		lwt_info(LWT_INFO_NTHD_ZOMBIES) == 0 &&		\
		lwt_info(LWT_INFO_NTHD_BLOCKED) == 0\
		)


#define ITER 1000000

void* fn(void* data, lwt_chan_t c)
{
	return NULL;
}

int test_basic_create_join()
{
	int i=0;
	unsigned long long start, end;
	lwt_t tid1, tid2, tid3;

	tid1 = lwt_create(fn, NULL, 0, 0);
	lwt_join(tid1);
	IS_RESET();

	rdtscll(start);
	for(i=0 ; i < ITER; i++)
	{
		tid1 = lwt_create(fn, NULL, 0, 0);
		lwt_join(tid1);
	}
	rdtscll(end);
	IS_RESET();
	printf("performance of fork/join: --> %lld\n", (end-start)/ITER);

	for(i=0 ; i < ITER; i++)
	{
		tid1 = lwt_create(fn, NULL, 0, 0);
		lwt_yield(LWT_NULL);
		tid2 = lwt_create(fn, NULL, 0, 0);
		lwt_yield(LWT_NULL);
		tid3 = lwt_create(fn, NULL, 0, 0);
		lwt_join(tid3);
		lwt_yield(LWT_NULL);
		lwt_join(tid1);
		lwt_yield(LWT_NULL);
		lwt_join(tid2);
	}

	IS_RESET();



	return 0;
}

int main(int argc, char** argv)
{

	IS_RESET();
	test_basic_create_join();
	IS_RESET();

	printf("back to main\n");
	return EXIT_SUCCESS;
}
