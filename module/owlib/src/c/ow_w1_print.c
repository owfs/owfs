/*
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include <config.h>
#include "owfs_config.h"

#if OW_W1
#include "ow_w1.h"
#include "ow_connection.h"

#define printcase( flag ) case flag: printf(#flag); break

static void NLM_print( struct nlmsghdr * nlm )
{
	if ( nlm == NULL ) {
		printf("NLMSGHDR: NULL nlm field\n") ;
		return ;
	}
	printf("NLMSGHDR: len=%u type=%u (",nlm->nlmsg_len, nlm->nlmsg_type) ;
	switch ( nlm->nlmsg_type )
	{
		printcase( NLMSG_NOOP );
		printcase( NLMSG_ERROR );
		printcase( NLMSG_DONE );
		printcase( NLMSG_OVERRUN );
		default:
			printf("NLMSG unknown message type %d",nlm->nlmsg_type);
			break ;
	}
	printf(") flags=%u seq=%u|%u pid=%u\n",nlm->nlmsg_flags, NL_BUS(nlm->nlmsg_seq), NL_SEQ(nlm->nlmsg_seq), nlm->nlmsg_pid ) ;
}

static void CN_print( struct cn_msg * cn )
{
	if ( cn == NULL ) {
		printf("CN_MSG: NULL cn field\n") ;
		return ;
	}
	printf("CN_MSG: idx/val=%u/%u (",cn->id.idx,cn->id.val) ;
	switch(cn->id.idx)
	{
		printcase( CN_IDX_PROC );
		printcase( CN_IDX_CIFS );
		printcase( CN_W1_IDX );
		printcase( CN_IDX_V86D );
		default:
			printf("CN unknown message type %d",cn->id.idx);
			break ;
	}
	printf(") seq=%u|%u ack=%u len=%u flags=%u\n",NL_BUS(cn->seq),NL_SEQ(cn->seq),cn->ack,cn->len,cn->flags) ;
}

static void W1M_print( struct w1_netlink_msg * w1m )
{
	if ( w1m == NULL ) {
		printf("W1_NETLINK_MSG: NULL w1m field\n") ;
		return ;
	}
	printf("W1_NETLINK_MSG: type=%u (",w1m->type) ;
	switch(w1m->type)
	{
		printcase( W1_SLAVE_ADD );
		printcase( W1_SLAVE_REMOVE );
		printcase( W1_MASTER_ADD );
		printcase( W1_MASTER_REMOVE );
		printcase( W1_MASTER_CMD );
		printcase( W1_SLAVE_CMD );
		printcase( W1_LIST_MASTERS );
		default:
			printf("W1_NETLINK_MSG unknown message type %d",w1m->type);
			break ;
	}
	printf(") len=%u id=%u\n",w1m->len, w1m->id.mst.id ) ;
}

static void W1C_print( struct w1_netlink_cmd * w1c )
{
	if ( w1c == NULL ) {
		printf("W1_NETLINK_CMD: NULL w1c field\n") ;
		return ;
	}
	printf("W1_NETLINK_CMD: cmd=%u (",w1c->cmd ) ;
	switch(w1c->cmd)
	{
		printcase( W1_CMD_READ );
		printcase( W1_CMD_WRITE );
		printcase( W1_CMD_SEARCH );
		printcase( W1_CMD_ALARM_SEARCH );
		printcase( W1_CMD_TOUCH );
		printcase( W1_CMD_RESET );
		default:
			printf("W1_NETLINK_CMD unknown message type %d",w1c->cmd);
			break ;
	}
	printf(") len=%u\n", w1c->len) ;
}

void Netlink_Print( struct nlmsghdr * nlm, struct cn_msg * cn, struct w1_netlink_msg * w1m, struct w1_netlink_cmd * w1c, unsigned char * data, int length )
{
	if ( Globals.error_level>=e_err_call ) {
		NLM_print( nlm ) ;
		CN_print( cn ) ;
		W1M_print( w1m ) ;
		W1C_print( w1c ) ;
		if ( data != NULL ) {
			_Debug_Bytes("Data", data, length) ;
		} else {
			printf("NULL data\n");
		}
	}
}

#endif /* OW_W1 */
