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

#if OW_W1 && OW_MT

#include "ow_w1.h"
#include "ow_connection.h"

static void * AddBus( void * v );
static void * RemoveBus( void * v );
static struct connection_in * CreateIn(int bus_master) ;
static struct connection_in *FindIn(int bus_master) ;

static struct connection_in * CreateIn(int bus_master)
{
	struct connection_in * in ;
	char name[63] ;
	int sn_ret ;

	in = NewIn(NULL) ;
	if ( in == NULL ) {
		return NULL ;
	}

	UCLIBCLOCK ;
	sn_ret = snprintf(name,62,"w1_bus_master%d",bus_master) ;
	UCLIBCLOCK ;
	if ( sn_ret < 0 ) {
		RemoveIn(in) ;
		return NULL ;
	}	

	in->name = owstrdup(name) ;
	in->connin.w1.id = bus_master ;
	if ( BAD( W1_detect(in)) ) {
		RemoveIn(in) ;
		return NULL ;
	}	
		 
	LEVEL_DEBUG("Created a new bus %s",in->name) ;
	return in ;
}

// Finds matching connection
// returns it if found,
// else NULL
static struct connection_in *FindIn(int bus_master)
{
	struct connection_in *now ;
	for ( now = Inbound_Control.head ; now != NULL ; now = now->next ) {
		//printf("Matching %d/%s/%s/%s/ to bus.%d %d/%s/%s/%s/\n",bus_zero,name,type,domain,now->index,now->busmode,now->connin.tcp.name,now->connin.tcp.type,now->connin.tcp.domain);
		if ( (now->busmode == bus_w1) && (now->connin.w1.id == bus_master) ) {
			return now ;
		}
	}
	return NULL;
}

/* (Dynamically) Add a new w1 bus to the list of connections */
static void * AddBus( void * v )
{
	int bus_master = ((int *) v)[0] ;
	struct connection_in * in ;

	pthread_detach(pthread_self());
	owfree(v) ;

	CONNIN_WLOCK ;

	if ( (in = FindIn(bus_master)) != NULL ) {
		LEVEL_DEBUG("Attempt to re-add bus master: %s",in->name);
	} else if ( (in = CreateIn(bus_master)) != NULL ) {
		in->connin.w1.id = bus_master ;
	} else {
		LEVEL_DEBUG("w1 bus <%d> couldn't be added",bus_master) ;
	}

	CONNIN_WUNLOCK ;
	return NULL ;
}

static void * RemoveBus( void * v )
{
	int bus_master = ((int *) v)[0] ;
	struct connection_in * in ;

	pthread_detach(pthread_self());
	owfree(v) ;

	CONNIN_WLOCK ;

	in =  FindIn( bus_master ) ;
	if ( in != NULL ) {
		LEVEL_DEBUG("Removing <%s>",in->name) ;
		RemoveIn(in) ;
	} else {
		LEVEL_DEBUG("Attempt to remove non-existent W1 master %d",bus_master);
	}
	CONNIN_WUNLOCK ;
	return NULL ;
}

/* Need to run the add/remove in a separate thread so that netlink messages can still be parsed and CONNIN_RLOCK won't deadlock */
void AddW1Bus( int bus_master )
{
	pthread_t thread;
	int err ;
	int * copy_bus_master = owmalloc( sizeof(bus_master) ) ;
	
	LEVEL_DEBUG("Request master be added: w1_master%d.",	bus_master);
	if ( copy_bus_master == NULL) {
		LEVEL_DEBUG("Cannot allocate memory to pass bus_master=%d\n", bus_master) ;
		return ;
	}
	copy_bus_master[0] = bus_master ;

	err = pthread_create(&thread, NULL, AddBus, copy_bus_master );
	if (err) {
		LEVEL_CONNECT("W1 bus add thread error %d.", err);
		owfree(copy_bus_master) ;
	}
}

void RemoveW1Bus( int bus_master )
{
	pthread_t thread;
	int err ;
	int * copy_bus_master = owmalloc( sizeof(bus_master) ) ;
	
	LEVEL_DEBUG("Request master be removed: w1_master%d.", bus_master);
	if ( copy_bus_master == NULL) {
		LEVEL_DEBUG("Cannot allocate memory to pass bus_master=%d\n", bus_master) ;
		return ;
	}
	copy_bus_master[0] = bus_master ;

	err = pthread_create(&thread, NULL, RemoveBus, copy_bus_master );
	if (err) {
		LEVEL_CONNECT("W1 bus add thread error %d.", err);
		owfree(copy_bus_master) ;
	}
}

#endif /* OW_W1 && OW_MT */
