/*
$Id$
    OWFS -- One-Wire filesystem
    OWHTTPD -- One-Wire Web Server
    Written 2003 Paul H Alfille
    email: paul.alfille@gmail.com
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
	LEVEL_CALL("Starting Library cleanup");
	LibStop();
	PIDstop();
	DeviceDestroy();
	Detail_Close() ;

	_MUTEX_ATTR_DESTROY(Mutex.mattr);

#if OW_ZERO
	// Used by browse and announce
	OW_Free_dnssd_library();
	OW_Free_avahi_library();
#endif

#if OW_USB
	if ( Globals.luc != NULL ) {
		libusb_exit( Globals.luc ) ;
		Globals.luc = NULL ;
	}
#endif /* OW_USB */

	LEVEL_CALL("Finished Library cleanup");
	if (log_available) {
		closelog();
		log_available = 0;
	}

	SAFEFREE(Globals.announce_name) ;
	SAFEFREE(Globals.progname) ;
	SAFEFREE(Globals.fatal_debug_file) ;
	LEVEL_DEBUG("Libraries closed");
}
