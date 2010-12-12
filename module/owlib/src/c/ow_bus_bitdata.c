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
#include "ow_counters.h"
#include "ow_connection.h"
#include "ow_codes.h"

GOOD_OR_BAD BUS_sendback_bits( const BYTE * databits, BYTE * respbits, const size_t len, const struct parsedname * pn )
{
	GOOD_OR_BAD (*sendback_bits) (const BYTE * databits, BYTE * respbits, const size_t len, const struct parsedname * pn) = ((pn)->selected_connection->iroutines.sendback_bits) ;
	if ( sendback_bits != NO_SENDBACKBITS_ROUTINE ) {
		return (sendback_bits)((databits),(respbits),(len),(pn)) ;
	}
	return gbBAD ;
}

