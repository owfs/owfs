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

SIZE_OR_ERROR COM_read_size_low( BYTE * data, size_t length, struct connection_in *connection ) ;

GOOD_OR_BAD COM_read( BYTE * data, size_t length, struct connection_in *connection)
{
	if ( length == 0 ) {
		return gbGOOD ;
	}
	
	if ( connection == NO_CONNECTION || data == NULL ) {
		// bad parameters
		return gbBAD ;
	}

	// unlike write or open, a closed connection isn't automatically opened.
	// the reason is that reopening won't have the data waiting. We really need
	// to restart the transaction from the "write" portion
	if ( FILE_DESCRIPTOR_NOT_VALID( SOC(connection)->file_descriptor ) ) {
		return gbBAD ;
	}

	switch ( SOC(connection)->type ) {
		// test the type of connection
		case ct_unknown:
		case ct_none:
			LEVEL_DEBUG("ERROR!!! ----------- ERROR!");
			break ;
		case ct_telnet:
			return telnet_read( data, length, connection ) ;
		case ct_tcp:
			// network is ok
			return COM_read_size_low( data, length, connection ) < 0 ? gbBAD : gbGOOD ;
		case ct_i2c:
		case ct_netlink:
		case ct_usb:
			LEVEL_DEBUG("Unimplemented!!!");
			break ;
		case ct_serial:
		// serial is ok
		{
			ssize_t actual = COM_read_size_low( data, length, connection ) ;
			if ( FILE_DESCRIPTOR_VALID( SOC(connection)->file_descriptor ) ) {
				// tcdrain only works on serial conections
				tcdrain( SOC(connection)->file_descriptor );
				return actual < 0 ? gbBAD : gbGOOD ;
			}
			break ;
		}
	}
	return gbBAD ;
}

SIZE_OR_ERROR COM_read_size( BYTE * data, size_t length, struct connection_in *connection)
{
	if ( length == 0 ) {
		return 0 ;
	}
	
	if ( connection == NO_CONNECTION || data == NULL ) {
		// bad parameters
		return -EIO ;
	}

	// unlike write or open, a closed connection isn't automatically opened.
	// the reason is that reopening won't have the data waiting. We really need
	// to restart the transaction from the "write" portion
	if ( FILE_DESCRIPTOR_NOT_VALID( SOC(connection)->file_descriptor ) ) {
		return -EBADF ;
	}

	return COM_read_size_low( data, length, connection ) ;
}

SIZE_OR_ERROR COM_read_size_low( BYTE * data, size_t length, struct connection_in *connection )
{
	ssize_t actual_size ;
	ZERO_OR_ERROR zoe = tcp_read( SOC(connection)->file_descriptor, data, length, &(SOC(connection)->timeout), &actual_size ) ;

	if ( zoe == -EBADF ) {
		COM_close(connection) ;
		return zoe ;
	} else {
		return actual_size ;
	}
}

		
	
