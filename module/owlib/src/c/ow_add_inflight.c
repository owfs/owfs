/*
$Id$
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

/* Can only be called by a separate (monitoring) thread  so the write-lock won't deadlock */
/* This allows adding new Bus Masters while program is running
 * The master is usually only read-locked for normal operation
 * write lock is done in a separate thread when no requests are being processed */
void Add_InFlight( GOOD_OR_BAD (*nomatch)(struct port_in * trial,struct port_in * existing), struct port_in * new_pin )
{
	if ( new_pin == NULL ) {
		return ;
	}

	LEVEL_DEBUG("Request master be added: %s", DEVICENAME(new_pin->first));

	CONNIN_WLOCK ;
	if ( nomatch != NULL ) {
		struct port_in * pin ;
		for ( pin = Inbound_Control.head_port ; pin != NULL ; pin = pin->next ) {
			if ( GOOD( nomatch( new_pin, pin )) ) {
				continue ;
			}
			LEVEL_DEBUG("Already exists as index=%d",pin->first->index) ;
			CONNIN_WUNLOCK ;
			RemovePort( new_pin ) ;
			return ;
		}
	}
	LinkPort(new_pin);
	CONNIN_WUNLOCK ;
}
