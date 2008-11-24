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

#if OW_W1
#include "ow_w1.h"

int w1_bind( void )
{
	struct sockaddr_nl l_local;
	
	Outbound_Control.nl_file_descriptor = socket(AF_NETLINK, SOCK_DGRAM, NETLINK_CONNECTOR);
	if (nl_file_descriptor == -1) {
		perror("socket");
		return -1;
	}
	
	l_local.nl_family = AF_NETLINK;
	l_local.nl_groups = 23;
	l_local.nl_pid    = getpid();
	
	if (bind(Outbound_Control.nl_file_descriptor, (struct sockaddr *)&l_local, sizeof(struct sockaddr_nl)) == -1) {
		perror("bind");
		close(Outbound_Control.nl_file_descriptor);
		Outbound_Control.nl_file_descriptor = -1 ;
		return -1;
	}
	
	return 0 ;
}

#endif /* OW_W1 */
