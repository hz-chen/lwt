/*******************************************
*
* Author: Hongzhou Chen - hzchen_cs@gwmail.gwu.edu
*
*
* Last modified:	04-09-2014 07:55
*
* Filename:		lwt_channel.h
*
* Description: 		contains all function declarations 
* 		about channel function
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


#include <lwt/lwt_types.h>


/**
 *	this function creates a channel referenced by the return value.
 *	The memory will be freed by function lwt_chan_deref.
 */
	extern lwt_chan_t 
lwt_chan(int sz);


/**
 *	Dereferencing channel. 
 */
	extern void 
lwt_chan_deref(lwt_chan_t c);


/**
 *	create a data packet, then send it
 */
	extern int
lwt_snd(lwt_chan_t c, void* data);


/**
 *	receive a data packet from channel, decrypt it and returns 
 *	curresponding content
 *
 */

	extern void* 
lwt_rcv(lwt_chan_t c);

	extern void
lwt_snd_chan(lwt_chan_t sender, lwt_chan_t sendee);

	extern lwt_chan_t 
lwt_rcv_chan(lwt_chan_t c);

/***********	channel group support	*********/

extern lwt_cgrp_t 
lwt_cgrp(void);


/**
 * 	Free a channel group and return 0 only if there are no pending
 * 	events. If there are pending events, return -1, and do not delete 
 * 	the channel.
 * 	Do not free the associated	channels.
 */
	extern int
lwt_cgrp_free(lwt_cgrp_t cg);

	extern int
lwt_cgrp_add(lwt_cgrp_t cg, lwt_chan_t c, lwt_chan_dir_t type);

	extern int
lwt_cgrp_rem(lwt_cgrp_t cg, lwt_chan_t c);

/**
 *
 *	This is the function for which the others exist. This is
 *	a blocking function. The calling thread blocks unless there 
 *	is a pending event on one of the channels.
 *	A blocking thread will block until one of the channels has an event.
 *
 */
extern lwt_chan_t
lwt_cgrp_wait(lwt_cgrp_t cg, lwt_chan_dir_t direction);

extern void
lwt_chan_mark_set(lwt_chan_t c, void* mark);

extern void*
lwt_chan_mark_get(lwt_chan_t c);

/*********channel delegation*************/

/**
 * Send channel delegating to channel c;
 * Increase the sender counter of channel c;
 * change the receiver of channel delegation into the receiver of channel c.
 */
extern void
lwt_snd_cdeleg(lwt_chan_t c, lwt_chan_t delegating);

extern lwt_chan_t
lwt_rcv_cdeleg(lwt_chan_t c);


#endif
