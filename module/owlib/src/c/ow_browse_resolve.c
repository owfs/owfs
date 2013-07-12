/*
$Id$
    OWFS -- One-Wire filesystem
    OWHTTPD -- One-Wire Web Server
    Written 2003 Paul H Alfille
	email: paul.alfille@gmail.com
	Released under the GPL
	See the header file: ow.h for full attribution
	1wire/iButton system from Dallas Semiconductor
*/

#include <config.h>
#include "owfs_config.h"
#include "ow.h"

#include "ow_connection.h"

static struct port_in * CreateZeroPort(const char * name, const char * type, const char * domain, const char * host, const char * service );
static struct connection_out *FindOut(const char * name, const char * type, const char * domain);
static GOOD_OR_BAD Zero_nomatch(struct port_in * trial,struct port_in * existing);
static GOOD_OR_BAD string_null_or_match( const char * one, const char * two );

void ZeroAdd(const char * name, const char * type, const char * domain, const char * host, const char * service)
{
	struct port_in * pin ;

	// Don't add yourself
	if ( FindOut(name,type,domain) != NULL ) {
		LEVEL_DEBUG( "Attempt to add ourselves -- ignored" ) ;
		return ;
	}

	pin = CreateZeroPort( name, type, domain, host, service ) ;
	if ( pin != NULL ) {
		if ( BAD( Zero_detect(pin)) ) {
			LEVEL_DEBUG("Failed to create new %s", DEVICENAME(pin->first) ) ;
			RemovePort(pin) ;
		} else {
			Add_InFlight( Zero_nomatch, pin ) ;
		}
	}
}

void ZeroDel(const char * name, const char * type, const char * domain )
{
	struct port_in * pin = CreateZeroPort( name, type, domain, "", "" ) ; // example
	
	if ( pin != NULL ) {
		Del_InFlight( Zero_nomatch, pin ) ;
		RemovePort( pin ) ; // remove example
	}
}

static struct port_in * CreateZeroPort(const char * name, const char * type, const char * domain, const char * host, const char * service )
{
	char addr_name[128] ;
	struct port_in * pin = AllocPort(NO_CONNECTION);
	struct connection_in * in ;

	if ( pin == NULL ) {
		LEVEL_DEBUG( "Cannot allocate position for a new Port Master %s (%s:%s) -- ignored",name,host,service) ;
		return NO_CONNECTION ;
	}
	in = pin->first ;
	if ( in == NO_CONNECTION ) {
		LEVEL_DEBUG( "Cannot allocate position for a new Bus Master %s (%s:%s) -- ignored",name,host,service) ;
		return NO_CONNECTION ;
	}

	UCLIBCLOCK;
	snprintf(addr_name,127,"%s:%s",host,service) ;
	UCLIBCUNLOCK;
	DEVICENAME(in) = owstrdup(addr_name) ;
	pin->init_data = owstrdup(addr_name) ;
	pin->type = ct_tcp ;
	in->master.server.name   = owstrdup( name  ) ;
	in->master.server.type   = owstrdup( type  ) ;
	in->master.server.domain = owstrdup( domain) ;

	return pin ;
}

// GOOD means no match
static GOOD_OR_BAD Zero_nomatch(struct port_in * trial,struct port_in * existing)
{
	if ( get_busmode(existing->first) != bus_zero ) {
		return gbGOOD ;
	}
	if ( BAD( string_null_or_match( trial->first->master.server.name   , existing->first->master.server.name   )) ) {
		return gbGOOD ;
	}
	if ( BAD( string_null_or_match( trial->first->master.server.type   , existing->first->master.server.type   )) ) {
		return gbGOOD ;
	}
	if ( BAD( string_null_or_match( trial->first->master.server.domain , existing->first->master.server.domain )) ) {
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
