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

#ifndef OW_2401_H
#define OW_2401_H

#ifndef OWFS_CONFIG_H
#error Please make sure owfs_config.h is included *before* this header file
#endif
#include "ow_standard.h"

/* DS2401 Simple ID device */
/* Also needed for all others */

/* ------- Prototypes ----------- */

	/* All from ow_xxxx.c file */

/* ------- Structures ----------- */

DeviceHeader(DS2401);
DeviceHeader(DS1420);

#endif
