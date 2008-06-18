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

/* All ow library setup */
void LibSetup(enum opt_program opt)
{
	/* Setup the multithreading synchronizing locks */
	LockSetup();

#if OW_ZERO
	OW_Load_dnssd_library();
#endif

	Globals.opt = opt;

	/* special resort in case static data (devices and filetypes) not properly sorted */
	DeviceSort();

#if OW_CACHE
	Cache_Open();
#endif							/* OW_CACHE */

	StateInfo.start_time = time(NULL);
	errno = 0;					/* set error level none */
}
