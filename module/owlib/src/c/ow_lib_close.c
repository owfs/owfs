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
	if (Mutex.pmattr) {
		my_pthread_mutexattr_destroy(Mutex.pmattr);
		Mutex.pmattr = NULL;
	}
#endif							/* OW_MT */

	if (Globals.announce_name) {
		owfree(Globals.announce_name);
		Globals.announce_name = NULL;
	}

	if (Globals.progname) {
		owfree(Globals.progname);
		Globals.progname = NULL;
	}
#if OW_ZERO
	if (Globals.browse && (libdnssd != NULL)) {
		DNSServiceRefDeallocate(Globals.browse);
	}

	OW_Free_dnssd_library();
	OW_Free_avahi_library();
#endif

	if (Globals.fatal_debug_file) {
		owfree(Globals.fatal_debug_file);
		Globals.fatal_debug_file = NULL;
	}

	LEVEL_CALL("Finished Library cleanup");
	if (log_available) {
		closelog();
		log_available = 0;
	}
}
