/*******************************************
*
* Author: Hongzhou Chen - hzchen_cs@gwmail.gwu.edu
*
*
* Last modified:	12-08-2013 22:36
*
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
******************************************/

#ifndef __LWT_H__
#define __LWT_H__

#include <lwt/lwt_types.h>
#include <lwt/lwt_channel.h>

//auxiliary function definitions:
typedef enum {
	LWT_INFO_NTHD_RUNNABLE,
	LWT_INFO_NTHD_ZOMBIES,
	LWT_INFO_NTHD_BLOCKED
} lwt_info_t;

extern __thread int num_of_zombie;
extern __thread int num_of_block;

extern lwt_t
lwt_create(lwt_fn_t fn, void* data, int flag, lwt_chan_t c);

extern 
void  lwt_yield(lwt_t target);


extern 
void* lwt_join(lwt_t target);

extern 
void lwt_die(void* retVal);


extern 
lwt_t lwt_current(void);

extern
int lwt_info(lwt_info_t info);


extern
int lwt_kthd_create(lwt_fn_t fn, void *data, lwt_chan_t c);

#endif
