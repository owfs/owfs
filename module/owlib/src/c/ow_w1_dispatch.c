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

static void Dispatch_Packet( struct netlink_parse * nlp)
{
    int bus = NL_BUS(nlp->nlm->nlmsg_seq) ;
    struct connection_in * in ;

    if ( bus == 0 ) {
        write(Inbound_Control.w1_write_descriptor, (const void *)nlp->nlm, nlp->nlm->nlmsg_len) ;
        return ;
    }


    CONNIN_RLOCK ;
    for ( in = Inbound_Control.head ; in != NULL ; in = in->next ) {
        //printf("Matching %d/%s/%s/%s/ to bus.%d %d/%s/%s/%s/\n",bus_zero,name,type,domain,now->index,now->busmode,now->connin.tcp.name,now->connin.tcp.type,now->connin.tcp.domain);
        if ( in->busmode == bus_w1 && in->connin.w1.id == bus ) {
            break ;
        }
    }
    if ( in == NULL ) {
        LEVEL_DEBUG("Netlink message sent to non-existent w1_bus_master%d\n",bus) ;
    } else {
        /* We could have select and a timeout for safety, but this is internal communication */
        write(in->connin.w1.write_file_descriptor, (const void *)nlp->nlm, nlp->nlm->nlmsg_len) ;
    }
    CONNIN_RUNLOCK ;
}

void * W1_Dispatch( void * v )
{
    (void) v;

#if OW_MT
    pthread_detach(pthread_self());
#endif                          /* OW_MT */

    while (Inbound_Control.w1_file_descriptor > -1 )
	{
		if ( W1Select() == 0 ) {
			struct netlink_parse nlp ;
			if ( Netlink_Parse_Get( &nlp ) == 0 ) {
                Dispatch_Packet( &nlp ) ;
                Netlink_Parse_Destroy(&nlp) ;
			}
		}
	}
	return 0;
}

#endif /* OW_W1 */
