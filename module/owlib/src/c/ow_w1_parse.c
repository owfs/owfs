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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include <asm/byteorder.h>
#include <asm/types.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <linux/netlink.h>
#include <linux/rtnetlink.h>

#include "w1_netlink.h"
#include "w1_repeater.h"

static void Netlink_Parse_Show( struct netlink_parse * nlp ) ;

void Netlink_Parse_Destroy( struct netlink_parse * nlp )
{
	if ( nlp && nlp->nlm ) {
		free( nlp->nlm ) ;
		nlp->nlm = NULL ;
	}
}

int Netlink_Parse_Get( struct netlink_parse * nlp )
{
	unsigned char * buffer ;
	struct nlmsghdr peek_nlm ;

	// first peek at message to get length and details
	int recv_len = recv(nl_file_descriptor, &peek_nlm, sizeof(peek_nlm), MSG_PEEK );

	printf("Pre-parse header: %d bytes len=%d type=%d seq=%d pid=%d (our pid=%d)\n",recv_len,peek_nlm.nlmsg_len,peek_nlm.nlmsg_type,peek_nlm.nlmsg_seq,peek_nlm.nlmsg_pid,getpid());
	if (recv_len == -1) {
		perror("recv header error\n");
		return -errno ;
	}
	
	// test pid
	if ( peek_nlm.nlmsg_pid != 0 ) {
		printf("Not our pid on message\n");
		// non-peek
		recv(nl_file_descriptor, &peek_nlm, sizeof(peek_nlm), 0 );
		return -EAGAIN ;
	}
	
	// test type
	if ( peek_nlm.nlmsg_type != NLMSG_DONE ) {
		printf("Bad message type\n");
		// non-peek
		recv(nl_file_descriptor, &peek_nlm, sizeof(peek_nlm), 0 );
		return -EINVAL ;
	}
	
	// test length
	if ( peek_nlm.nlmsg_len < sizeof(struct nlmsghdr) + sizeof(struct cn_msg) + sizeof(struct w1_netlink_msg) ) {
		printf("Bad message length (%d)\n",peek_nlm.nlmsg_len);
		// non-peek
		recv(nl_file_descriptor, &peek_nlm, sizeof(peek_nlm), 0 );
		return -EMSGSIZE ;
	}
	
	// allocate space
	buffer = malloc( peek_nlm.nlmsg_len ) ;
	if ( buffer == NULL ) {
		printf("Cannot allocate %d byte buffer for netlink data\n",peek_nlm.nlmsg_len) ;
		return -ENOMEM ;
	}

	// read whole packet
	recv_len = recv(nl_file_descriptor, buffer, peek_nlm.nlmsg_len, 0 );
	if (recv_len == -1) {
		perror("recv body error\n");
		free(buffer);
		return -EIO ;
	}

	nlp->nlm = (struct nlmsghdr *)       &buffer[0] ;
	nlp->cn  = (struct cn_msg *)         &buffer[sizeof(struct nlmsghdr)] ;
	nlp->w1m = (struct w1_netlink_msg *) &buffer[sizeof(struct nlmsghdr) + sizeof(struct cn_msg)] ;
	nlp->w1c = (struct w1_netlink_cmd *) &buffer[sizeof(struct nlmsghdr) + sizeof(struct cn_msg) + sizeof(struct w1_netlink_msg)] ;

	switch (nlp->w1m->type) {
		case W1_SLAVE_ADD:
		case W1_SLAVE_REMOVE:
		case W1_MASTER_ADD:
		case W1_MASTER_REMOVE:
		case W1_LIST_MASTERS:
			nlp->w1c = NULL ;
			nlp->data = nlp->w1m->data ;
			nlp->data_size = nlp->w1m->len ;
			break ;
		case W1_MASTER_CMD:
		case W1_SLAVE_CMD:
			nlp->data = nlp->w1c->data ;
			nlp->data_size = nlp->w1c->len ;
	}
	if ( nlp->data_size == 0 ) {
		nlp->data = NULL ;
	}
	Netlink_Parse_Show( nlp ) ;
	return 0 ;
}

static void Netlink_Parse_Show( struct netlink_parse * nlp )
{
	if ( nlp->nlm ) {
		printf("NLMSGHDR: len=%u type=%u flags=%u seq=%u pid=%u\n",nlp->nlm->nlmsg_len, nlp->nlm->nlmsg_type, nlp->nlm->nlmsg_flags, nlp->nlm->nlmsg_seq, nlp->nlm->nlmsg_pid ) ;
	}
	if ( nlp->cn ) {
		printf("CN_MSG: idx/val=%u/%u seq=%u ack=%u len=%u flags=%u\n",nlp->cn->id.idx,nlp->cn->id.val,nlp->cn->seq,nlp->cn->ack,nlp->cn->len,nlp->cn->flags) ;
	}
	if ( nlp->w1m ) {
		printf("W1_NETLINK_MSG: type=%u len=%u id=%u\n",nlp->w1m->type, nlp->w1m->len, nlp->w1m->id.mst.id ) ;
	}
	if ( nlp->w1c ) {
		printf("W1_NETLINK_CMD: cmd=%u len=%u\n",nlp->w1c->cmd, nlp->w1c->len) ;
	}
	if ( nlp->data_size ) {
		int i ;
		printf("DATA:");
		for ( i=0 ; i<nlp->data_size ; ++i ) {
			printf(" %.2X",nlp->data[i]) ;
		}
		printf("\n");
	}
}
