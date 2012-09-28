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

static SIZE_OR_ERROR COM_read_get_size( BYTE * data, size_t length, struct connection_in *connection ) ;

// read length bytes.
// return BAD if any error or timeout before full length
/* Called on head of multibus group */
GOOD_OR_BAD COM_read( BYTE * data, size_t length, struct connection_in *connection)
{
	struct port_in * pin ;
	
	if ( length == 0 ) {
		return gbGOOD ;
	}
	
	if ( connection == NO_CONNECTION || data == NULL ) {
		// bad parameters
		return gbBAD ;
	}
	pin = connection->pown ;

	// unlike write or open, a closed connection isn't automatically opened.
	// the reason is that reopening won't have the data waiting. We really need
	// to restart the transaction from the "write" portion
	if ( FILE_DESCRIPTOR_NOT_VALID( pin->file_descriptor ) ) {
		return gbBAD ;
	}

	switch ( pin->type ) {
		// test the type of connection
		case ct_unknown:
		case ct_none:
			LEVEL_DEBUG("Unknown type");
			break ;
		case ct_telnet:
			return telnet_read( data, length, connection ) ;
		case ct_tcp:
			// network is ok
			return COM_read_get_size( data, length, connection ) == (ssize_t) length ? gbGOOD : gbBAD ;
		case ct_i2c:
		case ct_netlink:
		case ct_usb:
			LEVEL_DEBUG("Unimplemented");
			break ; 
		case ct_serial:
		// serial is ok
		// printf("Serial read fd=%d length=%d\n",pin->file_descriptor, (int) length);
		{
			ssize_t actual = COM_read_get_size( data, length, connection ) ;
			if ( FILE_DESCRIPTOR_VALID( pin->file_descriptor ) ) {
				// tcdrain only works on serial conections
				tcdrain( pin->file_descriptor );
				return actual == (ssize_t) length ? gbGOOD : gbBAD ;
			}
			break ;
		}
	}
	return gbBAD ;
}

// try to read bytes and return number actually read.
// Timeout ok
/* Called on head of multibus group */
SIZE_OR_ERROR COM_read_with_timeout( BYTE * data, size_t length, struct connection_in *connection)
{
	struct port_in * pin ;
	
	if ( length == 0 ) {
		return 0 ;
	}
	
	if ( connection == NO_CONNECTION || data == NULL ) {
		// bad parameters
		return -EIO ;
	}
	pin = connection->pown ;

	// unlike write or open, a closed connection isn't automatically opened.
	// the reason is that reopening won't have the data waiting. We really need
	// to restart the transaction from the "write" portion
	if ( FILE_DESCRIPTOR_NOT_VALID( pin->file_descriptor ) ) {
		return -EBADF ;
	} else {
		size_t actual_size ;
		ZERO_OR_ERROR zoe = tcp_read( pin->file_descriptor, data, length, &(pin->timeout), &actual_size ) ;

		if ( zoe == -EBADF ) {
			COM_close(connection) ;
			return zoe ;
		} else {
			return actual_size ;
		}
	}
}

// Read only if no errors
// Returns actual read size
/* Called on head of multibus group */
static SIZE_OR_ERROR COM_read_get_size( BYTE * data, size_t length, struct connection_in *connection )
{
	size_t actual_size ;
	struct port_in * pin = connection->pown ;
	ZERO_OR_ERROR zoe = tcp_read( pin->file_descriptor, data, length, &(pin->timeout), &actual_size ) ;

	if ( zoe < 0 ) {
		COM_close(connection) ;
		return zoe ;
	} else {
		return actual_size ;
	}
}

		
	
