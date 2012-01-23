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

void COM_free(struct connection_in *connection)
{
	if (connection == NO_CONNECTION) {
		LEVEL_DEBUG("Attempt to close a NULL device");
		return ;
	}

	if ( connection != connection->channel_info.head ) {
		// Not the head of the multi-bus device
		return ;
	}

	switch ( SOC(connection)->type ) {
		case ct_unknown:
		case ct_none:
		case ct_usb:
			break ;
		case ct_telnet:
		case ct_tcp:
			tcp_free( connection ) ;
			break ;
		case ct_i2c:
		case ct_netlink:
			break ;
		case ct_serial:
			serial_free( connection ) ;
			break ;
	}

	SAFEFREE(SOC(connection)->devicename) ;
	SOC(connection)->state = cs_virgin ;
}
