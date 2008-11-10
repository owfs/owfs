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
#include "ow.h"

#include "ow_connection.h"

#if OW_W1

static struct connection_in * CreateIn(const char * name ) ;
static struct connection_in *FindIn(const char * name ) ;

void W1Scan(const char * name, const char * type, const char * domain, const char * host, const char * service)
{
	struct connection_in * in ;

	CONNIN_WLOCK ;

	++Inbound_Control.w1_seq ;
	
	for ( loop_through_w1_bus ) {
		in = FindIn( name ) ;

		if ( in == NULL ) {
			in = CreateIn(name) ;
		}

		if ( in != NULL ) {
			in->connin.w1.seq = Inbound_Control.w1_seq ;
		}
	}
	
	for ( in = Inbound_Control.head ; in ; in = in->next ) {
		if ( in->busmode == bus_w1
			&& in->connin.w1.seq  != Inbound_Control.w1_seq
			) {
			RemoveIn( in ) ;
		}
	}
		
	CONNIN_WUNLOCK ;
}

static struct connection_in * CreateIn(const char * name )
{
	struct connection_in * in ;

	in = NewIn(NULL) ;
	if ( in != NULL ) {
		in->name = strdup(addr_name) ;
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

#endif /* OW_W1 */