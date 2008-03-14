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

static int primative_lib_setup_lock = 0 ;

/* All ow library setup */
void LibSetup(enum opt_program opt)
{
	if ( primative_lib_setup_lock++ ) {
		return ;
	}
#if OW_ZERO
	OW_Load_dnssd_library();
#endif

	Global.opt = opt;

	/* special resort in case static data (devices and filetypes) not properly sorted */
	DeviceSort();

#if OW_CACHE
	Cache_Open();
#endif							/* OW_CACHE */

#ifndef __UCLIBC__
	/* Setup the multithreading synchronizing locks */
	LockSetup();
#endif							/* __UCLIBC__ */

	start_time = time(NULL);
	errno = 0;					/* set error level none */
}
