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

/* If cmd is specified, msg length will be adjusted */
/* Build a netlink message, rather tediously, from all the components
 * making the internal flags, length fields and headers be correct */

SEQ_OR_ERROR W1_send_msg( struct connection_in * in, struct w1_netlink_msg *msg, struct w1_netlink_cmd *cmd, const unsigned char * data)
{
	// outer structure -- nlm = netlink message
	struct nlmsghdr *nlm;
	// second structure -- cn = connection message
	struct cn_msg *cn;
	// third structure w1m = w1 message to the bus master
	struct w1_netlink_msg *w1m;
	// optional fourth w1c = w1 command to a device
	struct w1_netlink_cmd *w1c;
	unsigned char * pdata ;
	int data_size ;
	SEQ_OR_ERROR seq ;
	int bus ;
	int nlm_payload;

	// NULL connection for initial LIST_MASTERS, not assigned to a specific bus
	if ( in == NO_CONNECTION ) {
		// w1 root bus (scanner)
		// need to lock master w1 before incrementing
		_MUTEX_LOCK(Inbound_Control.w1_monitor->master.w1_monitor.seq_mutex) ;
		seq = ++Inbound_Control.w1_monitor->master.w1_monitor.seq ;
		_MUTEX_UNLOCK(Inbound_Control.w1_monitor->master.w1_monitor.seq_mutex) ;
		bus = 0 ;
	} else {
		// w1 subsidiary bus
		// this bus is locked
		seq = ++in->master.w1.seq ;
		bus = in->master.w1.id;
	}

	// figure out the full message length and allocate space
	nlm_payload = W1_CN_LENGTH + W1_W1M_LENGTH ; // default length before data
	if ( cmd == NULL ) {
		// no command
		data_size = msg->len ;
	} else {
		data_size = cmd->len ; // command data length
		// add command field
		nlm_payload += W1_W1C_LENGTH ;
	}
	// add data length
	nlm_payload += data_size ;

	nlm = owmalloc( NLMSG_SPACE(nlm_payload) );
	if (nlm==NULL) {
		// memory allocation error
		return SEQ_BAD;
	}

	// set the nlm fields
	memset(nlm, 0, NLMSG_SPACE(nlm_payload) );
	nlm->nlmsg_seq = MAKE_NL_SEQ( bus, seq );
	nlm->nlmsg_type = NLMSG_DONE;
	nlm->nlmsg_len = NLMSG_LENGTH(nlm_payload) ; // full message
	//nlm->nlmsg_flags = NLM_F_REQUEST ; // required for userspace -> kernel
	nlm->nlmsg_flags = NLM_F_REQUEST | NLM_F_ACK ; // respond, please
	nlm->nlmsg_pid = Inbound_Control.w1_monitor->master.w1_monitor.pid ;

	// set the cn fields
	cn = (struct cn_msg *) NLMSG_DATA(nlm) ; // just after nlm field
	cn->id.idx = CN_W1_IDX;
	cn->id.val = CN_W1_VAL;
	cn->seq = nlm->nlmsg_seq;
	cn->ack = cn->seq; // intentionally non-zero ;
	cn->flags = 0 ;
	cn->len = nlm_payload - W1_CN_LENGTH ; // size not including nlm or cn

	// set the w1m (and optionally w1c) fields
	w1m = (struct w1_netlink_msg *)(cn + 1); // just after cn field
	memcpy(w1m, msg, W1_W1M_LENGTH);
	w1m->len = cn->len - W1_W1M_LENGTH ; // size minus nlm, cn and w1m
	if ( cmd == NULL ) {
		// no command
		w1c = NULL ;
		pdata = (unsigned char *)(w1m + 1); // data just after w1m
	} else {
		w1c = (struct w1_netlink_cmd *)(w1m + 1); // just after w1m
		pdata = (unsigned char *)(w1c + 1); // data just after w1c
		memcpy(w1c, cmd, W1_W1C_LENGTH); // set command
	}
	
	if ( data_size > 0 ) {
		memcpy(pdata, data, data_size);
	} else {
		pdata = NULL ; // no data
	}

	LEVEL_DEBUG("Netlink send -----------------");
	Netlink_Print( nlm, cn, w1m, w1c, pdata, data_size ) ;
	
	if ( send( Inbound_Control.w1_monitor->pown->file_descriptor, nlm, NLMSG_SPACE(nlm_payload),  0) == -1 ) {
		//err = COM_write( nlm, nlm_size, Inbound_Control.w1.monitor ) ;
		owfree(nlm);
		ERROR_CONNECT("Failed to send w1 netlink message");
		return SEQ_BAD ;
	}

	owfree(nlm);
	LEVEL_DEBUG("NETLINK sent seq=%d", (int) seq);
	return seq;
}



#endif /* OW_W1 */
