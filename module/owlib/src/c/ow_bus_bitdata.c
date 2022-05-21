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
#include "ow.h"
#include "ow_counters.h"
#include "ow_connection.h"
#include "ow_codes.h"

/** BUS_send_data
    Send a data and expect response match
 */
GOOD_OR_BAD BUS_send_bits(const BYTE * data, const size_t len, const struct parsedname *pn)
{
	BYTE resp[len];

	if (len == 0) {
		return gbGOOD;
	}

	if ( BAD( BUS_sendback_bits(data, resp, len, pn) ) ) {
		STAT_ADD1_BUS(e_bus_errors, pn->selected_connection);
		return gbBAD ;
	}

	RETURN_GOOD_IF_GOOD( BUS_compare_bits( data, resp, len ) ) ;

	LEVEL_DEBUG("Response doesn't match bits sent");
	STAT_ADD1_BUS(e_bus_errors, pn->selected_connection);
	return gbBAD ;
}

// compare bits (just non-zero or zero)
GOOD_OR_BAD BUS_compare_bits(const BYTE * data1, const BYTE * data2, const size_t len)
{
	size_t i ;

	if (len == 0) {
		return gbGOOD;
	}

	for ( i=0 ; i< len ; ++i ) {
		if ( (data1[i]==0) != (data2[i]==0) ) {
			return gbBAD ;
		}
	}
	return gbGOOD;
}

/** readin_data
  Send 0xFFs and return response block
 */
GOOD_OR_BAD BUS_readin_bits(BYTE * data, const size_t len, const struct parsedname *pn)
{
	memset(data, 0xFF, (size_t) len) ;
	RETURN_GOOD_IF_GOOD( BUS_sendback_bits( data, data, len, pn) ) ;

	STAT_ADD1(BUS_readin_data_errors);
	return gbBAD;
}

GOOD_OR_BAD BUS_sendback_bits( const BYTE * databits, BYTE * respbits, const size_t len, const struct parsedname * pn )
{
	GOOD_OR_BAD (*sendback_bits) (const BYTE * databits, BYTE * respbits, const size_t len, const struct parsedname * pn) = ((pn)->selected_connection->iroutines.sendback_bits) ;
	if ( sendback_bits != NO_SENDBACKBITS_ROUTINE ) {
		return (sendback_bits)((databits),(respbits),(len),(pn)) ;
	}
	return gbBAD ;
}

