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

#include <config.h>
#include "owfs_config.h"
#include "ow_connection.h"

/* Can only be called by a separate (monitoring) thread  so the write-lock won't deadlock */
/* This allows adding new Bus Masters while program is running
 * The master is usually only read-locked for normal operation
 * write lock is done in a separate thread when no requests are being processed */
void Add_InFlight( GOOD_OR_BAD (*nomatch)(struct connection_in * trial,struct connection_in * existing), struct port_in * new_pin )
{
	struct connection_in * new_in ;
	if ( new_pin == NULL ) {
		return ;
	}

	new_in = new_pin->first ;
	LEVEL_DEBUG("Request master be added: %s", SOC(new_in)->devicename);

	CONNIN_WLOCK ;
	if ( nomatch != NULL ) {
		struct port_in * pin ;
		for ( pin = Inbound_Control.head_port ; pin != NULL ; pin = pin->next ) {
			struct connection_in * cin ;
			for ( cin = pin->first ; cin != NO_CONNECTION ; cin = cin->next ) {
				if ( GOOD( nomatch( new_in, cin )) ) {
					continue ;
				}
				LEVEL_DEBUG("Already exists as index=%d",cin->index) ;
				CONNIN_WUNLOCK ;
				RemovePort( new_pin ) ;
				return ;
			}
		}
	}
	LinkPort(new_pin);
	CONNIN_WUNLOCK ;
}
