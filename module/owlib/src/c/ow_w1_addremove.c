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

static struct port_in * CreateW1Port(int bus_master) ;
static GOOD_OR_BAD W1_nomatch( struct connection_in * trial, struct connection_in * existing ) ;

// Create a new bus master (W1 type)
static struct port_in * CreateW1Port(int bus_master)
{
	struct port_in * pin = AllocPort(NULL) ;
	struct connection_in * in ;
	char name[63] ;
	int sn_ret ;

	if ( pin == NO_CONNECTION ) {
		return NULL ;
	}
	in = pin->first ;

	UCLIBCLOCK ;
	sn_ret = snprintf(name,62,"w1_bus_master%d",bus_master) ;
	UCLIBCUNLOCK ;
	if ( sn_ret < 0 ) {
		RemovePort(pin) ;
		return NULL ;
	}	

	pin->init_data = owstrdup(name) ;
	in->master.w1.id = bus_master ;
	pin->busmode = bus_w1 ;
	in->master.w1.w1_slave_order = w1_slave_order_unknown ;
	Init_Pipe( in->master.w1.netlink_pipe ) ;
	if ( BAD( W1_detect(pin)) ) {
		RemovePort(pin) ;
		return NULL ;
	}	
		 
	LEVEL_DEBUG("Setup structure for %s",DEVICENAME(in)) ;
	return pin ;
}

static GOOD_OR_BAD W1_nomatch( struct connection_in * trial, struct connection_in * existing )
{
	if ( get_busmode(existing) != bus_w1 ) {
		return gbGOOD ;
	}
	if ( trial->master.w1.id != existing->master.w1.id ) {
		return gbGOOD ;
	}
	return gbBAD;
}


void AddW1Bus( int bus_master )
{
	struct port_in * new_pin = CreateW1Port(bus_master) ;
	
	if ( new_pin == NULL ) {
		LEVEL_DEBUG("cannot create a new W1 master");
		return ;
	}

	LEVEL_DEBUG("Request master be added: w1_bus_master%d.", bus_master);
	new_pin->type = ct_none ;
	Add_InFlight( W1_nomatch, new_pin ) ;
}

void RemoveW1Bus( int bus_master )
{
	struct port_in * pin = CreateW1Port( bus_master ) ;
	
	Del_InFlight( W1_nomatch, pin ) ; // tolerant of pin==NULL
}

#endif /* OW_W1 && OW_MT */
