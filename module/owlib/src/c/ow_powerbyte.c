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

// Send 8 bits of communication to the 1-Wire Net
// Delay delay msec and return to normal
//
// Power is supplied by normal pull-up resistor, nothing special
//
/* Returns 0=good
   bad = -EIO
 */
GOOD_OR_BAD BUS_PowerByte(BYTE data, BYTE * resp, UINT delay, const struct parsedname *pn)
{
	GOOD_OR_BAD ret;
	struct connection_in * in = pn->selected_connection ;

	if ( in->iroutines.PowerByte !=NO_POWERBYTE_ROUTINE ) {
		// use available byte level routine
		ret = (in->iroutines.PowerByte) (data, resp, delay, pn);
	} else if ( in->iroutines.PowerBit != NO_POWERBIT_ROUTINE && in->iroutines.sendback_bits != NO_SENDBACKBITS_ROUTINE ) {
		// use available bit level routines
		BYTE sending[8];
		BYTE receive[8] ;
		int i ;
		for ( i = 0 ; i < 8 ; ++i ) {
			sending[i] = UT_getbit(&data,i) ? 0xFF : 0x00 ;
		}
		ret = BUS_sendback_bits( sending, receive, 7, pn ) ;
		if ( GOOD(ret) ) {
			ret = BUS_PowerBit( sending[7], &receive[7], delay, pn ) ;
		}
		for ( i = 0 ; i < 8 ; ++i ) {
			UT_setbit( resp, i, receive[i] ) ;
		}
	} else {
		// send with no true power applied
		ret = BUS_sendback_data(&data, resp, 1, pn);
		// delay
		UT_delay(delay);
	}
	if ( BAD(ret) ) {
		STAT_ADD1_BUS(e_bus_pullup_errors, in);
		return gbBAD ;
	}
	return gbGOOD ;
}
