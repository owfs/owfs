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

static struct connection_in * CreateIn(const char * name, const char * type, const char * domain, const char * host, const char * service );
static struct connection_out *FindOut(const char * name, const char * type, const char * domain);
static GOOD_OR_BAD Zero_nomatch(struct connection_in * trial,struct connection_in * existing);
static GOOD_OR_BAD string_null_or_match( const char * one, const char * two );

void ZeroAdd(const char * name, const char * type, const char * domain, const char * host, const char * service)
{
	struct connection_in * in ;

	// Don't add yourself
	if ( FindOut(name,type,domain) != NULL ) {
		LEVEL_DEBUG( "Attempt to add ourselves -- ignored" ) ;
		return ;
	}

	in = CreateIn( name, type, domain, host, service ) ;
	if ( BAD( Zero_detect(in)) ) {
		LEVEL_DEBUG("Failed to create new %s",in->name) ;
		RemoveIn(in) ;
	} else {
		Add_InFlight( Zero_nomatch, in ) ;
	}
}

void ZeroDel(const char * name, const char * type, const char * domain )
{
	struct connection_in * in = CreateIn( name, type, domain, "", "" ) ;
	
	if ( in != NULL ) {
		SAFEFREE(in->name) ;
		in->name = owstrdup(name) ;
		Del_InFlight( Zero_nomatch, in ) ;
	}
}

static struct connection_in * CreateIn(const char * name, const char * type, const char * domain, const char * host, const char * service )
{
	char addr_name[128] ;
	struct connection_in * in = AllocIn(NULL);

	if ( in == NULL ) {
		LEVEL_DEBUG( "Cannot allocate position for a new Bus Master %s (%s:%s) -- ignored",name,host,service) ;
		return NULL ;
	}

	UCLIBCLOCK;
	snprintf(addr_name,127,"%s:%s",host,service) ;
	UCLIBCUNLOCK;
	in->name = owstrdup(addr_name) ;
	in->master.tcp.name   = owstrdup( name  ) ;
	in->master.tcp.type   = owstrdup( type  ) ;
	in->master.tcp.domain = owstrdup( domain) ;

	return in ;
}

// GOOD means no match
static GOOD_OR_BAD Zero_nomatch(struct connection_in * trial,struct connection_in * existing)
{
	if ( existing->busmode != bus_zero ) {
		return gbGOOD ;
	}
	if ( BAD( string_null_or_match( trial->master.tcp.name   , existing->master.tcp.name   )) ) {
		return gbGOOD ;
	}
	if ( BAD( string_null_or_match( trial->master.tcp.type   , existing->master.tcp.type   )) ) {
		return gbGOOD ;
	}
	if ( BAD( string_null_or_match( trial->master.tcp.domain , existing->master.tcp.domain )) ) {
		return gbGOOD ;
	}
	return gbBAD ;
}

// Finds matching connection in OUTBOUND list (so you don't match yourself)
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
