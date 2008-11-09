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

static int CreateIn(const char * name, const char * type, const char * domain, const char * address, int port, struct connection_in * in ) ;
static struct connection_in *FindIn(const char * name, const char * type, const char * domain);

void ZeroAdd(const char * name, const char * type, const char * domain, const char * address, int port)
{
	struct connection_in * in = FindIn( name, type, domain ) ;
	
	if ( in != NULL ) {
		BUSUNLOCKIN(in);
		LEVEL_DEBUG( "Zeroconf: Repeat add of %s (%s:%d) -- ignored\n",name,address,port) ;
		return ;
	}
	
	CONNIN_WLOCK ;
	in = NewIn(NULL) ;
	CONNIN_WUNLOCK ;
	if ( in != NULL ) {
		CreateIn( name,type, domain, address, port, in ) ;
		return ;
	}
}

void ZeroDel(const char * name, const char * type, const char * domain )
{
	struct connection_in * in = FindIn( name, type, domain ) ;
	
	if ( in != NULL ) {
		RemoveIn( in ) ;
		LEVEL_DEBUG( "Zeroconf: Removing %s \n",name) ;
		return ;
	}
}	

static int CreateIn(const char * name, const char * type, const char * domain, const char * address, int port, struct connection_in * in )
{
	char addr_name[128] ;
	UCLIBCLOCK;
	snprintf(addr_name,128,"%s:%d",address,port) ;
	UCLIBCUNLOCK;
	in->name = strdup(addr_name) ;
	in->connin.tcp.name = strdup(name) ;
	in->connin.tcp.type = strdup(type) ;
	in->connin.tcp.domain = strdup(domain) ;
	if ( Zero_detect(in) != 0 ) {
		RemoveIn(in) ;
		return 1 ;
	}
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
	for (; now != NULL; now = now->next) {
		if (now->busmode != bus_zero || strcasecmp(now->name,name)
			|| strcasecmp(now->connin.tcp.type, type)
			|| strcasecmp(now->connin.tcp.domain, domain)
			) {
			continue;
		}
		break;
	}
	return now;
}
