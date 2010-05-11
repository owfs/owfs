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
	STAT_ADD1_BUS(e_bus_resets, pn->selected_connection);

	switch ( (pn->selected_connection->iroutines.reset) (pn) ) {
	case BUS_RESET_OK:
		pn->selected_connection->reconnect_state = reconnect_ok;	// Flag as good!
		return BUS_RESET_OK ;
	case BUS_RESET_SHORT:
		/* Shorted 1-wire bus or minor error shouldn't cause a reconnect */
		pn->selected_connection->AnyDevices = anydevices_unknown;
		LEVEL_CONNECT("1-wire bus short circuit.");
		STAT_ADD1_BUS(e_bus_short_errors, pn->selected_connection);
		return BUS_RESET_SHORT;
	case BUS_RESET_ERROR:
	default:
		pn->selected_connection->reconnect_state++;	// Flag for eventual reconnection
		LEVEL_DEBUG("Reset error. Reconnection %d/%d",pn->selected_connection->reconnect_state,reconnect_error); 
		STAT_ADD1_BUS(e_bus_reset_errors, pn->selected_connection);
		return BUS_RESET_ERROR ;
	}
}
