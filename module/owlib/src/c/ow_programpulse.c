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

int BUS_ProgramPulse(const struct parsedname *pn)
{
	int ret;

	if (pn->selected_connection->iroutines.ProgramPulse) {
		ret = (pn->selected_connection->iroutines.ProgramPulse) (pn);
	} else {
		ret = -ENOTSUP;
	}
	if (ret) {
		STAT_ADD1_BUS(e_bus_program_errors, pn->selected_connection);
	}
	return ret;
}
