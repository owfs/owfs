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

/* Tests whether this bus (pn->in) has too many consecutive reset errors */
/* If so, the bus is closed and "reconnected" */
/* Reconnection usually just means reopening (detect) with the same initial name like ttyS0 */
/* USB is a special case, in gets reenumerated, so we look for similar DS2401 chip */
int TestConnection(const struct parsedname *pn)
{
    int ret = 0;
    struct connection_in *in;

    // Test without a lock -- efficient
    if (pn == NULL || pn->in == NULL
	|| pn->in->reconnect_state < reconnect_error)
	return 0;
    CONNINLOCK;
    in = indevice;
    CONNINUNLOCK;
    // Lock the bus
    BUSLOCK(pn);
    // Test again
    if (pn->in->reconnect_state >= reconnect_error) {
	// Add Statistics
	STAT_ADD1(BUS_reconnects);
	STAT_ADD1(pn->in->bus_reconnect);

	if (pn->in == in)
	    Global.SimpleBusName = "Reconnecting...";

	// Close the bus (should leave enough reconnection information available)
	BUS_close(pn->in);	// already locked

	// Call reconnection
	if ((pn->in->iroutines.reconnect)
	    ? (pn->in->iroutines.reconnect) (pn)	// call bus-specific reconnect
	    : BUS_detect(pn->in)	// call initial opener
	    ) {
	    STAT_ADD1(BUS_reconnect_errors);
	    STAT_ADD1(pn->in->bus_reconnect_errors);
	    LEVEL_DEFAULT("Failed to reconnect %s adapter!\n",
			  pn->in->adapter_name);
	    pn->in->reconnect_state = reconnect_ok + 1;
	    // delay to slow thrashing
	    UT_delay(200);
	    ret = -EIO;
	} else {
	    LEVEL_DEFAULT("%s adapter reconnected\n",
			  pn->in->adapter_name);
	    pn->in->reconnect_state = reconnect_ok;
	    if (pn->in == in)
		Global.SimpleBusName = in->name;
	}
    }
    BUSUNLOCK(pn);

    return ret;
}
