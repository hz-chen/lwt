/*******************************************
*
* Author: HzChen - hzchen_cs@gwmail.gwu.edu
*
*
* Last modified:	04-10-2014 13:55
*
* Filename:		lwt_types.c
*
* Description: 
*
* Input: 
*
* Output: 
*
*
******************************************/
#include <lwt/lwt_types.h>

__thread struct lwt_kthd_evnt nil_evnt;
__thread _lwt_tcb* nil_tcb = NULL;
__thread lwt_chan_t nil_channel = NULL;
