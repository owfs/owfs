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

GOOD_OR_BAD BUS_detect( struct port_in * pin )
{
	GOOD_OR_BAD (*detect) (struct port_in * pin) = pin->first->iroutines.detect ;
	if ( detect != NO_DETECT_ROUTINE ) {
		return (detect)(pin) ;
	}
	return gbBAD ;
}

void BUS_close( struct connection_in * in )
{
	void (*bus_close) (struct connection_in * in) = in->iroutines.close ;
	if ( bus_close != NO_CLOSE_ROUTINE ) {
		return (bus_close)(in);
	}
}

