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

int w1_bind( void )
{
	struct sockaddr_nl l_local ;

	Inbound_Control.nl_file_descriptor = socket(AF_NETLINK, SOCK_DGRAM, NETLINK_CONNECTOR);
	if (Inbound_Control.nl_file_descriptor == -1) {
		ERROR_CONNECT("Netlink (w1) socket");
		return -1;
	}

	Inbound_Control.w1_pid = getpid() ;
	l_local.nl_family = AF_NETLINK;
	l_local.nl_groups = 23;
	l_local.nl_pid    = Inbound_Control.w1_pid;
	
	if (bind(Inbound_Control.nl_file_descriptor, (struct sockaddr *)&l_local, sizeof(struct sockaddr_nl)) == -1) {
		ERROR_CONNECT("Netlink (w1) bind");
		w1_unbind() ;
		return -1;
	}
	
	return Inbound_Control.nl_file_descriptor ;
}

void w1_unbind( void )
{
	if ( Inbound_Control.nl_file_descriptor > -1 ) {
		close(Inbound_Control.nl_file_descriptor);
		Inbound_Control.nl_file_descriptor = -1 ;
	}
}

#endif /* OW_W1 */
