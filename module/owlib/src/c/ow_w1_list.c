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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

/* w1 list
 * Routines to read the list of w1 masters
 * This is called from ow_w1_browse
 */

#include <config.h>
#include "owfs_config.h"

#if OW_W1

#include "ow_w1.h"
#include "ow_connection.h"

SEQ_OR_ERROR w1_list_masters( void )
{
	struct w1_netlink_msg w1m;

	memset(&w1m, 0, W1_W1M_LENGTH);
	w1m.type = W1_LIST_MASTERS;
	w1m.len = 0;
	w1m.id.mst.id = 0;

	LEVEL_DEBUG("Sending w1 bus master list message");
	return W1_send_msg( NULL, &w1m, NULL, NULL );
}

void w1_parse_master_list(struct netlink_parse * nlp)
{
	int * bus_master = (int32_t *) nlp->data ;
	int num_masters = nlp->data_size / 4 ;
	int i ;
	
	LEVEL_DEBUG("W1 List %d masters",num_masters);
	for ( i=0  ;i<num_masters ; ++i ) {
		//printf("W1 master %d of %d data size %d\n",i,num_masters,nlp->data_size);
		AddW1Bus( bus_master[i] ) ;
	}
}

#endif /* OW_W1 */
