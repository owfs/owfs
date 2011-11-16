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

#if OW_W1 && OW_MT 

#include "ow_w1.h"
#include "ow_connection.h"

void * w1_master_command(void * v)
{
	struct netlink_parse * nlp = v ;
	// This is a structure that was allocated in Dispatch_Packet()
	// It needs to be destroyed before this (detached) thread is finished.
	
	DETACH_THREAD;

	if ( nlp->nlm->nlmsg_pid != 0 ) {
		LEVEL_DEBUG("Netlink packet PID not from kernel");
	} else if (nlp->w1m) {
		int bus_master = nlp->w1m->id.mst.id ;
		switch (nlp->w1m->type) {
			case W1_LIST_MASTERS:
				w1_parse_master_list( nlp );
				break;
			case W1_MASTER_ADD:
				AddW1Bus(bus_master) ;
				break;
			case W1_MASTER_REMOVE:
				RemoveW1Bus(bus_master) ;
				break;
			case W1_SLAVE_ADD:
			case W1_SLAVE_REMOVE:
				// ignored since there is no bus attribution and real use will discover the change.
				// at some point we could make the w1 master lists static and only changed when triggered by one of these messages.
				LEVEL_DEBUG("Netlink (w1) Slave announcements");
				break ;
			default:
				LEVEL_DEBUG("Netlink (w1) Other command.");
				break ;
		}
	}
	
	SAFEFREE(nlp->nlm) ;
	owfree(nlp) ;
	
	return VOID_RETURN ;
}

#endif /* OW_W1 && OW_MT */
