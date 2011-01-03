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

GOOD_OR_BAD COM_test( struct connection_in * connection )
{
	if (connection == NO_CONNECTION) {
		LEVEL_DEBUG("Attempt to open a NULL serial device");
		return gbBAD;
	}

	switch ( SOC(connection)->type ) {
		case ct_unknown:
		case ct_none:
			LEVEL_DEBUG("ERROR!!! ----------- ERROR!");
			return gbBAD ;
		case ct_telnet:
		case ct_tcp:
			break ;
		case ct_i2c:
		case ct_netlink:
		case ct_usb:
			LEVEL_DEBUG("Unimplemented!!!");
			return gbBAD ;
		case ct_serial:
			break ;
	}

	if ( SOC(connection)->state == cs_virgin ) {
		LEVEL_DEBUG("Auto initialization of %s", SAFESTRING(SOC(connection)->devicename)) ;
	} else if ( FILE_DESCRIPTOR_VALID( SOC(connection)->file_descriptor ) ) {
		return gbGOOD ;
	}
	return COM_open(connection) ;
}

void COM_flush( const struct connection_in *connection)
{
	if (connection == NO_CONNECTION) {
		LEVEL_DEBUG("Attempt to flush a NULL device");
		return ;
	}

	switch ( SOC(connection)->type ) {
		case ct_unknown:
		case ct_none:
			LEVEL_DEBUG("ERROR!!! ----------- ERROR!");
			return ;
		case ct_telnet:
		case ct_tcp:
			tcp_read_flush( SOC(connection)->file_descriptor) ;
			break ;
		case ct_netlink:
		case ct_i2c:
		case ct_usb:
			LEVEL_DEBUG("Unimplemented!!!");
			return ;
		case ct_serial:
			tcflush( SOC(connection)->file_descriptor, TCIOFLUSH);
			break ;
	}
}

void COM_break(struct connection_in *in)
{
	tcsendbreak(SOC(in)->file_descriptor, 0);
}
