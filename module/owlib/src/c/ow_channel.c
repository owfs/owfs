/*
$Id$
    OWFS -- One-Wire filesystem
    OWHTTPD -- One-Wire Web Server
    Written 2003 Paul H Alfille
	email: palfille@earthlink.net
	Released under the GPL
	See the header file: ow.h for full attribution
	1wire/iButton system from Dallas Semiconductor
*/

/* Common routines for a multi-channel bus master */
/* OWSERVER_ENET HA5 DS2482-x00 are examples */

#include <config.h>
#include "owfs_config.h"
#include "ow.h"
#include "ow_counters.h"
#include "ow_connection.h"
#include "ow_codes.h"

void OW_channel_init( struct connection_in * in )
{
	if ( in == NO_CONNECTION ) {
		return ;
	}
	in->channel_info.channel = '\0' ;
	in->channel_info.head = in ;
	in->channel_info.psoc = &(in->soc) ;
	_MUTEX_INIT(in->channel_info.all_channel_lock);
}

void OW_channel_close( struct connection_in * in )
{
	if ( in == NO_CONNECTION ) {
		return ;
	}
	if ( in->channel_info.head == in ) {
		_MUTEX_DESTROY(in->channel_info.all_channel_lock);
	}
	in->channel_info.head = NO_CONNECTION ;
}

void OW_channel_lock( struct connection_in * in )
{
	struct connection_in * head_in = in->channel_info.head ;
	if ( head_in != NO_CONNECTION ) {
		_MUTEX_LOCK(head_in->channel_info.all_channel_lock ) ;
	}
}

void OW_channel_unlock( struct connection_in * in )
{
	struct connection_in * head_in = in->channel_info.head ;
	if ( head_in != NO_CONNECTION ) {
		_MUTEX_UNLOCK(head_in->channel_info.all_channel_lock ) ;
	}
}

	
