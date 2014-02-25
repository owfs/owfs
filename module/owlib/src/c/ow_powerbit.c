/*
$Id$
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

// Send 8 bits of communication to the 1-Wire Net
// Delay delay msec and return to normal
//
// Power is supplied by normal pull-up resistor, nothing special
//
/* Returns 0=good
   bad = -EIO
 */
GOOD_OR_BAD BUS_PowerBit(BYTE data, BYTE * resp, UINT delay, const struct parsedname *pn)
{
	GOOD_OR_BAD ret;
	struct connection_in * in = pn->selected_connection ;

	if ( in->iroutines.PowerBit !=NO_POWERBIT_ROUTINE ) {
		ret = (in->iroutines.PowerBit) (data, resp, delay, pn);
	} else if (in->iroutines.flags & ADAP_FLAG_unlock_during_delay) {
		/* Very tricky -- we will release the port but not the channel 
		 * so other threads can use the port's other channels
		 * but we have to be careful of deadlocks
		 * by fully releasing before relocking.
		 * We are locking and releasing out of sequence
		 * */
		// No real "power" in the default case
		ret = BUS_sendback_bits(&data, resp, 1, pn);
		// delay
		PORTUNLOCKIN(in);
		// we no longer have exclusive control of the channel
		UT_delay(delay);
		CHANNELUNLOCKIN(in);
		// Could sneak into this channel
		BUSLOCKIN(in);
		// Wait for other thread to complete
		/* we need to relock because that's what the surrounding code expects 
		 * -- that this routine is called with locked port and channel 
		 * and leaves with same
		 * */
	} else {
		// send the packet
		// No real "power" in the default case
		ret = BUS_sendback_bits(&data, resp, 1, pn);
		// delay
		UT_delay(delay);
	}
	if ( BAD(ret) ) {
		STAT_ADD1_BUS(e_bus_pullup_errors, in);
		return gbBAD ;
	}
	return gbGOOD ;
}
