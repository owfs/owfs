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

static int send_cmd( struct w1_netlink_msg *msg)
{
	struct cn_msg *cmsg;
	struct w1_netlink_msg *m;
	struct nlmsghdr *nlh;
	int seq = Inbound_Control.w1_seq ++ ;
	
	int size, err;
	
	size = sizeof(struct nlmsghdr) + sizeof(struct cn_msg) + sizeof(struct w1_netlink_msg) + msg->len;
	
	nlh = malloc(size);
	if (!nlh)
		return -ENOMEM;
	
	memset(nlh, 0, size);
	
	nlh->nlmsg_seq = seq;
	nlh->nlmsg_type = NLMSG_DONE;
	nlh->nlmsg_len = NLMSG_LENGTH(sizeof(struct cn_msg) + sizeof(struct w1_netlink_msg) + msg->len);
	nlh->nlmsg_flags = NLM_F_REQUEST;
	nlh->nlmsg_pid = getpid() ;
	
	cmsg = NLMSG_DATA(nlh);
	
	cmsg->id.idx = CN_W1_IDX;
	cmsg->id.val = CN_W1_VAL;
	cmsg->seq = seq;
	cmsg->ack = 1;
	cmsg->flags = 0 ;
	cmsg->len = sizeof(struct w1_netlink_msg) + msg->len;
	
	m = (struct w1_netlink_msg *)(cmsg + 1);
	memcpy(m, msg, sizeof(struct w1_netlink_msg));
	memcpy(m+1, msg->data, msg->len);

	err = send(Inbound_Control.nl_file_descriptor, nlh, size,  0);
	if (err == -1) {
		ERROR_CONNECT("Failed to send W1_LIST_MASTERS\n");
	}
	free(nlh);
	
	return seq;
}

static int w1_list_masters( void )
{
	struct w1_netlink_msg w1lm;
	
	memset(&w1lm, 0, sizeof(w1lm));
	
	w1lm.type = W1_LIST_MASTERS;
	w1lm.len = 0;
	w1lm.id.mst.id = 0;
	
	return send_cmd(&w1lm);
}

static void w1_parse_master_list(struct netlink_parse * nlp)
{
	int * bus_master = (int *) nlp->data ;
	int i ;

	for ( i=0  ;i<nlp->data_size/4 ; ++i ) {
		AddW1Bus( bus_master[i] ) ;
	}
}

static void w1_masters(struct netlink_parse * nlp)
{
	switch (nlp->w1m->type) {
		case W1_LIST_MASTERS:
			w1_parse_master_list( nlp );
				break;
		default:
			LEVEL_DEBUG("Other command (Not master list)\n");
			break ;
	}
}

int W1NLList( void )
{
	int repeat_loop = 1 ;
	int seq ;

	if ( Inbound_Control.nl_file_descriptor == -1 && w1_bind() == -1 ) {
		ERROR_DEBUG("Netlink problem -- are you root?\n");
		return -1 ;
	}
	
	seq = w1_list_masters() ;
	if  (seq < 0 ) {
		LEVEL_CONNECT("Couldn't send the W1_LIST_MASTERS request\n");
		return seq ;
	}

	while (repeat_loop)
	{
		int select_value ;
		struct timeval tv = { 5, 0 } ;
		
		fd_set readset ;
		FD_ZERO(&readset) ;
		FD_SET(Inbound_Control.nl_file_descriptor,&readset) ;
		
		select_value = select(Inbound_Control.nl_file_descriptor+1,&readset,NULL,NULL,&tv) ;
		
		if ( select_value == -1 ) {
			if (errno != EINTR) {
				ERROR_CONNECT("Netlink (w1) Select returned -1\n");
				return 1 ;
			}
		} else if ( select_value == 0 ) {
			LEVEL_DEBUG("Select returned zero (timeout)\n");
			return 1;
		} else {
			struct netlink_parse nlp ;

			switch ( Netlink_Parse_Get( &nlp ) ) {
				case -EAGAIN:
					break ;
				case 0:
					if ( nlp.nlm->nlmsg_seq != seq || nlp.w1m->type != W1_LIST_MASTERS ) {
						LEVEL_DEBUG("Non-matching netlink packet\n");
						continue ;
					}
					w1_masters( &nlp );
					repeat_loop = nlp.cn->ack ;
					Netlink_Parse_Destroy(&nlp) ;
					break ;
				default:
					return  1;
			}
		}
	}
	return 0;
}

#endif /* OW_W1 */
