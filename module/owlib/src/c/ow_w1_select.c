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

/* Wait for a netlink packet */
GOOD_OR_BAD W1PipeSelect_timeout( FILE_DESCRIPTOR_OR_ERROR file_descriptor )
{
	do {
		int select_value ;
		struct timeval tv = { Globals.timeout_w1, 0 } ;
		
		fd_set readset ;
		FD_ZERO(&readset) ;
		FD_SET(file_descriptor,&readset) ;
		select_value = select(file_descriptor+1,&readset,NULL,NULL,&tv) ;
		
		if ( select_value == -1 ) {
			if (errno != EINTR) {
				ERROR_CONNECT("Netlink (w1) Select error");
				return gbBAD ;
			}
		} else if ( select_value == 0 ) {
			struct timeval now ;
			struct timeval diff ;
			gettimeofday(&now,NULL);
			// Set time of last read
			_MUTEX_LOCK(Inbound_Control.w1_monitor->master.w1_monitor.read_mutex) ;
			timersub( &now, &(Inbound_Control.w1_monitor->master.w1_monitor.last_read), &diff );
			_MUTEX_UNLOCK(Inbound_Control.w1_monitor->master.w1_monitor.read_mutex) ;
			if ( diff.tv_sec <= Globals.timeout_w1 ) {
				LEVEL_DEBUG("Select legal timeout -- try again");
				continue ;
			}
			LEVEL_DEBUG("Select returned zero (timeout)");
			return gbBAD;
		} else {
			return gbGOOD ;
		}
	} while (1) ;
}

#endif /* OW_W1 && OW_MT */
