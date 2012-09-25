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
/* This allows removing Bus Masters while program is running
 * The master is usually only read-locked for normal operation
 * write lock is done in a separate thread when no requests are being processed */
void Del_InFlight( GOOD_OR_BAD (*nomatch)(struct connection_in * trial,struct connection_in * existing), struct port_in * old_pin )
{
	struct connection_in * old_in ;
	if ( old_pin == NULL ) {
		return ;
	}

	old_in = old_pin->first ;
	LEVEL_DEBUG("Request master be removed: %s", DEVICENAME(old_in));

	CONNIN_WLOCK ;
	if ( nomatch != NULL ) {
		struct port_in * pin ;
		for ( pin = Inbound_Control.head_port ; pin != NULL ; pin = pin->next ) {
			struct connection_in * cin = pin->first ;
			while ( cin != NO_CONNECTION ) {
				struct connection_in * next = cin->next ;
				if ( BAD( nomatch( old_in, cin )) ) {
					LEVEL_DEBUG("Removing BUS index=%d %s",cin->index,SAFESTRING(DEVICENAME(cin)));
					RemoveIn(cin) ;
				}
				cin = next ;
			}
		}
	}
	CONNIN_WUNLOCK ;
	RemovePort(old_pin);
}
