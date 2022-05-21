/*
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
#include "ow_connection.h"

/* Debugging interface to showing all bus traffic
 * You need to configure compile with
 */

static struct connection_in * Bus_from_file_descriptor( FILE_DESCRIPTOR_OR_ERROR file_descriptor )
{
	struct port_in * pin ; 
	
	for ( pin=Inbound_Control.head_port ; pin != NULL ; pin=pin->next ) {
		if ( pin->file_descriptor == file_descriptor ) {
			return pin->first ;
		}
	}
	return NO_CONNECTION ;
}

void TrafficOut( const char * data_type, const BYTE * data, size_t length, const struct connection_in * in )
{
	if (Globals.traffic) {
		fprintf(stderr, "TRAFFIC OUT <%s> bus=%d (%s)\n", SAFESTRING(data_type), in->index, DEVICENAME(in) ) ;
		_Debug_Bytes( in->adapter_name, data, length ) ;
	}
}

void TrafficIn( const char * data_type, const BYTE * data, size_t length, const struct connection_in * in )
{
	if (Globals.traffic) {
		fprintf(stderr, "TRAFFIC IN  <%s> bus=%d (%s)\n", SAFESTRING(data_type), in->index, DEVICENAME(in) ) ;
		_Debug_Bytes( in->adapter_name, data, length ) ;
	}
}

void TrafficOutFD( const char * data_type, const BYTE * data, size_t length, FILE_DESCRIPTOR_OR_ERROR file_descriptor )
{
	if (Globals.traffic) {
		struct connection_in * in = Bus_from_file_descriptor( file_descriptor ) ;
		if ( in != NO_CONNECTION ) {
			TrafficOut( data_type, data, length, in ) ;
		} else {
			fprintf(stderr, "TRAFFIC OUT <%s> file descriptor=%d\n", SAFESTRING(data_type), file_descriptor ) ;
			_Debug_Bytes( "FD", data, length ) ;
		}
	}
}

void TrafficInFD( const char * data_type, const BYTE * data, size_t length, FILE_DESCRIPTOR_OR_ERROR file_descriptor )
{
	if (Globals.traffic) {
		struct connection_in * in = Bus_from_file_descriptor( file_descriptor ) ;
		if ( in != NO_CONNECTION ) {
			TrafficIn( data_type, data, length, in ) ;
		} else {
			fprintf(stderr, "TRAFFIC IN  <%s> file descriptor=%d\n", SAFESTRING(data_type), file_descriptor ) ;
			_Debug_Bytes( "FD", data, length ) ;
		}
	}
}
