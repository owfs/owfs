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

/* Tests whether this bus (pn->selected_connection) has too many consecutive reset errors */
/* If so, the bus is closed and "reconnected" */
/* Reconnection usually just means reopening (detect) with the same initial name like ttyS0 */
/* USB is a special case, in gets reenumerated, so we look for similar DS2401 chip */
int TestConnection(const struct parsedname *pn)
{
	int ret = 0;
	struct connection_in *selected_connection = pn->selected_connection;

	// Test without a lock -- efficient
    if (pn == NULL || selected_connection == NULL || selected_connection->reconnect_state < reconnect_error) {
		return 0;
    }
	// Lock the bus
	BUSLOCK(pn);
	// Test again
	if (selected_connection->reconnect_state >= reconnect_error) {
		// Add Statistics
		STAT_ADD1_BUS(e_bus_reconnects, selected_connection);

        if (selected_connection->index == 0) {
			Global.SimpleBusName = "Reconnecting...";
        }

		// Close the bus (should leave enough reconnection information available)
		BUS_close(selected_connection);	// already locked

		// Call reconnection
		if ((selected_connection->iroutines.reconnect)
			? (selected_connection->iroutines.reconnect) (pn)	// call bus-specific reconnect
			: BUS_detect(selected_connection)	// call initial opener
			) {
			STAT_ADD1_BUS(e_bus_reconnect_errors, selected_connection);
			LEVEL_DEFAULT("Failed to reconnect %s adapter!\n", selected_connection->adapter_name);
			selected_connection->reconnect_state = reconnect_ok + 1;
			// delay to slow thrashing
			UT_delay(200);
			ret = -EIO;
		} else {
			LEVEL_DEFAULT("%s adapter reconnected\n", selected_connection->adapter_name);
			selected_connection->reconnect_state = reconnect_ok;
            if (selected_connection->index == 0) {
                Global.SimpleBusName = selected_connection->name;
            }
		}
	}
	BUSUNLOCK(pn);

	return ret;
}
