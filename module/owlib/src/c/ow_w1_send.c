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
int W1_send_msg( int bus, struct w1_netlink_msg *msg, struct w1_netlink_cmd *cmd)
{
	struct cn_msg *cn;
	struct w1_netlink_msg *w1m;
	struct w1_netlink_cmd *w1c;
	struct nlmsghdr *nlm;
	unsigned char * data ;
	int length ;
	int seq ;

	pthread_mutex_lock( &Inbound_Control.w1_mutex ) ;
	seq = ++Inbound_Control.w1_seq  ;
	pthread_mutex_unlock( &Inbound_Control.w1_mutex ) ;

	int size, err;

	size = W1_NLM_LENGTH + W1_CN_LENGTH + W1_W1M_LENGTH ;
	if ( cmd != NULL ) {
		length = cmd->len ;
		size += W1_W1C_LENGTH + length;
	} else {
		length = msg->len ;
		size += length;
	}

	nlm = malloc(size);
	if (!nlm) {
		return -ENOMEM;
	}

	memset(nlm, 0, size);
	nlm->nlmsg_seq = MAKE_NL_SEQ( bus, seq );
	nlm->nlmsg_type = NLMSG_DONE;
	nlm->nlmsg_len = size;
	nlm->nlmsg_flags = 0;
	//nlm->nlmsg_flags = NLM_F_REQUEST;
	nlm->nlmsg_pid = Inbound_Control.w1_pid ;

	cn = (struct cn_msg *)(nlm + 1);

	cn->id.idx = CN_W1_IDX;
	cn->id.val = CN_W1_VAL;
	cn->seq = nlm->seq;
	cn->ack = 0;
	cn->flags = 0 ;
	cn->len = size - W1_NLM_LENGTH - W1_CN_LENGTH ;

	w1m = (struct w1_netlink_msg *)(cn + 1);
	memcpy(w1m, msg, W1_W1M_LENGTH);
	w1m->len = cn->len - W1_W1M_LENGTH ;
	if ( cmd != NULL ) {
		w1c = (struct w1_netlink_cmd *)(w1m + 1);
		data = (unsigned char *)(w1c + 1);
		memcpy(w1c, cmd, W1_W1C_LENGTH);
		memcpy(data, cmd->data, length);
	} else {
		w1c = NULL ;
		data = (unsigned char *)(w1m + 1);
		memcpy(data, msg->data, length);
	}
	LEVEL_DEBUG("Netlink send -----------------\n");
	Netlink_Print( nlm, cn, w1m, w1c, data, length ) ;

	err = send(Inbound_Control.w1_file_descriptor, nlm, size,  0);
	if (err == -1) {
		ERROR_CONNECT("Failed to send W1_LIST_MASTERS\n");
	}
	free(nlm);

	return seq;
}



#endif /* OW_W1 */
