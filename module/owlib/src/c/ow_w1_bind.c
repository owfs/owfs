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

// Bind the master netlink socket for all communication
// responses will have to be routed to appropriate bus master
GOOD_OR_BAD w1_bind( struct connection_in * in )
{
	struct sockaddr_nl l_local ;

	SOC(in)->type = ct_netlink ;
	Test_and_Close( &(in->head->file_descriptor) ) ; // just in case
	
	// Example from http://lxr.linux.no/linux+v3.0/Documentation/connector/ucon.c#L114
	//in->head->file_descriptor = socket(PF_NETLINK, SOCK_DGRAM, NETLINK_CONNECTOR);
//    in->head->file_descriptor = socket(PF_NETLINK, SOCK_RAW | SOCK_CLOEXEC , NETLINK_CONNECTOR);
    in->head->file_descriptor = socket(PF_NETLINK, SOCK_RAW , NETLINK_CONNECTOR);
	if ( FILE_DESCRIPTOR_NOT_VALID( in->head->file_descriptor ) ) {
		ERROR_CONNECT("Netlink (w1) socket (are you root?)");
		return gbBAD;
	}
//	fcntl (in->head->file_descriptor, F_SETFD, FD_CLOEXEC); // for safe forking

	l_local.nl_pid = in->master.w1_monitor.pid = getpid() ;
	l_local.nl_pad = 0;
	l_local.nl_family = AF_NETLINK;
	l_local.nl_groups = 23;

	if ( bind( in->head->file_descriptor, (struct sockaddr *)&l_local, sizeof(struct sockaddr_nl) ) == -1 ) {
		ERROR_CONNECT("Netlink (w1) bind (are you root?)");
		Test_and_Close( &( in->head->file_descriptor) );
		return gbBAD ;
	}
	in->head->state = cs_deflowered ;
	return gbGOOD ;
}

#endif /* OW_W1 */
