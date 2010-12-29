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

/* ---------------------------------------------- */
/* raw COM port interface routines                */
/* ---------------------------------------------- */

//open a port (serial or tcp)
GOOD_OR_BAD COM_open(struct connection_in *connection)
{
	if (connection == NO_CONNECTION) {
		LEVEL_DEBUG("Attempt to open a NULL serial device");
		return gbBAD;
	}

	switch ( SOC(connection)->state ) {
		case cs_deflowered:
			// Attempt to reopen a good connection?
			COM_close(connection) ;
			break ;
		case cs_virgin:
			break ;
	}

	switch ( SOC(connection)->type ) {
		case ct_unknown:
		case ct_none:
			LEVEL_DEBUG("ERROR!!! ----------- ERROR!");
			return gbBAD ;
		case ct_telnet:
			SOC(connection)->dev.telnet.telnet_negotiated = 0 ;
			SOC(connection)->dev.telnet.telnet_supported = 0 ;
			return tcp_open( connection ) ;		
		case ct_tcp:
			return tcp_open( connection ) ;
		case ct_i2c:
		case ct_netlink:
		case ct_usb:
			LEVEL_DEBUG("Unimplemented!!!");
			return gbBAD ;
		case ct_serial:
			return serial_open( connection ) ;
	}
}
