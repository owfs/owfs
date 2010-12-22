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

GOOD_OR_BAD COM_write( const BYTE * data, size_t length, struct connection_in *connection)
{
	ssize_t to_be_written = length ;
	FILE_DESCRIPTOR_OR_ERROR fd ;

	if ( length == 0 || data == NULL ) {
		return gbGOOD ;
	}

	if ( connection == NO_CONNECTION ) {
		return gbBAD ;
	}

	switch ( SOC(connection)->type ) {
		case ct_unknown:
		case ct_none:
			LEVEL_DEBUG("ERROR!!! ----------- ERROR!");
			return gbBAD ;
		case ct_i2c:
		case ct_usb:
			LEVEL_DEBUG("Unimplemented!!!");
			return gbBAD ;
		case ct_telnet:
		case ct_tcp:
		case ct_serial:
		case ct_netlink:
			break ;
	}

	RETURN_BAD_IF_BAD( COM_test(connection) ) ;
	fd = SOC(connection)->file_descriptor ;

	while (to_be_written > 0) {
		int select_result ;

		fd_set writeset;

		// use same timeout as read for write
		struct timeval tv = { Globals.timeout_serial, 0 } ;

		/* Initialize readset */
		FD_ZERO(&writeset);
		FD_SET( fd, &writeset);

		/* Read if it doesn't timeout first */
		select_result = select( fd + 1, NULL, &writeset, NULL, &tv);
		if (select_result > 0) {
			ssize_t write_result ;

			if (FD_ISSET( fd, &writeset) == 0) {
				ERROR_CONNECT("Select no FD found (write) serial port: %s", SAFESTRING(SOC(connection)->devicename));				
				STAT_ADD1_BUS(e_bus_write_errors, connection);
				return gbBAD;	/* error */
			}
			TrafficOut("write", &data[length - to_be_written], to_be_written, connection );
			write_result = write( fd, &data[length - to_be_written], to_be_written); /* write bytes */ 			
			if (write_result < 0) {
				if (errno != EWOULDBLOCK && errno != EAGAIN) {
					ERROR_CONNECT("Trouble writing to serial port: %s", SAFESTRING(SOC(connection)->devicename));
					COM_close(connection) ;
					STAT_ADD1_BUS(e_bus_write_errors, connection);
					return gbBAD;
				}
				/* write() was interrupted, try again */
				STAT_ADD1_BUS(e_bus_timeouts, connection);
			} else {
				to_be_written -= write_result ;	
			}
		} else {			/* timed out or select error */
			ERROR_CONNECT("Select/timeout error (write) serial port: %s", SAFESTRING(SOC(connection)->devicename));
			STAT_ADD1_BUS(e_bus_timeouts, connection);
			if ( errno == EBADF ) {
				LEVEL_DEBUG("Close file descriptor -- EBADF");
				COM_close(connection) ;
			}
			return gbBAD;
		}
	}

	tcdrain( fd );

	return gbGOOD ;
}
