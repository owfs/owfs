/*
    OWFS -- One-Wire filesystem
    OWHTTPD -- One-Wire Web Server
    Written 2003 Paul H Alfille
	email: paul.alfille@gmail.com
	Released under the GPL
	See the header file: ow.h for full attribution
	1wire/iButton system from Dallas Semiconductor
*/

#include <config.h>
#include "owfs_config.h"
#include "ow_connection.h"

static GOOD_OR_BAD Default_nomatch( struct port_in * trial, struct port_in * existing ) ;

/* Can only be called by a separate (monitoring) thread  so the write-lock won't deadlock */
/* This allows removing Bus Masters while program is running
 * The master is usually only read-locked for normal operation
 * write lock is done in a separate thread when no requests are being processed */
void Del_InFlight( GOOD_OR_BAD (*nomatch)(struct port_in * trial,struct port_in * existing), struct port_in * old_pin )
{
	struct connection_in * old_in ;
	struct port_in * pin ;

	if ( old_pin == NULL ) {
		return ;
	}

	old_in = old_pin->first ;
	LEVEL_DEBUG("Request master be removed: %s", DEVICENAME(old_in));

	if ( nomatch == NULL ) {
		nomatch = Default_nomatch ;
	}

	CONNIN_WLOCK ;
	for ( pin = Inbound_Control.head_port ; pin != NULL ; pin = pin->next ) {
		if ( BAD( nomatch( old_pin, pin )) ) {
			LEVEL_DEBUG("Removing BUS index=%d %s",pin->first->index,SAFESTRING(DEVICENAME(pin->first)));
			RemovePort(pin) ;
		}
	}
	CONNIN_WUNLOCK ;
}

// GOOD means no match
static GOOD_OR_BAD Default_nomatch( struct port_in * trial, struct port_in * existing )
{
	// just delete this port
	if ( trial != existing ) {
		return gbGOOD ;
	}
	return gbBAD;
}

