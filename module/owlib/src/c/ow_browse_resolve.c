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

static int CreateIn(const char * name, const char * type, const char * domain, const char * host, const char * service ) ;
static struct connection_in *FindIn(const char * name, const char * type, const char * domain);

void ZeroAdd(const char * name, const char * type, const char * domain, const char * host, const char * service)
{
	struct connection_in * in = FindIn( name, type, domain ) ;
	
	if ( in != NULL ) {
		if ( in->connin.tcp.host && strcmp(in->connin.tcp.host,host)==0 && in->connin.tcp.service && strcmp(in->connin.tcp.service,service)==0 ) {
			LEVEL_DEBUG( "Zeroconf: Repeat add of %s (%s:%s) -- ignored\n",name,host,service) ;
			return ;
		} else {
			LEVEL_DEBUG( "Zeroconf: Add a new connection replaces a previous entry\n" ) ;
			RemoveIn(in) ;
		}
	}
	
	CreateIn( name, type, domain, host, service ) ;
}

void ZeroDel(const char * name, const char * type, const char * domain )
{
	struct connection_in * in = FindIn( name, type, domain ) ;
	
	if ( in != NULL ) {
		LEVEL_DEBUG( "Zeroconf: Removing %s (bus.%d)\n",name,in->index) ;
		RemoveIn( in ) ;
		return ;
	} else {
		LEVEL_DEBUG("Couldn't find matching bus to remove\n");
	}
}	

static int CreateIn(const char * name, const char * type, const char * domain, const char * host, const char * service )
{
	char addr_name[128] ;
	struct connection_in * in ;

	CONNIN_WLOCK ;
	in = NewIn(NULL) ;
	CONNIN_WUNLOCK ;
	if ( in == NULL ) {
		return 1 ;
	}

	UCLIBCLOCK;
	snprintf(addr_name,128,"%s:%s",host,service) ;
	UCLIBCUNLOCK;
	in->name = strdup(addr_name) ;
	in->connin.tcp.name   = strdup( name  ) ;
	in->connin.tcp.type   = strdup( type  ) ;
	in->connin.tcp.domain = strdup( domain) ;

	if ( Zero_detect(in) != 0 ) {
		LEVEL_DEBUG("Created a new bus.%d\n",in->index) ;
		RemoveIn(in) ;
		return 1 ;
	}
	LEVEL_DEBUG("Created a new bus.%d\n",in->index) ;
	return 0 ;
}

// Finds matching connection
// returns it if found,
// else NULL
static struct connection_in *FindIn(const char * name, const char * type, const char * domain)
{
	struct connection_in *now;
	CONNIN_RLOCK;
	now = Inbound_Control.head;
	CONNIN_RUNLOCK;
	while ( now != NULL ) {
		//printf("Matching %d/%s/%s/%s/ to bus.%d %d/%s/%s/%s/\n",bus_zero,name,type,domain,now->index,now->busmode,now->connin.tcp.name,now->connin.tcp.type,now->connin.tcp.domain);
		if (now->busmode == bus_zero
			&& strcasecmp( now->connin.tcp.name  , name  ) == 0
			&& strcasecmp( now->connin.tcp.type  , type  ) == 0
			&& strcasecmp( now->connin.tcp.domain, domain) == 0
			) {
			return now ;
		}
		now = now->next ;
	}
	return NULL;
}
