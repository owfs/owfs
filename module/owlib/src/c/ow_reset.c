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
//#include "ow_codes.h"

// RESET called with bus locked
int BUS_reset(const struct parsedname *pn)
{
	int ret = (pn->selected_connection->iroutines.reset) (pn);
	/* Shorted 1-wire bus or minor error shouldn't cause a reconnect */
	if (ret == BUS_RESET_OK) {
		pn->selected_connection->reconnect_state = reconnect_ok;	// Flag as good!
	} else if (ret == BUS_RESET_SHORT) {
		pn->selected_connection->AnyDevices = anydevices_unknown;
		STAT_ADD1_BUS(e_bus_short_errors, pn->selected_connection);
		LEVEL_CONNECT("1-wire bus short circuit.");
		return 1;
	} else {
		pn->selected_connection->reconnect_state++;	// Flag for eventual reconnection
		LEVEL_DEBUG("Reset error. Reconnection %d/%d",pn->selected_connection->reconnect_state,reconnect_error); 
		STAT_ADD1_BUS(e_bus_reset_errors, pn->selected_connection);
	}
	STAT_ADD1_BUS(e_bus_resets, pn->selected_connection);

	return ret;
}
