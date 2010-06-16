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
#include "ow_devices.h"
#include "ow_pid.h"
#include "ow_specialcase.h"

/* All ow library closeup */
void LibClose(void)
{
	LEVEL_CALL("Starting Library cleanup");
	LibStop();
	PIDstop();
	DeviceDestroy();
	SpecialCase_close();

#if OW_MT
	_MUTEX_ATTR_DESTROY(Mutex.mattr);
#endif							/* OW_MT */

	SAFEFREE(Globals.announce_name) ;
	SAFEFREE(Globals.progname) ;
#if OW_ZERO
	if (Globals.browse && (libdnssd != NULL)) {
		DNSServiceRefDeallocate(Globals.browse);
	}

	OW_Free_dnssd_library();
	OW_Free_avahi_library();
#endif

	SAFEFREE(Globals.fatal_debug_file) ;

	LEVEL_CALL("Finished Library cleanup");
	if (log_available) {
		closelog();
		log_available = 0;
	}
}
