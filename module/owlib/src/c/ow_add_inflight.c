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
 * The connin is usually only read-locked for normal operation
 * write lock is done in a separate thread when no requests are being processed */
void Add_InFlight( GOOD_OR_BAD (*nomatch)(struct connection_in * trial,struct connection_in * existing), struct connection_in * new_in )
{
	if ( new_in == NULL ) {
		return ;
	}

	LEVEL_DEBUG("Request master be added: %s", new_in->name);

	CONNIN_WLOCK ;
	if ( nomatch != NULL ) {
		struct connection_in * in ;
		for ( in = Inbound_Control.head ; in != NULL ; in = in->next ) {
			if ( GOOD( nomatch( new_in, in )) ) {
				continue ;
			}
			LEVEL_DEBUG("Already exists as index=%d",in->index) ;
			CONNIN_WUNLOCK ;
			RemoveIn( new_in ) ;
			return ;
		}
	}
	LinkIn(new_in);
	CONNIN_WUNLOCK ;
}
