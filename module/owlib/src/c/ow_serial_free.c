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

//free serial port and restore attributes

void serial_free(struct connection_in *connection)
{
	FILE_DESCRIPTOR_OR_ERROR fd ;

	if ( SOC(connection)->state == cs_virgin ) {
		return ;
	}

	fd = SOC(connection)->file_descriptor ;
	if ( FILE_DESCRIPTOR_NOT_VALID( fd ) ) {
		// reopen to restore attributes
		fd = open( SOC(connection)->devicename, O_RDWR | O_NONBLOCK | O_NOCTTY) ;
	}

	// restore tty settings
	if ( FILE_DESCRIPTOR_VALID( fd ) ) {
		LEVEL_DEBUG("COM_close: flush");
		tcflush( fd, TCIOFLUSH);
		LEVEL_DEBUG("COM_close: restore");
		if ( tcsetattr( fd, TCSANOW, &(SOC(connection)->dev.serial.oldSerialTio) ) < 0) {
			ERROR_CONNECT("Cannot restore port attributes: %s", SAFESTRING(SOC(connection)->devicename));
		}
	}
	Test_and_Close( &( SOC(connection)->file_descriptor) ) ;
}

