/*
W1 Announce -- daemon  for showing w1 busmasters using Avahi
Written 2008 Paul H Alfille
email: paul.alfille@gmail.com
Released under the GPLv2
Much thanks to Evgeniy Polyakov
This file itself  is a modestly modified version of w1d by Evgeniy Polyakov
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include <config.h>
#include "owfs_config.h"

#if OW_W1

#include "ow_w1.h"
#include "ow_connection.h"

static void Netlink_Parse_Show( struct netlink_parse * nlp ) ;
static GOOD_OR_BAD Get_and_Parse_Pipe( FILE_DESCRIPTOR_OR_ERROR file_descriptor, struct netlink_parse * nlp ) ;

GOOD_OR_BAD Netlink_Parse_Buffer( struct netlink_parse * nlp )
{
	/* NLM Netlink header */
	struct nlmsghdr * nlm = nlp->nlm ;

	// test message
	if ( ! NLMSG_OK(nlm,nlm->nlmsg_len) ) {
		LEVEL_DEBUG("Netlink message truncated");
		// non-peek
		return gbBAD ;
	}

	// test pid
	if ( nlm->nlmsg_pid != 0 ) {
		LEVEL_DEBUG("Netlink (w1) Not from kernel");
		// non-peek
		return gbBAD ;
	}

	// test type
	if ( nlm->nlmsg_type != NLMSG_DONE ) {
		LEVEL_DEBUG("Netlink (w1) Bad message type");
		return gbBAD ;
	}

	// test length
	if ( nlm->nlmsg_len < W1_NLM_LENGTH + W1_CN_LENGTH + W1_W1M_LENGTH ) {
		LEVEL_DEBUG("Netlink (w1) Bad message length (%d)",nlm->nlmsg_len );
		return gbBAD ;
	}

	/* CONNECTOR MESSAGE */
	nlp->cn  = (struct cn_msg *) NLMSG_DATA(nlm) ;

	// sequence numbers
	if ( nlm->nlmsg_seq != nlp->cn->seq ) {
		LEVEL_DEBUG("Netlink (w1) sequence numbers internally inconsistent nlm_seq=%u|%u cn_seq=%u|%u", NL_BUS(nlm->nlmsg_seq), NL_SEQ(nlm->nlmsg_seq),NL_BUS(nlp->cn->seq),NL_SEQ(nlp->cn->seq));
		return gbBAD ;
	}

	/* W1_NETLINK_MESSAGE */
	nlp->w1m = (struct w1_netlink_msg *) nlp->cn->data ;
	//printf("w1m=%p nlm=%p \n" , nlp->w1m, nlm ) ;

	/* W1_NETLINK_COMMAND -- optional depending on w1_netlink_message type */
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
			nlp->w1c = (struct w1_netlink_cmd *) nlp->w1m->data ;
			nlp->data = nlp->w1c->data ;
			nlp->data_size = nlp->w1c->len ;
	}
	if ( nlp->data_size == 0 ) {
		nlp->data = NULL ;
	}
	return gbGOOD ;
}

GOOD_OR_BAD Netlink_Parse_Get( struct netlink_parse * nlp )
{
	struct nlmsghdr peek_nlm ;

	// first peek at message to get length and details
	LEVEL_DEBUG("Wait to peek at message");
	int recv_len = recv( Inbound_Control.w1_monitor->pown->file_descriptor, &peek_nlm, W1_NLM_LENGTH, MSG_PEEK );

	// Set time of last read
	_MUTEX_LOCK(Inbound_Control.w1_monitor->master.w1_monitor.read_mutex) ;
	timernow( &(Inbound_Control.w1_monitor->master.w1_monitor.last_read) );
	_MUTEX_UNLOCK(Inbound_Control.w1_monitor->master.w1_monitor.read_mutex) ;

	LEVEL_DEBUG("Pre-parse header: %u bytes len=%u type=%u seq=%u|%u pid=%u",recv_len,peek_nlm.nlmsg_len,peek_nlm.nlmsg_type,NL_BUS(peek_nlm.nlmsg_seq),NL_SEQ(peek_nlm.nlmsg_seq),peek_nlm.nlmsg_pid);
	if (recv_len < 0) {
		ERROR_DEBUG("Netlink (w1) recv header error");
		return gbBAD ;
	}

	// allocate space
	nlp->nlm = owmalloc( peek_nlm.nlmsg_len ) ;
	if ( nlp->nlm == NULL ) {
		LEVEL_DEBUG("Netlink (w1) Cannot allocate %d byte buffer for data",peek_nlm.nlmsg_len) ;
		return gbBAD ;
	}

	// read whole packet
	recv_len = recv( Inbound_Control.w1_monitor->pown->file_descriptor, &(nlp->nlm[0]), peek_nlm.nlmsg_len, 0 );
	if (recv_len == -1) {
		ERROR_DEBUG("Netlink (w1) recv body error");
		return gbBAD ;
	}

	if ( GOOD( Netlink_Parse_Buffer( nlp )) ) {
		LEVEL_DEBUG("Netlink read -----------------");
		Netlink_Parse_Show( nlp ) ;
		return gbGOOD ;
	}
	return gbBAD ;
}

/* Reads a packet from a pipe that was originally a netlink packet */
/* allocate buffer but free unless able to read and parse nlm fully */
static GOOD_OR_BAD Get_and_Parse_Pipe( FILE_DESCRIPTOR_OR_ERROR file_descriptor, struct netlink_parse * nlp )
{
	struct nlmsghdr peek_nlm ;
	struct nlmsghdr * nlm ;
	size_t payload_length ;

	// first read start of message to get length and details
	if ( read( file_descriptor, &peek_nlm, W1_NLM_LENGTH ) != W1_NLM_LENGTH ) {
		ERROR_DEBUG("Pipe (w1) read header error");
		return gbBAD ;
	}

	LEVEL_DEBUG("Pipe header: len=%u type=%u seq=%u|%u pid=%u ",peek_nlm.nlmsg_len,peek_nlm.nlmsg_type,NL_BUS(peek_nlm.nlmsg_seq),NL_SEQ(peek_nlm.nlmsg_seq),peek_nlm.nlmsg_pid);

	// allocate space
	nlm = owmalloc( peek_nlm.nlmsg_len ) ;
	nlp->nlm = nlm ;
	if ( nlm == NULL ) {
		LEVEL_DEBUG("Netlink (w1) Cannot allocate %d byte buffer for data", peek_nlm.nlmsg_len ) ;
		return gbBAD ;
	}

	memcpy( nlm, &peek_nlm, W1_NLM_LENGTH ) ; // copy header
	// read rest of packet
	if ( peek_nlm.nlmsg_len <= W1_NLM_LENGTH ) {
		owfree(nlm) ;
		LEVEL_DEBUG( "W1 packet error. Length is too short." ) ;
		return gbBAD ;
	}
	payload_length = peek_nlm.nlmsg_len - W1_NLM_LENGTH ;
	if ( read( file_descriptor, NLMSG_DATA(nlm), payload_length ) != (ssize_t) payload_length ) {
		owfree(nlm) ;
		nlp->nlm = NULL ;
		ERROR_DEBUG("Pipe (w1) read payload error");
		return gbBAD ;
	}

	if ( BAD( Netlink_Parse_Buffer( nlp )) ) {
		LEVEL_DEBUG("Buffer parsing error");
		owfree(nlm) ;
		nlp->nlm = NULL ;
		return gbBAD ;
	}

	// Good, the receiving routine should owfree buffer
	LEVEL_DEBUG("Pipe read --------------------");
	Netlink_Parse_Show( nlp ) ;
	return gbGOOD ;
}

static void Netlink_Parse_Show( struct netlink_parse * nlp )
{
	Netlink_Print( nlp->nlm, nlp->cn, nlp->w1m, nlp->w1c, nlp->data, nlp->data_size ) ;
}

enum Netlink_Read_Status W1_Process_Response( void (* nrs_callback)( struct netlink_parse * nlp, void * v, const struct parsedname * pn), SEQ_OR_ERROR seq, void * v, const struct parsedname * pn )
{
	struct connection_in * in = pn->selected_connection ;
	FILE_DESCRIPTOR_OR_ERROR file_descriptor ;
	int bus ;

	if ( seq == SEQ_BAD ) {
		return nrs_bad_send ;
	}

	if ( in == NO_CONNECTION ) {
		// Send to main netlink rather than a particular bus
		file_descriptor = FILE_DESCRIPTOR_BAD ;
		bus = 0 ;
	} else {
		// Bus-specifc
		file_descriptor = in->master.w1.netlink_pipe[fd_pipe_read] ;
		bus = in->master.w1.id ;
	}

	while ( GOOD( W1PipeSelect_timeout(file_descriptor)) ) {
		struct netlink_parse nlp ;
		nlp.nlm = NULL ;
		
		LEVEL_DEBUG("Loop waiting for netlink piped message");
		if ( BAD( Get_and_Parse_Pipe( file_descriptor, &nlp )) ) {
			LEVEL_DEBUG("Error reading pipe for w1_bus_master%d",bus);
			// Don't need to free since nlm not set if BAD
			return nrs_error ;
		}
		if ( NL_SEQ(nlp.nlm->nlmsg_seq) != NL_SEQ(seq) ) {
			LEVEL_DEBUG("Netlink sequence number out of order");
			owfree(nlp.nlm) ;
			continue ;
		}
		if ( nlp.w1m->status != 0) {
			owfree(nlp.nlm) ;
			return nrs_nodev ;
		}
		if ( nrs_callback == NULL ) { // bus reset
			owfree(nlp.nlm) ;
			return nrs_complete ;
		}

		LEVEL_DEBUG("About to call nrs_callback");
		nrs_callback( &nlp, v, pn ) ;
		LEVEL_DEBUG("Called nrs_callback");
		owfree(nlp.nlm) ;
		if ( nlp.cn->seq != nlp.cn->ack ) {
			if ( nlp.w1m->type == W1_LIST_MASTERS ) {
				continue ; // look for more data
			}
			if ( nlp.w1c && (nlp.w1c->cmd==W1_CMD_SEARCH || nlp.w1c->cmd==W1_CMD_ALARM_SEARCH) ) {
				continue ; // look for more data
			}
		}
		return nrs_complete ; // status message
	}
	return nrs_timeout ;
}

#endif /* OW_W1 */
