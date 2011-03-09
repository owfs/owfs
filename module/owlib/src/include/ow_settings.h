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

#ifndef OW_SETTINGS_H
#define OW_SETTINGS_H

#ifndef OWFS_CONFIG_H
#error Please make sure owfs_config.h is included *before* this header file
#endif
#include "ow_standard.h"

/* -------- Structures ---------- */
DeviceHeader(set_timeout);
DeviceHeader(set_units);
DeviceHeader(set_alias);

#endif							/* OW_SETTINGS_H */
