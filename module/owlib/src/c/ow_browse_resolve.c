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

static void CreateIn(const char * name, const char * type, const char * domain, const char * host, const char * service ) ;
static struct connection_in *FindIn(const char * name, const char * type, const char * domain);
static struct connection_out *FindOut(const char * name, const char * type, const char * domain);
static GOOD_OR_BAD string_null_or_match( const char * one, const char * two );

void ZeroAdd(const char * name, const char * type, const char * domain, const char * host, const char * service)
{
	struct connection_in * in ;

	// Don't add yourself
	if ( FindOut(name,type,domain) != NULL ) {
		LEVEL_DEBUG( "Attempt to add ourselves -- ignored" ) ;
		return ;
	}

	CONNIN_WLOCK ;
	in = FindIn( name, type, domain ) ;
	if ( in != NULL ) {
		if ( GOOD( string_null_or_match( in->connin.tcp.host, host)) && GOOD( string_null_or_match(in->connin.tcp.service,service)) ) {
			LEVEL_DEBUG( "Repeat add of %s (%s:%s) -- ignored",name,host,service) ;
			CONNIN_WUNLOCK ;
			return ;
		} else {
			LEVEL_DEBUG( "The new connection replaces a previous entry" ) ;
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
		LEVEL_DEBUG( "Removing %s (bus.%d)",name,in->index) ;
		RemoveIn( in ) ;
	} else {
		LEVEL_DEBUG("Couldn't find matching bus to remove");
	}
	CONNIN_WUNLOCK ;
}

static void CreateIn(const char * name, const char * type, const char * domain, const char * host, const char * service )
{
	char addr_name[128] ;
	struct connection_in * in ;

	in = NewIn(NULL) ;
	if ( in == NULL ) {
		return ;
	}

	UCLIBCLOCK;
	snprintf(addr_name,128,"%s:%s",host,service) ;
	UCLIBCUNLOCK;
	in->name = owstrdup(addr_name) ;
	in->connin.tcp.name   = owstrdup( name  ) ;
	in->connin.tcp.type   = owstrdup( type  ) ;
	in->connin.tcp.domain = owstrdup( domain) ;

	if ( BAD( Zero_detect(in)) ) {
		LEVEL_DEBUG("Created a new bus.%d",in->index) ;
		RemoveIn(in) ;
	} else {
		LEVEL_DEBUG("Created a new bus.%d",in->index) ;
	}
}

// Finds matching connection
// returns it if found,
// else NULL
static struct connection_in *FindIn(const char * name, const char * type, const char * domain)
{
	struct connection_in *now ;
	for ( now = Inbound_Control.head ; now != NULL ; now = now->next ) {
		if ( now->busmode != bus_zero ) {
			continue ;
		}
		if ( BAD( string_null_or_match( name   , now->connin.tcp.name   )) ) {
			continue ;
		}
		if ( BAD( string_null_or_match( type   , now->connin.tcp.type   )) ) {
			continue ;
		}
		if ( BAD( string_null_or_match( domain , now->connin.tcp.domain )) ) {
			continue ;
		}
		return now ;
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
		if ( BAD( string_null_or_match( name   , now->zero.name   )) ) {
			continue ;
		}
		if ( BAD( string_null_or_match( type   , now->zero.type   )) ) {
			continue ;
		}
		if ( BAD( string_null_or_match( domain , now->zero.domain )) ) {
			continue ;
		}
		return now ;
	}
	return NULL;
}

static GOOD_OR_BAD string_null_or_match( const char * one, const char * two )
{
	if ( one == NULL ) {
		return (two==NULL) ? gbGOOD : gbBAD ;
	}
	if ( two == NULL ) {
		return gbBAD ;
	}
	return ( strcasecmp(one,two) == 0 ) ? gbGOOD : gbBAD ;
}
