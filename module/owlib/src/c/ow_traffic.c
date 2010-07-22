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
#include "ow_connection.h"

/* Debugging interface to showing all bus traffic
 * You need to configure compile with
 * ./configure --enable-owtraffic
 * */

#if OW_SHOW_TRAFFIC

void TrafficOut( const char * data_type, const BYTE * data, size_t length, const struct connection_in * in )
{
	fprintf(stderr, "TRAFFIC OUT <%s> bus=%d (%s)\n", SAFESTRING(data_type), in->index, in->name ) ;
	_Debug_Bytes( in->adapter_name, data, length ) ;
}

void TrafficIn( const char * data_type, const BYTE * data, size_t length, const struct connection_in * in )
{
	fprintf(stderr, "TRAFFIC IN  <%s> bus=%d (%s)\n", SAFESTRING(data_type), in->index, in->name ) ;
	_Debug_Bytes( in->adapter_name, data, length ) ;
}

#endif /* OW_SHOW_TRAFFIC */
