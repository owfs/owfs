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

static void w1_masters(struct netlink_parse * nlp)
{
	if (nlp->w1m) {
		int bus_master = nlp->w1m->id.mst.id ;
		switch (nlp->w1m->type) {
			case W1_MASTER_ADD:
				AddW1Bus(bus_master) ;
				//printf("Master has been added: w1_master%d.\n",	bus_master);
					break;
			case W1_MASTER_REMOVE:
				RemoveW1Bus(bus_master) ;
				//printf("Master has been removed: w1_master%d.\n", bus_master);
					break;
			default:
				//printf("Other command (Not master)\n");
				break ;
		}
	}
}

int W1NLScan( void )
{
	while (1)
	{
		int select_value ;
		
		fd_set readset ;
		FD_ZERO(&readset) ;
		FD_SET(Inbound_Control.nl_file_descriptor,&readset) ;
		
		select_value = select(Inbound_Control.nl_file_descriptor+1,&readset,NULL,NULL,NULL) ;
		
		if ( select_value == -1 ) {
			ERROR_DEBUG("Select returned -1\n");
			if (errno == EINTR) {
				continue ;
			}
		} else if ( select_value == 0 ) {
			LEVEL_DEBUG("Select returned zero\n");
		} else {
			struct netlink_parse nlp ;
			
			switch ( Netlink_Parse_Get( &nlp ) ) {
				case -EAGAIN:
					break ;
				case 0:
					w1_masters( &nlp );
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
