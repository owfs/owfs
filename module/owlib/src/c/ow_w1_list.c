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

static int w1_list_masters( void )
{
	struct w1_netlink_msg w1m;

	memset(&w1m, 0, W1_W1M_LENGTH);
	w1m.type = W1_LIST_MASTERS;
	w1m.len = 0;
	w1m.id.mst.id = 0;

	return W1_send_msg( NULL, &w1m, NULL, NULL );
}

static void w1_parse_master_list(struct netlink_parse * nlp)
{
	int * bus_master = (int *) nlp->data ;
	int num_masters = nlp->data_size / 4 ;
	int i ;

	
	for ( i=0  ;i<num_masters ; ++i ) {
		AddW1Bus( bus_master[i] ) ;
	}
}

static void w1_masters(struct netlink_parse * nlp, void * v, const struct parsedname * pn)
{
	(void) pn ;
	(void) v ;
	switch (nlp->w1m->type) {
		case W1_LIST_MASTERS:
			w1_parse_master_list( nlp );
				break;
		default:
			LEVEL_DEBUG("Other command (Not master list)\n");
			break ;
	}
}

enum Netlink_Read_Status W1NLList( void )
{
	return W1_Process_Response( w1_masters, w1_list_masters(), NULL, NULL ) ;
}

#endif /* OW_W1 */
