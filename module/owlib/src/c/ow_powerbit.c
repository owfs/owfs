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

	/* Special handling for some adapters that can handle independent channels
	 * but others, like the DS2482-800 aren't completely independent
	 * */
	 
	/* Very tricky -- we will release the port but not the channel 
	 * so other threads can use the port's other channels
	 * but we have to be careful of deadlocks
	 * by fully releasing before relocking.
	 * We are locking and releasing out of sequence
	 * */

	/* At the end, we need to relock because that's what the surrounding code expects 
	 * -- that this routine is called with locked port and channel 
	 * and leaves with same
	 * */

	if ( in->iroutines.PowerBit !=NO_POWERBIT_ROUTINE ) {
		// use available bit level routine
		if (in->iroutines.flags & ADAP_FLAG_unlock_during_delay) {
			/* Can unlock port (but not channel) */
			PORTUNLOCKIN(in);
			ret = (in->iroutines.PowerBit) (data, resp, delay, pn);
			CHANNELUNLOCKIN(in); // have to release channel, too
			// now need to relock for further work in transaction
			BUSLOCKIN(in);
		} else {
			// adapter doesn't support other channels during power		
			ret = (in->iroutines.PowerBit) (data, resp, delay, pn);
		}
	} else { // send a bit and delay (use normal pull-up for power)
		if (in->iroutines.flags & ADAP_FLAG_unlock_during_delay) {
			ret = BUS_sendback_bits(&data, resp, 1, pn);
			/* Can unlock port (but not channel) */
			PORTUNLOCKIN(in);
			// other channels are accessible, but this channel is still locked
			// delay
			UT_delay(delay);
			CHANNELUNLOCKIN(in); // have to release channel, too
			// now need to relock for further work in transaction
			BUSLOCKIN(in);
			// Wait for other thread to complete
		} else {
			// send the packet but keep port locked
			// No real "power" in the default case
			ret = BUS_sendback_bits(&data, resp, 1, pn);
			// delay
			UT_delay(delay);
		}
	}
	if ( BAD(ret) ) {
		STAT_ADD1_BUS(e_bus_pullup_errors, in);
		return gbBAD ;
	}
	return gbGOOD ;
}
