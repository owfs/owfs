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

static SIZE_OR_ERROR COM_read_size( BYTE * data, size_t length, struct connection_in *connection ) ;

GOOD_OR_BAD COM_read( BYTE * data, size_t length, struct connection_in *connection)
{
	if ( length == 0 || data == NULL ) {
		return gbGOOD ;
	}
	
	if ( connection == NO_CONNECTION ) {
		return gbBAD ;
	}

	if ( FILE_DESCRIPTOR_NOT_VALID( SOC(connection)->file_descriptor ) ) {
		return gbBAD ;
	}

	switch ( SOC(connection)->type ) {
		case ct_unknown:
		case ct_none:
			LEVEL_DEBUG("ERROR!!! ----------- ERROR!");
			return gbBAD ;
		case ct_telnet:
		case ct_tcp:
			return COM_read_size( data, length, connection ) < 0 ? gbBAD : gbGOOD ;
		case ct_i2c:
		case ct_netlink:
		case ct_usb:
			LEVEL_DEBUG("Unimplemented!!!");
			return gbBAD ;
		case ct_serial:
		{
			ssize_t actual = COM_read_size( data, length, connection ) ;
			if ( FILE_DESCRIPTOR_VALID( SOC(connection)->file_descriptor ) ) {
				tcdrain( SOC(connection)->file_descriptor );
				return actual < 0 ? gbBAD : gbGOOD ;
			}
			return gbBAD ;
		}
	}
}

static SIZE_OR_ERROR COM_read_size( BYTE * data, size_t length, struct connection_in *connection )
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

		
	
