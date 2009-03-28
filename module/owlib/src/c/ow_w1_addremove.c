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

#if OW_W1

#include "ow_w1.h"
#include "ow_connection.h"

static struct connection_in * CreateIn(const char * name ) ;
static struct connection_in *FindIn(const char * name ) ;

static struct connection_in * CreateIn(const char * name )
{
	struct connection_in * in ;

	in = NewIn(NULL) ;
	if ( in != NULL ) {
		in->name = owstrdup(name) ;
		W1_detect(in) ;
		LEVEL_DEBUG("Created a new bus.%d\n",in->index) ;
	}

	return in ;
}

// Finds matching connection
// returns it if found,
// else NULL
static struct connection_in *FindIn(const char * name)
{
	struct connection_in *now ;
	for ( now = Inbound_Control.head ; now != NULL ; now = now->next ) {
		//printf("Matching %d/%s/%s/%s/ to bus.%d %d/%s/%s/%s/\n",bus_zero,name,type,domain,now->index,now->busmode,now->connin.tcp.name,now->connin.tcp.type,now->connin.tcp.domain);
		if ( now->busmode == bus_w1
			&& now->name   != NULL
			&& strcasecmp( now->name  , name  ) == 0
			) {
			return now ;
		}
	}
	return NULL;
}

static void * AddBus( void * v )
{
	int bus_master = ((int *) v)[0] ;
	char name[63] ;
	int sn_ret ;
	struct connection_in * in ;

#if OW_MT
	pthread_detach(pthread_self());
#endif /* OW_MT */

	UCLIBCLOCK ;
	sn_ret = snprintf(name,62,"w1_bus_master%d",bus_master) ;
	UCLIBCLOCK ;

	if ( sn_ret < 0 ) {
		return NULL ;
	}

	CONNIN_WLOCK ;

	if ( (in = FindIn(name)) != NULL ) {
		LEVEL_DEBUG("w1 bus <%s> already known\n",name) ;
		in->connin.w1.entry_mark = Inbound_Control.w1_entry_mark ;
		;
	} else if ( (in = CreateIn(name)) != NULL ) {
		LEVEL_DEBUG("w1 bus <%s> to be added\n",name) ;
		in->connin.w1.entry_mark = Inbound_Control.w1_entry_mark ;
		in->connin.w1.id = bus_master ;
	} else {
		LEVEL_DEBUG("w1 bus <%s> couldn't be added\n",name) ;
	}

	CONNIN_WUNLOCK ;
	LEVEL_DEBUG("Normal exit.\n");
	return NULL ;
}

void * RemoveBus( void * v )
{
	int bus_master = ((int *) v)[0] ;
	char name[63] ;
	int sn_ret ;
	struct connection_in * in ;

#if OW_MT
	pthread_detach(pthread_self());
#endif /* OW_MT */

	UCLIBCLOCK ;
	sn_ret = snprintf(name,62,"w1_bus_master%d",bus_master) ;
	UCLIBCLOCK ;

	if ( sn_ret < 0 ) {
		return NULL ;
	}

	CONNIN_WLOCK ;

	in =  FindIn( name ) ;
	if ( in != NULL ) {
		RemoveIn(in) ;
		LEVEL_DEBUG("<%s>\n",name) ;
	}

	CONNIN_WUNLOCK ;
	LEVEL_DEBUG("Normal exit.\n");
	return NULL ;
}

#if OW_MT

/* Need to run the add/remove in a separate thread so that netlink messatges can still be parsed and CONNIN_RLOCK won't deadlock */
void AddW1Bus( int bus_master )
{
	pthread_t thread;
	int err = pthread_create(&thread, NULL, AddBus, &bus_master );
	if (err) {
		LEVEL_CONNECT("W1 bus add thread error %d.\n", err);
	}
}

void RemoveW1Bus( int bus_master )
{
	pthread_t thread;
	int err = pthread_create(&thread, NULL, RemoveBus, &bus_master );
	if (err) {
		LEVEL_CONNECT("W1 bus add thread error %d.\n", err);
	}
}

#else /* OW_MT */

void AddW1Bus( int bus_master )
{
	AddBus( &bus_master) ;
}

void RemoveW1Bus( int bus_master )
{
	RemoveBus( &bus_master) ;
}

#endif /* OW_MT */
#endif /* OW_W1 */
