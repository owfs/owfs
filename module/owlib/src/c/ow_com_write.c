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

	if ( length == 0 || data == NULL ) {
		return gbGOOD ;
	}

	if ( connection == NO_CONNECTION ) {
		return gbBAD ;
	}

	if ( FILE_DESCRIPTOR_NOT_VALID(connection->file_descriptor) ) {
		return gbBAD ;
	}

	while (to_be_written > 0) {
		int select_result ;

		fd_set writeset;

		// use same timeout as read for write
		struct timeval tv = { Globals.timeout_serial, 0 } ;

		/* Initialize readset */
		FD_ZERO(&writeset);
		FD_SET(connection->file_descriptor, &writeset);

		/* Read if it doesn't timeout first */
		select_result = select(connection->file_descriptor + 1, NULL, &writeset, NULL, &tv);
		if (select_result > 0) {
			ssize_t write_result ;

			if (FD_ISSET(connection->file_descriptor, &writeset) == 0) {
				ERROR_CONNECT("Select no FD found (write) serial port: %s", SAFESTRING(connection->name));
				STAT_ADD1_BUS(e_bus_write_errors, connection);
				return gbBAD;	/* error */
			}
			TrafficOut("write", &data[length - to_be_written], to_be_written, connection );
			write_result = write(connection->file_descriptor, &data[length - to_be_written], to_be_written); /* write bytes */ 			
			if (write_result < 0) {
				if (errno != EWOULDBLOCK && errno != EAGAIN) {
					ERROR_CONNECT("Trouble writing to serial port: %s", SAFESTRING(connection->name));
					Test_and_Close( &(connection->file_descriptor) ) ;
					STAT_ADD1_BUS(e_bus_write_errors, connection);
					return gbBAD;
				}
				/* write() was interrupted, try again */
				STAT_ADD1_BUS(e_bus_timeouts, connection);
			} else {
				to_be_written -= write_result ;	
			}
		} else {			/* timed out or select error */
			ERROR_CONNECT("Select/timeout error (write) serial port: %s", SAFESTRING(connection->name));
			STAT_ADD1_BUS(e_bus_timeouts, connection);
			if ( errno == EBADF ) {
				LEVEL_DEBUG("Close file descriptor -- EBADF");
				Test_and_Close( &(connection->file_descriptor) ) ;
			}
			return gbBAD;
		}
	}

	tcdrain(connection->file_descriptor);

	return gbGOOD ;
}
