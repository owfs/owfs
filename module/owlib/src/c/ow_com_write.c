/*
$Id$
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

#ifdef HAVE_LINUX_LIMITS_H
#include <linux/limits.h>
#endif

static GOOD_OR_BAD COM_write_once( const BYTE * data, size_t length, struct connection_in *connection) ;

/* Called on head of multibus group */
GOOD_OR_BAD COM_write( const BYTE * data, size_t length, struct connection_in *connection)
{
	struct port_in * pin ;
	
	if ( connection == NO_CONNECTION ) {
		return gbBAD ;
	}
	pin = connection->pown ;

	switch ( pin->type ) {
		case ct_unknown:
		case ct_none:
			LEVEL_DEBUG("Bad bus type for writing %s",SAFESTRING(DEVICENAME(connection)));
			return gbBAD ;
		case ct_i2c:
		case ct_usb:
			// usb and i2c use their own write logic currently
			LEVEL_DEBUG("Unimplemented write %s",SAFESTRING(DEVICENAME(connection)));
			return gbBAD ;
		case ct_telnet:
			// telnet gets special processing to send communication parameters (in band)
			if ( pin->dev.telnet.telnet_negotiated == needs_negotiation ) {
				if (Globals.traffic) {
					LEVEL_DEBUG("TELNET: Do negotiation");
				}
				RETURN_BAD_IF_BAD(  telnet_change( connection ) ) ;
				pin->dev.telnet.telnet_negotiated = completed_negotiation ;
			}
			break ;
		case ct_tcp:
		case ct_serial:
		case ct_netlink:
			// These protocols need no special pre-processing
			break ;
	}
	// is connection thought to be open?
	RETURN_BAD_IF_BAD( COM_test(connection) ) ;

	if ( length == 0 || data == NULL ) {
		return gbGOOD ;
	}

	// try the write
	RETURN_GOOD_IF_GOOD( COM_write_once( data, length, connection ) );
	
	LEVEL_DEBUG("Trouble writing to %s", SAFESTRING(DEVICENAME(connection)) ) ;

	if ( connection->pown->file_descriptor == FILE_DESCRIPTOR_BAD ) {
		// connection was bad, now closed, try again
		LEVEL_DEBUG("Need to reopen %s", SAFESTRING(DEVICENAME(connection)) ) ;
		RETURN_BAD_IF_BAD( COM_test(connection) ) ;
		LEVEL_DEBUG("Reopened %s, now slurp", SAFESTRING(DEVICENAME(connection)) ) ;
		COM_slurp(connection) ;
		LEVEL_DEBUG("Write again to %s", SAFESTRING(DEVICENAME(connection)) ) ;
		return COM_write_once( data, length, connection ) ;
	}

	// Can't open or another type of error
	return gbBAD ;
}

// No retries, let the upper level handle problems.

GOOD_OR_BAD COM_write_simple( const BYTE * data, size_t length, struct connection_in *connection)
{
	struct port_in * pin ;
	
	if ( length == 0 || data == NULL ) {
		return gbGOOD ;
	}

	if ( connection == NO_CONNECTION ) {
		return gbBAD ;
	}
	pin = connection->pown ;

	switch ( pin->type ) {
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

	if ( pin->file_descriptor == FILE_DESCRIPTOR_BAD ) {
		LEVEL_DEBUG("Writing to closed device %d",SAFESTRING(DEVICENAME(connection)));
		return gbBAD ;
	}

	return COM_write_once( data, length, connection ) ;
}

/* Called on head of multibus group */
static GOOD_OR_BAD COM_write_once( const BYTE * data, size_t length, struct connection_in *connection)
{
	ssize_t to_be_written = length ;
	FILE_DESCRIPTOR_OR_ERROR fd = connection->pown->file_descriptor ;

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
				ERROR_CONNECT("Select no FD found on write to %s", SAFESTRING(DEVICENAME(connection)));				
				STAT_ADD1_BUS(e_bus_write_errors, connection);
				return gbBAD;	/* error */
			}
			TrafficOut("write", &data[length - to_be_written], to_be_written, connection );
			write_result = write( fd, &data[length - to_be_written], to_be_written); /* write bytes */ 			
			if (write_result < 0) {
				if (errno != EWOULDBLOCK && errno != EAGAIN) {
					ERROR_CONNECT("Trouble writing to %s", SAFESTRING(DEVICENAME(connection)));
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
			ERROR_CONNECT("Select/timeout error writing to %s", SAFESTRING(DEVICENAME(connection)));
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
