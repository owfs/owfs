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
 * The connin is usually only read-locked for normal operation
 * write lock is done in a separate thread when no requests are being processed */
void Del_InFlight( GOOD_OR_BAD (*nomatch)(struct connection_in * trial,struct connection_in * existing), struct connection_in * old_in )
{
	if ( old_in == NULL ) {
		return ;
	}

	LEVEL_DEBUG("Request master be removed: %s", old_in->name);

	CONNIN_WLOCK ;
	if ( nomatch != NULL ) {
		struct connection_in * in = Inbound_Control.head ;
		while ( in != NULL ) {
			struct connection_in * next = in-> next ;
			if ( BAD( nomatch( old_in, in )) ) {
				LEVEL_DEBUG("Removing BUS index=%d %s",in->index,in->name);
				RemoveIn(in) ;
			}
			in = next ;
		}
	}
	CONNIN_WUNLOCK ;
	RemoveIn(old_in);
}
