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
static struct connection_out *FindOut(const char * name, const char * type, const char * domain);

void ZeroAdd(const char * name, const char * type, const char * domain, const char * host, const char * service)
{
	struct connection_in * in ;

	// Don't add yourself
	if ( FindOut(name,type,domain) != NULL ) {
		LEVEL_DEBUG( "Attempt to add ourselves -- ignored\n" ) ;
		return ;
	}

	CONNIN_WLOCK ;
	in = FindIn( name, type, domain ) ;

	if ( in != NULL ) {
		if ( in->connin.tcp.host && strcmp(in->connin.tcp.host,host)==0 && in->connin.tcp.service && strcmp(in->connin.tcp.service,service)==0 ) {
			LEVEL_DEBUG( "Repeat add of %s (%s:%s) -- ignored\n",name,host,service) ;
			CONNIN_WUNLOCK ;
			return ;
		} else {
			LEVEL_DEBUG( "The new connection replaces a previous entry\n" ) ;
			RemoveIn(in) ;
		}
	}

	CreateIn( name, type, domain, host, service ) ;
	CONNIN_WUNLOCK ;
}

void ZeroDel(const char * name, const char * type, const char * domain )
{
	struct connection_in * in ;

	CONNIN_WLOCK ;
	in = FindIn( name, type, domain ) ;
	if ( in != NULL ) {
		LEVEL_DEBUG( "Removing %s (bus.%d)\n",name,in->index) ;
		RemoveIn( in ) ;
	} else {
		LEVEL_DEBUG("Couldn't find matching bus to remove\n");
	}
	CONNIN_WUNLOCK ;
}

static int CreateIn(const char * name, const char * type, const char * domain, const char * host, const char * service )
{
	char addr_name[128] ;
	struct connection_in * in ;

	in = NewIn(NULL) ;
	if ( in == NULL ) {
		return 1 ;
	}

	UCLIBCLOCK;
	snprintf(addr_name,128,"%s:%s",host,service) ;
	UCLIBCUNLOCK;
	in->name = owstrdup(addr_name) ;
	in->connin.tcp.name   = owstrdup( name  ) ;
	in->connin.tcp.type   = owstrdup( type  ) ;
	in->connin.tcp.domain = owstrdup( domain) ;

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
	struct connection_in *now ;
	for ( now = Inbound_Control.head ; now != NULL ; now = now->next ) {
		//printf("Matching %d/%s/%s/%s/ to bus.%d %d/%s/%s/%s/\n",bus_zero,name,type,domain,now->index,now->busmode,now->connin.tcp.name,now->connin.tcp.type,now->connin.tcp.domain);
		if ( now->busmode != bus_zero
			|| now->connin.tcp.name   == NULL
			|| now->connin.tcp.type   == NULL
			|| now->connin.tcp.domain == NULL
			) {
			continue ;
		}
		if (
			   strcasecmp( now->connin.tcp.name  , name  ) == 0
			&& strcasecmp( now->connin.tcp.type  , type  ) == 0
			&& strcasecmp( now->connin.tcp.domain, domain) == 0
			) {
			return now ;
		}
	}
	return NULL;
}

// Finds matching connection
// returns it if found,
// else NULL
static struct connection_out *FindOut(const char * name, const char * type, const char * domain)
{
	struct connection_out *now ;
	for ( now = Outbound_Control.head ; now != NULL ; now = now->next ) {
		if (	   now->zero.name   == NULL
			|| now->zero.type   == NULL
			|| now->zero.domain == NULL
			) {
			continue ;
		}
		if (	   strcasecmp( now->zero.name  , name  ) == 0
			&& strcasecmp( now->zero.type  , type  ) == 0
			&& strcasecmp( now->zero.domain, domain) == 0
			) {
			return now ;
		}
	}
	return NULL;
}
