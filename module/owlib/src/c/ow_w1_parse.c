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
	int recv_len = recv(Inbound_Control.w1_file_descriptor, &peek_nlm, W1_NLM_LENGTH, MSG_PEEK );
	
	LEVEL_DEBUG("Pre-parse header: %u bytes len=%u type=%u seq=%u|%u pid=%u\n",recv_len,peek_nlm.nlmsg_len,peek_nlm.nlmsg_type,NL_BUS(peek_nlm.nlmsg_seq),NL_SEQ(peek_nlm.nlmsg_seq),peek_nlm.nlmsg_pid);
	if (recv_len == -1) {
		ERROR_DEBUG("Netlink (w1) recv header error\n");
		return -errno ;
	}
	
	// test pid
	if ( peek_nlm.nlmsg_pid != 0 ) {
		LEVEL_DEBUG("Netlink (w1) Not our pid on message\n");
		// non-peek
		recv(Inbound_Control.w1_file_descriptor, &peek_nlm, W1_NLM_LENGTH, 0 );
		return -EAGAIN ;
	}
	
	// test type
	if ( peek_nlm.nlmsg_type != NLMSG_DONE ) {
		LEVEL_DEBUG("Netlink (w1) Bad message type\n");
		// non-peek
		recv(Inbound_Control.w1_file_descriptor, &peek_nlm, W1_NLM_LENGTH, 0 );
		return -EINVAL ;
	}
	
	// test length
	if ( peek_nlm.nlmsg_len < W1_NLM_LENGTH + W1_CN_LENGTH + W1_W1M_LENGTH ) {
		LEVEL_DEBUG("Netlink (w1) Bad message length (%d)\n",peek_nlm.nlmsg_len);
		// non-peek
		recv(Inbound_Control.w1_file_descriptor, &peek_nlm, W1_NLM_LENGTH, 0 );
		return -EMSGSIZE ;
	}
	
	// allocate space
	buffer = malloc( peek_nlm.nlmsg_len ) ;
	if ( buffer == NULL ) {
		LEVEL_DEBUG("Netlink (w1) Cannot allocate %d byte buffer for data\n",peek_nlm.nlmsg_len) ;
		return -ENOMEM ;
	}
	
	// read whole packet
	recv_len = recv(Inbound_Control.w1_file_descriptor, buffer, peek_nlm.nlmsg_len, 0 );
	if (recv_len == -1) {
		ERROR_DEBUG("Netlink (w1) recv body error\n");
		free(buffer);
		return -EIO ;
	}
	
	nlp->nlm = (struct nlmsghdr *)       &buffer[0] ;
	nlp->cn  = (struct cn_msg *)         &buffer[W1_NLM_LENGTH] ;
	nlp->w1m = (struct w1_netlink_msg *) &buffer[W1_NLM_LENGTH + W1_CN_LENGTH] ;
	nlp->w1c = (struct w1_netlink_cmd *) &buffer[W1_NLM_LENGTH + W1_CN_LENGTH + W1_W1M_LENGTH] ;
	
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
	LEVEL_DEBUG("Netlink read -----------------\n");
	Netlink_Parse_Show( nlp ) ;
	return 0 ;
}

/* Reads a packet from a pipe that was originally a netlink packet */
int Get_and_Parse_Pipe( int file_descriptor, struct netlink_parse * nlp )
{
	unsigned char * buffer ;
	struct nlmsghdr peek_nlm ;
	struct timeval tv = { Globals.timeout_w1, 0 } ;
	
	// first read start of message to get length and details
	if ( tcp_read( file_descriptor, &peek_nlm, W1_NLM_LENGTH, &tv ) != W1_NLM_LENGTH ) {
		LEVEL_DEBUG("Pipe (w1) read header error\n");
		return -1 ;
	}

	LEVEL_DEBUG("Pipe header: len=%u type=%u seq=%u|%u pid=%u \n",peek_nlm.nlmsg_len,peek_nlm.nlmsg_type,NL_BUS(peek_nlm.nlmsg_seq),NL_SEQ(peek_nlm.nlmsg_seq),peek_nlm.nlmsg_pid);
	
	// allocate space
	buffer = malloc( peek_nlm.nlmsg_len ) ;
	if ( buffer == NULL ) {
		LEVEL_DEBUG("Netlink (w1) Cannot allocate %d byte buffer for data\n",peek_nlm.nlmsg_len) ;
		return -ENOMEM ;
	}
	
	memcpy( buffer, &peek_nlm, W1_NLM_LENGTH ) ;
	// read rest of packet
	if ( tcp_read( file_descriptor, &buffer[W1_NLM_LENGTH], peek_nlm.nlmsg_len - W1_NLM_LENGTH, &tv ) != peek_nlm.nlmsg_len - W1_NLM_LENGTH ) {
		LEVEL_DEBUG("Pipe (w1) read body error\n");
		return -1 ;
	}
	
	nlp->nlm = (struct nlmsghdr *)       &buffer[0] ;
	nlp->cn  = (struct cn_msg *)         &buffer[W1_NLM_LENGTH] ;
	nlp->w1m = (struct w1_netlink_msg *) &buffer[W1_NLM_LENGTH + W1_CN_LENGTH] ;
	nlp->w1c = (struct w1_netlink_cmd *) &buffer[W1_NLM_LENGTH + W1_CN_LENGTH + W1_W1M_LENGTH] ;
	
	/* All piped packets have w1_netlink_cmd segments */
	nlp->data = nlp->w1c->data ;
	nlp->data_size = nlp->w1c->len ;

	if ( nlp->data_size == 0 ) {
		nlp->data = NULL ;
	}
	LEVEL_DEBUG("Pipe read --------------------\n");
	Netlink_Parse_Show( nlp ) ;
	return 0 ;
}

static void Netlink_Parse_Show( struct netlink_parse * nlp )
{
	Netlink_Print( nlp->nlm, nlp->cn, nlp->w1m, nlp->w1c, nlp->data, nlp->data_size ) ;
}

#endif /* OW_W1 */
