/*
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
#include "ow_ftdi.h"

#ifdef HAVE_LINUX_LIMITS_H
#include <linux/limits.h>
#endif

void COM_close(struct connection_in *connection)
{
	struct port_in * pin ;
	
	if (connection == NO_CONNECTION) {
		LEVEL_DEBUG("Attempt to close a NULL device");
		return ;
	}
	pin = connection->pown ;

	switch ( pin->type ) {
		case ct_unknown:
		case ct_none:
		case ct_usb:
			LEVEL_DEBUG("ERROR!!! ----------- ERROR!");
			return ;
		case ct_telnet:
		case ct_tcp:
			break ;
		case ct_i2c:
		case ct_netlink:
			LEVEL_DEBUG("Unimplemented!!!");
			return ;
		case ct_serial:
			break ;
		case ct_ftdi:
#if OW_FTDI
			return owftdi_close(connection);
#endif
			break;
	}

	switch ( pin->state ) {
		case cs_virgin:
			break ;
		default:
		case cs_deflowered:
			Test_and_Close( &( pin->file_descriptor) ) ;
			break ;
	}
}
