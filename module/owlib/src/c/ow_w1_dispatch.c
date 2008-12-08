/*
$Id$
W1 Announce -- daemon  for showing w1 busmasters using Avahi
Written 2008 Paul H Alfille
email: paul.alfille@gmail.com
Released under the GPLv2
Much thanks to Evgeniy Polyakov
This file itself  is amodestly modified version of w1d by Evgeniy Polyakov
*/

/*
 * 	w1d.c
 *
 * Copyright (c) 2004 Evgeniy Polyakov <johnpol@2ka.mipt.ru>
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#include <config.h>
#include "owfs_config.h"

#if OW_W1

#include "ow_w1.h"
#include "ow_connection.h"

static struct {
	struct nlmsghdr	nlm ;
	struct cn_msg 	cn ;
	struct w1_netlink_msg	w1m ;
} nl_wait = {
	{
		.nlmsg_len = W1_NLM_LENGTH + W1_CN_LENGTH + W1_W1M_LENGTH,
		.nlmsg_seq = MAKE_NL_SEQ(0,0),
		.nlmsg_type = NLMSG_DONE,
		.nlmsg_pid = 0,
	} ,
	{
		.seq = MAKE_NL_SEQ(0,0),
		.len = W1_W1M_LENGTH,
		.ack = 0,
	} ,
	{
		.len = 0,
		.type = W1_MASTER_CMD,
	} ,
};
static struct netlink_parse nlp_wait = {
	&nl_wait.nlm,
	&nl_wait.cn,
	&nl_wait.w1m,
	NULL,
	NULL,
	0,
} ;

static int W1_write_pipe( int file_descriptor, struct netlink_parse * nlp )
{
	int size = nlp->nlm->nlmsg_len ;
	do {
		int select_value ;
		struct timeval tv = { Globals.timeout_w1, 0 } ;
		
		fd_set writeset ;
		FD_ZERO(&writeset) ;
		FD_SET(file_descriptor,&writeset) ;
		
		select_value = select(file_descriptor+1,NULL,&writeset,NULL,&tv) ;
		
		if ( select_value == -1 ) {
			if (errno != EINTR) {
				ERROR_CONNECT("Pipe (w1) Select returned -1\n");
				return -1 ;
			}
		} else if ( select_value == 0 ) {
			LEVEL_DEBUG("Select returned zero (timeout)\n");
			return -1;
		} else {
			int write_ret = write( file_descriptor, (const void *)nlp->nlm, size ) ;
			if (write_ret < 0 ) {
				ERROR_DEBUG("Trouble writing to pipe\n");
			} else if ( write_ret < size ) {
				LEVEL_DEBUG("Only able to write %d of %d bytes to pipe\n",write_ret, size);
				return -1 ;
			}
			return 0 ;
		}
	} while (1) ;
}


static void Dispatch_Packet( struct netlink_parse * nlp)
{
	int bus = NL_BUS(nlp->nlm->nlmsg_seq) ;
	struct connection_in * in ;
	struct connection_in * target = NULL ;
	
	CONNIN_RLOCK ;
	for ( in = Inbound_Control.head ; in != NULL ; in = in->next ) {
		//printf("Matching %d/%s/%s/%s/ to bus.%d %d/%s/%s/%s/\n",bus_zero,name,type,domain,now->index,now->busmode,now->connin.tcp.name,now->connin.tcp.type,now->connin.tcp.domain);
		if ( in->busmode != bus_w1 ) {
			continue ;
		} else if ( in->connin.w1.id == bus ) {
			target = in ; // defer until after pause messages
		} else if ( in->connin.w1.awaiting_response ) {
			LEVEL_DEBUG("Sending a pause to w1_bus_master%d\n",in->connin.w1.id);
			W1_write_pipe(in->connin.w1.write_file_descriptor, &nlp_wait ) ;
		}
	}
	CONNIN_RUNLOCK ;
	if ( bus == 0 ) {
		LEVEL_DEBUG("Sending this packet to root bus\n");
		W1_write_pipe(Inbound_Control.w1_write_file_descriptor, nlp) ;
	} else if ( target != NULL ) {
		LEVEL_DEBUG("Sending this packet to w1_bus_master%d\n",bus);
		W1_write_pipe(target->connin.w1.write_file_descriptor, nlp) ;
	}
}

void * W1_Dispatch( void * v )
{
	(void) v;

#if OW_MT
	pthread_detach(pthread_self());
#endif                          /* OW_MT */

	while (Inbound_Control.w1_file_descriptor > -1 ) {
		if ( W1Select_no_timeout() == 0 ) {
			struct netlink_parse nlp ;
			if ( Netlink_Parse_Get( &nlp ) == 0 ) {
				Dispatch_Packet( &nlp ) ;
				Netlink_Parse_Destroy(&nlp) ;
			}
		}
	}
	return 0;
}

#endif /* OW_W1 */
