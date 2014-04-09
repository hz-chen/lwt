/*******************************************
*
* Author: Hongzhou Chen - hzchen_cs@gwmail.gwu.edu
*
*
* Last modified:	04-09-2014 09:43
*
* Filename:		lwt_core.c
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

#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <lwt/lwt_types.h>

/*****public data structures*****/

struct lwt_kthd_tcb kthds[_LWT_KTHD_MAX_NUM];
int kthd_num;

/*****private data of thread*****/

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
