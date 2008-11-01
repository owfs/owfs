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

/* Globals for port and bus communication */
/* connections globals stored in ow_connect.c */
/* i.e. connection_in * head_inbound_list ...         */

/* char * pid_file in ow_opt.c */

/* State information, sent to remote or kept locally */
/* cacheenabled, presencecheck, tempscale, devform */
#if OW_CACHE
int32_t SemiGlobal = ((uint8_t) fdi) << 24 | ((uint8_t) temp_celsius) << 16 | ((uint8_t) 1)
	<< 8 | ((uint8_t) 1);
#else
int32_t SemiGlobal = ((uint8_t) fdi) << 24 | ((uint8_t) temp_celsius) << 16 | ((uint8_t) 1)
	<< 8;
#endif

// This structure is defined in ow_global.h
// These are all the tunable constants and flags. Usually they have
// 1 A command line entry (ow_opt.h)
// 2 A help entry (ow_help.c)
// An entry in /settings (ow_settings.c)
struct global Globals = {
	.announce_off = 0,
	.announce_name = NULL,
#if OW_ZERO
	.browse = NULL,
#endif

	.opt = opt_swig,
	.progname = NULL,			// "One Wire File System" , Can't allocate here since it's freed
	.want_background = 1,
	.now_background = 0,

	.error_level = e_err_default,
	.error_print = e_err_print_mixed,

	.readonly = 0,
	.SimpleBusName = "None",
	.max_clients = 250,
	.autoserver = 0,

	.cache_size = 0,

	.one_device = 0,

	.altUSB = 0,
	.usb_flextime = 1,

	.timeout_volatile = 15,
	.timeout_stable = 300,
	.timeout_directory = 60,
	.timeout_presence = 120,
	.timeout_serial = 5, // serial read and write use the same timeout currently
	.timeout_usb = 5,			// 5 seconds
	.timeout_network = 1,
	.timeout_server = 10,
	.timeout_ftp = 900,
	.timeout_ha7 = 60,
	.timeout_persistent_low = 600,
	.timeout_persistent_high = 3600,
	.clients_persistent_low = 10,
	.clients_persistent_high = 20,

	.pingcrazy = 0,
	.no_dirall = 0,
	.no_get = 0,
	.no_persistence = 0,
	.eightbit_serial = 0,
	.zero = zero_unknown ,
};

/* Statistics globals are stored in ow_stats.c */
