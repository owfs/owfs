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

#ifndef OW_LINK_H
#define OW_LINK_H

#ifndef OWFS_CONFIG_H
#error Please make sure owfs_config.h is included *before* this header file
#endif
#include "ow_xxxx.h"
#include "ow_none.h"

/* ------- Structures ----------- */

DeviceHeader( DS9097 )
DeviceHeader( DS9097U )
DeviceHeader( iButtonLink )
DeviceHeader( DS9490 )

#endif


