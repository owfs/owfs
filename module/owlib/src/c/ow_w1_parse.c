/*
$Id$
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#include <config.h>
#include "owfs_config.h"

#if OW_W1 && OW_MT

#include "ow_w1.h"
#include "ow_connection.h"

static void Netlink_Parse_Show( struct netlink_parse * nlp ) ;
static GOOD_OR_BAD Netlink_Parse_Buffer( unsigned char * buffer, struct netlink_parse * nlp ) ;

void Netlink_Parse_Destroy( struct netlink_parse * nlp )
{
	if ( nlp == NULL ) {
		return ;
	}
	SAFEFREE(nlp->nlm ) ;
}

static GOOD_OR_BAD Netlink_Parse_Buffer( unsigned char * buffer, struct netlink_parse * nlp )
{
	/* NLM Netlink header */
	nlp->nlm = (struct nlmsghdr *)       &buffer[0] ;

	// test pid
	if ( nlp->nlm->nlmsg_pid != 0 ) {
		LEVEL_DEBUG("Netlink (w1) Not from kernel");
		// non-peek
		return gbBAD ;
	}

	// test type
	if ( nlp->nlm->nlmsg_type != NLMSG_DONE ) {
		LEVEL_DEBUG("Netlink (w1) Bad message type");
		return gbBAD ;
	}

	// test length
	if ( nlp->nlm->nlmsg_len < W1_NLM_LENGTH + W1_CN_LENGTH + W1_W1M_LENGTH ) {
		LEVEL_DEBUG("Netlink (w1) Bad message length (%d)",nlp->nlm->nlmsg_len);
		return gbBAD ;
	}

	/* CONNECTOOR MESSAGE */
	nlp->cn  = (struct cn_msg *)         &buffer[W1_NLM_LENGTH] ;

	// sequence numbers
	if ( nlp->nlm->nlmsg_seq != nlp->cn->seq ) {
		LEVEL_DEBUG("Netlink (w1) sequence numbers internally inconsitent");
		return gbBAD ;
	}

	/* W1_NETLINK_MESSAGE */
	nlp->w1m = (struct w1_netlink_msg *) &buffer[W1_NLM_LENGTH + W1_CN_LENGTH] ;

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
			nlp->w1c = (struct w1_netlink_cmd *) &buffer[W1_NLM_LENGTH + W1_CN_LENGTH + W1_W1M_LENGTH] ;
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
	unsigned char * buffer ;
	struct nlmsghdr peek_nlm ;

	// first peek at message to get length and details
	LEVEL_DEBUG("Wait to peek at message");
	int recv_len = recv(Inbound_Control.w1_file_descriptor, &peek_nlm, W1_NLM_LENGTH, MSG_PEEK );

	// Set time of last read
	_MUTEX_LOCK(Inbound_Control.w1_read_mutex) ;
	gettimeofday(&Inbound_Control.w1_last_read,NULL);
	_MUTEX_UNLOCK(Inbound_Control.w1_read_mutex) ;

	LEVEL_DEBUG("Pre-parse header: %u bytes len=%u type=%u seq=%u|%u pid=%u",recv_len,peek_nlm.nlmsg_len,peek_nlm.nlmsg_type,NL_BUS(peek_nlm.nlmsg_seq),NL_SEQ(peek_nlm.nlmsg_seq),peek_nlm.nlmsg_pid);
	if (recv_len < 0) {
		ERROR_DEBUG("Netlink (w1) recv header error");
		return gbBAD ;
	}

	// allocate space
	buffer = owmalloc( peek_nlm.nlmsg_len ) ;
	if ( buffer == NULL ) {
		LEVEL_DEBUG("Netlink (w1) Cannot allocate %d byte buffer for data",peek_nlm.nlmsg_len) ;
		return gbBAD ;
	}

	// read whole packet
	recv_len = recv(Inbound_Control.w1_file_descriptor, buffer, peek_nlm.nlmsg_len, 0 );
	if (recv_len == -1) {
		ERROR_DEBUG("Netlink (w1) recv body error");
		owfree(buffer);
		return gbBAD ;
	}

	if ( GOOD( Netlink_Parse_Buffer( buffer, nlp )) ) {
		LEVEL_DEBUG("Netlink read -----------------");
		Netlink_Parse_Show( nlp ) ;
		return gbGOOD ;
	}
	owfree( buffer ) ;
	return gbBAD ;
}

/* Reads a packet from a pipe that was originally a netlink packet */
GOOD_OR_BAD Get_and_Parse_Pipe( FILE_DESCRIPTOR_OR_ERROR file_descriptor, struct netlink_parse * nlp )
{
	unsigned char * buffer ;
	struct nlmsghdr peek_nlm ;

	// first read start of message to get length and details
	if ( read( file_descriptor, &peek_nlm, W1_NLM_LENGTH ) != W1_NLM_LENGTH ) {
		ERROR_DEBUG("Pipe (w1) read header error");
		return gbBAD ;
	}

	LEVEL_DEBUG("Pipe header: len=%u type=%u seq=%u|%u pid=%u ",peek_nlm.nlmsg_len,peek_nlm.nlmsg_type,NL_BUS(peek_nlm.nlmsg_seq),NL_SEQ(peek_nlm.nlmsg_seq),peek_nlm.nlmsg_pid);

	// allocate space
	buffer = owmalloc( peek_nlm.nlmsg_len ) ;
	if ( buffer == NULL ) {
		LEVEL_DEBUG("Netlink (w1) Cannot allocate %d byte buffer for data",peek_nlm.nlmsg_len) ;
		return gbBAD ;
	}

	memcpy( buffer, &peek_nlm, W1_NLM_LENGTH ) ;
	// read rest of packet
	if ( read( file_descriptor, &buffer[W1_NLM_LENGTH], peek_nlm.nlmsg_len - W1_NLM_LENGTH ) != (ssize_t) peek_nlm.nlmsg_len - W1_NLM_LENGTH ) {
		ERROR_DEBUG("Pipe (w1) read body error");
		owfree(buffer) ;
		return gbBAD ;
	}

	if ( BAD( Netlink_Parse_Buffer( buffer, nlp )) ) {
		LEVEL_DEBUG("Buffer parsing error");
		owfree(buffer) ;
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
	FILE_DESCRIPTOR_OR_ERROR file_descriptor ;
	int bus ;

	if ( seq == SEQ_BAD ) {
		return nrs_bad_send ;
	}

	if ( pn == NULL ) {
		// Send to main netlink rather than a particular bus
		file_descriptor = FILE_DESCRIPTOR_BAD ;
		bus = 0 ;
	} else {
		// Bus-specifc
		file_descriptor = pn->selected_connection->connin.w1.netlink_pipe[fd_pipe_read] ;
		bus = pn->selected_connection->connin.w1.id ;
	}

	while ( GOOD( W1PipeSelect_timeout(file_descriptor)) ) {
		struct netlink_parse nlp ;
		LEVEL_DEBUG("Loop waiting for netlink piped message");
		if ( BAD( Get_and_Parse_Pipe( file_descriptor, &nlp )) ) {
			LEVEL_DEBUG("Error reading pipe for w1_bus_master%d",bus);
			return nrs_error ;
		}
		if ( NL_SEQ(nlp.nlm->nlmsg_seq) != (unsigned int) seq ) {
			LEVEL_DEBUG("Netlink sequence number out of order");
			owfree(nlp.nlm) ;
			continue ;
		}
		if ( nlp.w1m->status != 0) {
			owfree(nlp.nlm) ;
			return nrs_nodev ;
		}
		if ( nrs_callback == NULL ) { // status message
			owfree(nlp.nlm) ;
			return nrs_complete ;
		}

		LEVEL_DEBUG("About to call nrs_callback");
		nrs_callback( &nlp, v, pn ) ;
		LEVEL_DEBUG("Called nrs_callback");
		owfree(nlp.nlm) ;
		if ( nlp.cn->ack != 0 ) {
			if ( nlp.w1m->type == W1_LIST_MASTERS ) {
				continue ; // look for more data
			}
			if ( nlp.w1c && (nlp.w1c->cmd==W1_CMD_SEARCH || nlp.w1c->cmd==W1_CMD_ALARM_SEARCH) ) {
				continue ; // look for more data
			}
		}
		nrs_callback = NULL ; // now look for status message
	}
	return nrs_timeout ;
}

#endif /* OW_W1 && OW_MT */
