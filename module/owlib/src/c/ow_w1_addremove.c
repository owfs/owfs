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

static struct connection_in * CreateIn(int bus_master) ;
static GOOD_OR_BAD W1_nomatch( struct connection_in * trial, struct connection_in * existing ) ;

static struct connection_in * CreateIn(int bus_master)
{
	struct connection_in * in = AllocIn(NULL) ;
	char name[63] ;
	int sn_ret ;

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

static GOOD_OR_BAD W1_nomatch( struct connection_in * trial, struct connection_in * existing )
{
	if ( existing->busmode != bus_w1 ) {
		return gbGOOD ;
	}
	if ( trial->connin.w1.id != existing->connin.w1.id ) {
		return gbGOOD ;
	}
	return gbBAD;
}


void AddW1Bus( int bus_master )
{
	struct connection_in * new_in = CreateIn(bus_master) ;
	
	LEVEL_DEBUG("Request master be added: w1_master%d.", bus_master);
	Add_InFlight( W1_nomatch, new_in ) ;
}

void RemoveW1Bus( int bus_master )
{
	Del_InFlight( W1_nomatch, CreateIn(bus_master) ) ;
}

#endif /* OW_W1 && OW_MT */
