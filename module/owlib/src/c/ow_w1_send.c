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

int W1_send_msg( struct w1_netlink_msg *msg)
{
	struct cn_msg *cn;
	struct w1_netlink_msg *w1m;
	struct nlmsghdr *nlm;
	int seq = Inbound_Control.w1_seq ++ ;
	
	int size, err;
	
	size = sizeof(struct nlmsghdr) + sizeof(struct cn_msg) + sizeof(struct w1_netlink_msg) + msg->len;
	
	nlm = malloc(size);
	if (!nlm)
		return -ENOMEM;
	
	memset(nlm, 0, size);
	
	nlm->nlmsg_seq = seq;
	nlm->nlmsg_type = NLMSG_DONE;
	nlm->nlmsg_len = NLMSG_LENGTH(sizeof(struct cn_msg) + sizeof(struct w1_netlink_msg) + msg->len);
	nlm->nlmsg_flags = NLM_F_REQUEST;
	nlm->nlmsg_pid = Inbound_Control.w1_pid ;
	
	cn = NLMSG_DATA(nlm);
	
	cn->id.idx = CN_W1_IDX;
	cn->id.val = CN_W1_VAL;
	cn->seq = seq;
	cn->ack = 1;
	cn->flags = 0 ;
	cn->len = sizeof(struct w1_netlink_msg) + msg->len;
	
	w1m = (struct w1_netlink_msg *)(cn + 1);
	memcpy(w1m, msg, sizeof(struct w1_netlink_msg));
	memcpy(w1m+1, msg->data, msg->len);

	err = send(Inbound_Control.nl_file_descriptor, nlm, size,  0);
	if (err == -1) {
		ERROR_CONNECT("Failed to send W1_LIST_MASTERS\n");
	}
	free(nlm);
	
	return seq;
}



#endif /* OW_W1 */
