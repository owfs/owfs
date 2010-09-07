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

// RESET called with bus locked
RESET_TYPE BUS_reset(const struct parsedname *pn)
{
	struct connection_in * in = pn->selected_connection ;
	STAT_ADD1_BUS(e_bus_resets, in);

	switch ( (in->iroutines.reset) (pn) ) {
	case BUS_RESET_OK:
		in->reconnect_state = reconnect_ok;	// Flag as good!
		if (in->ds2404_found && (in->iroutines.flags&ADAP_FLAG_no2404delay==0) ) {
			// extra delay for alarming DS1994/DS2404 complience
			UT_delay(5);
		}

		return BUS_RESET_OK ;
	case BUS_RESET_SHORT:
		/* Shorted 1-wire bus or minor error shouldn't cause a reconnect */
		in->AnyDevices = anydevices_unknown;
		LEVEL_CONNECT("1-wire bus short circuit.");
		STAT_ADD1_BUS(e_bus_short_errors, in);
		return BUS_RESET_SHORT;
	case BUS_RESET_ERROR:
		if ( in->ds2404_found ) {
			// extra reset for DS1994/DS2404 might be needed
			if ( (in->iroutines.reset) (pn) == BUS_RESET_OK ) {
				return BUS_RESET_OK ;
			}
		}
	default:
		in->reconnect_state++;	// Flag for eventual reconnection
		LEVEL_DEBUG("Reset error. Reconnection %d/%d",in->reconnect_state,reconnect_error); 
		STAT_ADD1_BUS(e_bus_reset_errors, in);
		return BUS_RESET_ERROR ;
	}
}
