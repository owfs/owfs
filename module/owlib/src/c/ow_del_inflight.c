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

static void * DelBus( void * v );

/* (Dynamically) remove a bus from the list of connections */
static void * DelBus( void * v )
{
	struct connection_in * old_in = v ;

	pthread_detach(pthread_self());

	CONNIN_WLOCK ;

	RemoveIn(old_in);

	CONNIN_WUNLOCK ;
	return NULL ;
}

/* Need to run the add/remove in a separate thread so that 1-wire requests can still be parsed and CONNIN_RLOCK won't deadlock */
/* This allows adding new Bus Masters while program is running
 * The connin is usually only read-locked for normal operation
 * write lock is done in a separate thread when no requests are being processed */
void Del_InFlight( struct connection_in * new_in )
{
	pthread_t thread;
	int err ;
	
	LEVEL_DEBUG("Request master be removed: %s", old_in->name);

	err = pthread_create(&thread, NULL, DelBus, old_in );
	if (err) {
		LEVEL_CONNECT("Remove thread error %d.", err);
		RemoveIn(old_in);
	}
}

#else /* OW_MT */

void Del_InFlight( struct connection_in * old_in )
{
	RemoveIn(old_in);
}

#endif /* OW_MT */
