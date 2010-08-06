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

#if OW_MT

/* Can only be called by a separate (monitoring) thread  so the write-lock won't deadlock */
/* This allows adding new Bus Masters while program is running
 * The connin is usually only read-locked for normal operation
 * write lock is done in a separate thread when no requests are being processed */
void Add_InFlight( struct connection_in * new_in )
{
	LEVEL_DEBUG("Request master be added: %s", new_in->name);

	CONNIN_WLOCK ;
	LinkIn(new_in);
	CONNIN_WUNLOCK ;
}

#else /* OW_MT */

void Add_InFlight( struct connection_in * new_in )
{
	LEVEL_DEBUG("Request master be added: %s", new_in->name);
	LinkIn(new_in);
}

#endif /* OW_MT */
