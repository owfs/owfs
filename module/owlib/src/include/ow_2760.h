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

#ifndef OW_2760_H
#define OW_2760_H

#ifndef OWFS_CONFIG_H
#error Please make sure owfs_config.h is included *before* this header file
#endif
#include "ow_standard.h"

/* ------- Structures ----------- */

DeviceHeader(DS2720);
DeviceHeader(DS2740);
DeviceHeader(DS2751);
DeviceHeader(DS2755);
DeviceHeader(DS2756);
DeviceHeader(DS2760);
DeviceHeader(DS2770);
DeviceHeader(DS2780);
DeviceHeader(DS2781);

#endif							/* OW_2760_H */
