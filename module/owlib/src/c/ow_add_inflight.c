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

static void * AddBus( void * v );

/* (Dynamically) Add a new w1 bus to the list of connections */
static void * AddBus( void * v )
{
	struct connection_in * new_in = v ;

	pthread_detach(pthread_self());

	CONNIN_WLOCK ;

	LinkIn(new_in);

	CONNIN_WUNLOCK ;
	return NULL ;
}

/* Need to run the add/remove in a separate thread so that 1-wire requests can still be parsed and CONNIN_RLOCK won't deadlock */
/* This allows adding new Bus Masters while program is running
 * The connin is usually only read-locked for normal operation
 * write lock is done in a separate thread when no requests are being processed */
void Add_InFlight( struct connection_in * new_in )
{
	pthread_t thread;
	int err ;
	
	LEVEL_DEBUG("Request master be added: %s", new_in->name);

	err = pthread_create(&thread, NULL, AddBus, new_in );
	if (err) {
		LEVEL_CONNECT("Add thread error %d.", err);
		RemoveIn(new_in);
	}
}

#else /* OW_MT */

void Add_InFlight( struct connection_in * new_in )
{
	(void) new_in ;
}

#endif /* OW_MT */
