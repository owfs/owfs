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

int COM_read( BYTE * data, size_t length, const struct parsedname * pn )
{
	struct connection_in * connection = pn->selected_connection ;
	ssize_t to_be_read = length ;
	
	if ( length == 0 || data == NULL ) {
		return 0 ;
	}
	
	if ( connection == NULL ) {
		return -EIO ;
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
				ERROR_CONNECT("Select error serial port: %s\n", SAFESTRING(connection->name));
				STAT_ADD1_BUS(e_bus_read_errors, connection);
				return -EIO;	/* error */
			}
			update_max_delay(pn);
			read_result = read(connection->file_descriptor, &data[length - to_be_read], to_be_read);	/* write bytes */
			if (read_result < 0) {
				if (errno != EWOULDBLOCK) {
					ERROR_CONNECT("Trouble reading from serial port: %s\n", SAFESTRING(connection->name));
					STAT_ADD1_BUS(e_bus_read_errors, connection);
					return read_result;
				}
				/* write() was interrupted, try again */
				ERROR_CONNECT("Interrupt serial port: %s\n", SAFESTRING(connection->name));
				STAT_ADD1_BUS(e_bus_timeouts, connection);
			} else {
				Debug_Bytes("Serial read:", &data[length - to_be_read], read_result);
				to_be_read -= read_result ;
			}
		} else {			/* timed out or select error */
			if ( errno == EINTR ) {
				STAT_ADD1_BUS(e_bus_timeouts, connection);
				continue ;
			}
			ERROR_CONNECT("Select or timeout error serial port: %s\n", SAFESTRING(connection->name));
			STAT_ADD1_BUS(e_bus_timeouts, connection);
			return -errno;
		}
	}
	
	tcdrain(connection->file_descriptor);
	gettimeofday(&(connection->bus_read_time), NULL);
	
	return to_be_read ;
}

/* slurp up any pending chars -- used at the start to clear the com buffer */
void COM_slurp( int file_descriptor )
{
	BYTE data[1] ;
	while (1) {
		fd_set readset;
		struct timeval tv;
		
		// very short timeout
		tv.tv_sec = 0;
		tv.tv_usec = 1000;
		/* Initialize readset */
		FD_ZERO(&readset);
		FD_SET(file_descriptor, &readset);
		
		/* Read if it doesn't timeout first */
		if ( select(file_descriptor + 1, &readset, NULL, NULL, &tv) < 1 ) {
			return ;
		}
		if (FD_ISSET(file_descriptor, &readset) == 0) {
			return ;
		}
		if ( read(file_descriptor, data, 1) != 1 ) {
			return ;
		}
	}
}
