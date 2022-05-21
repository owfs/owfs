/*
    OWFS -- One-Wire filesystem
    OWHTTPD -- One-Wire Web Server
    Written 2003 Paul H Alfille
	email: paul.alfille@gmail.com
	Released under the GPL
	See the header file: ow.h for full attribution
	1wire/iButton system from Dallas Semiconductor
*/

#ifndef OW_STATS_H
#define OW_STATS_H

#ifndef OWFS_CONFIG_H
#error Please make sure owfs_config.h is included *before* this header file
#endif
#include "ow_standard.h"

/* -------- Structures ---------- */
DeviceHeader(stats_cache);
DeviceHeader(stats_read);
DeviceHeader(stats_write);
DeviceHeader(stats_directory);
DeviceHeader(stats_errors);
DeviceHeader(stats_thread);
DeviceHeader(stats_return_code);

#endif							/* OW_STATS */
