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

GOOD_OR_BAD COM_read( BYTE * data, size_t length, struct connection_in *connection)
{
	ssize_t to_be_read = length ;
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
		case ct_telnet:
		case ct_tcp:
		case ct_i2c:
		case ct_netlink:
		case ct_usb:
			LEVEL_DEBUG("Unimplemented!!!");
			return gbBAD ;
		case ct_serial:
			break ;
	}

	RETURN_BAD_IF_BAD( COM_test(connection) ) ;
	fd = SOC(connection)->file_descriptor ;
	
	while (to_be_read > 0) {
		int select_result ;
		
		fd_set readset;
		
		// use same timeout for read as for write
		struct timeval tv = { Globals.timeout_serial, 0 } ;

		/* Initialize readset */
		FD_ZERO(&readset);
		FD_SET( fd, &readset);
		
		/* Read if it doesn't timeout first */
		select_result = select( fd + 1, &readset, NULL, NULL, &tv);
		if (select_result > 0) {
			ssize_t read_result ;
			
			if (FD_ISSET( fd, &readset) == 0) {
				ERROR_CONNECT("Select no FD found (read) serial port: %s", SAFESTRING(SOC(connection)->devicename));
				STAT_ADD1_BUS(e_bus_read_errors, connection);
				return gbBAD;	/* error */
			}
			read_result = read( fd, &data[length - to_be_read], to_be_read);	/* read bytes */
			if (read_result < 0) {
				if (errno != EWOULDBLOCK) {
					ERROR_CONNECT("Trouble reading from serial port: %s", SAFESTRING(SOC(connection)->devicename));
					COM_close(connection) ;
					STAT_ADD1_BUS(e_bus_read_errors, connection);
					return gbBAD;
				}
				/* write() was interrupted, try again */
				ERROR_CONNECT("Interrupt (read) serial port: %s", SAFESTRING(SOC(connection)->devicename));
				STAT_ADD1_BUS(e_bus_timeouts, connection);
			} else {
				TrafficIn( "read", &data[length - to_be_read], read_result, connection ) ;
				to_be_read -= read_result ;
			}
		} else if ( select_result == 0 ) { // timeout
			ERROR_CONNECT("Timeout error (read) serial port: %s", SAFESTRING(SOC(connection)->devicename));
			return gbBAD ;
		} else {			/* select error */
			if ( errno == EINTR ) {
				STAT_ADD1_BUS(e_bus_timeouts, connection);
				continue ;
			} else if ( errno == EBADF ) {
				LEVEL_DEBUG("Close file descriptor -- EBADF");
				COM_close(connection) ;
			}
			ERROR_CONNECT("Select error (read) serial port: %s", SAFESTRING(SOC(connection)->devicename));
			STAT_ADD1_BUS(e_bus_timeouts, connection);
			return gbBAD;
		}
	}
	
	tcdrain( fd );
	
	return gbGOOD ;
}
