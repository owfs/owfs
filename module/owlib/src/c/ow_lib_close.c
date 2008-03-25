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

/* All ow library closeup */
void LibClose(void)
{
	LEVEL_CALL("Starting Library cleanup\n");
    LibStop() ;
    PIDstop();
    DeviceDestroy();

#if OW_MT
	if (Mutex.pmattr) {
		pthread_mutexattr_destroy(Mutex.pmattr);
		Mutex.pmattr = NULL;
	}
#endif							/* OW_MT */

	if (Global.announce_name) {
        free(Global.announce_name);
        Global.announce_name = NULL ;
    }

    if (Global.progname) {
		free(Global.progname);
        Global.progname = NULL ;
    }

#if OW_ZERO
	if (Global.browse && (libdnssd != NULL)) {
		DNSServiceRefDeallocate(Global.browse);
    }
    
	OW_Free_dnssd_library();
#endif
	LEVEL_CALL("Finished Library cleanup\n");
	if (log_available) {
		closelog();
        log_available = 0 ;
    }
}
