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
	
	if ( length == 0 || data == NULL ) {
		return gbGOOD ;
	}
	
	if ( connection == NULL ) {
		return gbBAD ;
	}
	
	while (to_be_read > 0) {
		int select_result ;
		
		fd_set readset;
		struct timeval tv;
		
		// use same timeout as read as for write
		tv.tv_sec = Globals.timeout_serial;
		tv.tv_usec = 0;
		/* Initialize readset */
		FD_ZERO(&readset);
		FD_SET(connection->file_descriptor, &readset);
		
		/* Read if it doesn't timeout first */
		select_result = select(connection->file_descriptor + 1, &readset, NULL, NULL, &tv);
		if (select_result > 0) {
			ssize_t read_result ;
			
			if (FD_ISSET(connection->file_descriptor, &readset) == 0) {
				ERROR_CONNECT("Select no FD found (read) serial port: %s", SAFESTRING(connection->name));
				STAT_ADD1_BUS(e_bus_read_errors, connection);
				return gbBAD;	/* error */
			}
			update_max_delay(connection);
			read_result = read(connection->file_descriptor, &data[length - to_be_read], to_be_read);	/* read bytes */
			if (read_result < 0) {
				if (errno != EWOULDBLOCK) {
					ERROR_CONNECT("Trouble reading from serial port: %s", SAFESTRING(connection->name));
					STAT_ADD1_BUS(e_bus_read_errors, connection);
					return gbBAD;
				}
				/* write() was interrupted, try again */
				ERROR_CONNECT("Interrupt (read) serial port: %s", SAFESTRING(connection->name));
				STAT_ADD1_BUS(e_bus_timeouts, connection);
			} else {
				TrafficIn( "read", &data[length - to_be_read], read_result, connection ) ;
				to_be_read -= read_result ;
			}
		} else if ( select_result == 0 ) { // timeout
			ERROR_CONNECT("Timeout error (read) serial port: %s", SAFESTRING(connection->name));
			return gbBAD ;
		} else {			/* select error */
			if ( errno == EINTR ) {
				STAT_ADD1_BUS(e_bus_timeouts, connection);
				continue ;
			}
			ERROR_CONNECT("Select error (read) serial port: %s", SAFESTRING(connection->name));
			STAT_ADD1_BUS(e_bus_timeouts, connection);
			return gbBAD;
		}
	}
	
	tcdrain(connection->file_descriptor);
	gettimeofday(&(connection->bus_read_time), NULL);
	
	return gbGOOD ;
}
