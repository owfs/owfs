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

static void Slurp( FILE_DESCRIPTOR_OR_ERROR file_descriptor, unsigned long usec ) ;

void COM_slurp( struct connection_in * connection ) {
	unsigned long usec = 0 ; // just to suppress compiler warning

	if ( connection == NO_CONNECTION ) {
		return ;
	}

	switch ( connection->head->type ) {
		case ct_unknown:
		case ct_none:
			LEVEL_DEBUG("Unknown type");
			return ;
		case ct_telnet:
			// see if we can tell the port to dump all pending data
			if ( connection->head->dev.telnet.telnet_negotiated == completed_negotiation ) {
				if ( BAD( COM_test(connection) ) ) {
					return ;
				}
				telnet_purge( connection ) ;
			}
			// now do it the old fashioned way of swallowing the pending data
			// fall through
		case ct_tcp:
		case ct_netlink:
			usec = 100000 ;
			break ;
		case ct_i2c:
		case ct_usb:
			LEVEL_DEBUG("Unimplemented");
			return ;
		case ct_serial:
			usec = 1000 ;
			break ;
	}

	if ( BAD( COM_test(connection) ) ) {
		return ;
	}

	Slurp( connection->head->file_descriptor, usec ) ;
}

/* slurp up any pending chars -- used at the start to clear the com buffer */
static void Slurp( FILE_DESCRIPTOR_OR_ERROR file_descriptor, unsigned long usec )
{
	BYTE data[1] ;
	while (1) {
		fd_set readset;
		
		// very short timeout
		struct timeval tv = { 0, usec } ;
		
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
		if ( read(file_descriptor, data, 1) < 1 ) {
			return ;
		}
		TrafficInFD("slurp",data,1,file_descriptor);
	}
}
