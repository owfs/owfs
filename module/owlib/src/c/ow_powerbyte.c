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
int BUS_PowerByte(BYTE data, BYTE * resp, UINT delay, const struct parsedname *pn)
{
	int ret;

	if (pn->selected_connection->iroutines.PowerByte) {
		ret = (pn->selected_connection->iroutines.PowerByte) (data, resp, delay, pn);
	} else {
		// send the packet
		ret = BUS_sendback_data(&data, resp, 1, pn);
		// delay
		UT_delay(delay);
	}
	if (ret) {
		STAT_ADD1_BUS(e_bus_pullup_errors, pn->selected_connection);
	}
	return ret;
}
