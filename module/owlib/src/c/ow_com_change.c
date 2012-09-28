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

#ifdef HAVE_LINUX_LIMITS_H
#include <linux/limits.h>
#endif

GOOD_OR_BAD COM_change( struct connection_in *connection)
{
	struct port_in * pin ;
	
	if ( connection == NO_CONNECTION ) {
		return gbBAD ;
	}
	pin = connection->pown ;

	// is connection thought to be open?
	RETURN_BAD_IF_BAD( COM_test(connection) ) ;

	switch ( pin->type ) {
		case ct_i2c:
		case ct_usb:
			LEVEL_DEBUG("Unimplemented!!!");
			return gbBAD ;
		case ct_telnet:
			// set to change settings at next write
			if ( pin->dev.telnet.telnet_negotiated == completed_negotiation ) {
				pin->dev.telnet.telnet_negotiated = needs_negotiation ;
			}
			return gbGOOD ;
		case ct_tcp:
		case ct_netlink:
			LEVEL_DEBUG("Cannot change baud rate on %s",SAFESTRING(DEVICENAME(connection))) ;
			return gbGOOD ;
		case ct_serial:
			return serial_change( connection ) ;
		case ct_unknown:
		case ct_none:
		default:
			LEVEL_DEBUG("ERROR!!! ----------- ERROR!");
			return gbBAD ;
	}
}

void COM_set_standard( struct connection_in *connection)
{
	struct port_in * pin = connection->pown ;
	
	pin -> baud            = B9600 ;
	pin -> vmin            = 0;           // minimum chars
	pin -> vtime           = 3;           // decisec wait
	pin -> parity          = parity_none; // parity
	pin -> stop            = stop_1;      // stop bits
	pin -> bits            = 8;           // bits/byte
	pin -> state           = cs_virgin ;
	pin -> dev.telnet.telnet_negotiated = needs_negotiation ;

	connection->CRLF_size = 2 ;

	switch (pin->type) {
		case ct_telnet:
			pin->timeout.tv_sec = Globals.timeout_network ;
			pin->timeout.tv_usec = 0 ;
			break ;

		case ct_serial:
		default:
			pin->timeout.tv_sec = Globals.timeout_serial ;
			pin->timeout.tv_usec = 0 ;
			break ;
	}

	pin->flow            = flow_first;  // flow control
}
