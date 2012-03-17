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
/* i.e. connection_in * Inbound_Control.head ...         */

/* char * pid_file in ow_opt.c */

// some improbably sub-absolute-zero number
#define GLOBAL_UNTOUCHED_TEMP_LIMIT	(-999.)

/* State information, sent to remote or kept locally */
/* cacheenabled, presencecheck, tempscale, devform */
int32_t LocalControlFlags ;

// This structure is defined in ow_global.h
// These are all the tunable constants and flags. Usually they have
// 1 A command line entry (ow_opt.h)
// 2 A help entry (ow_help.c)
// An entry in /settings (ow_settings.c)
struct global Globals = {
	.announce_off = 0,
	.announce_name = NULL,
	.program_type = program_type_swig,
	.progname = NULL,			// "One Wire File System" , Can't allocate here since it's freed
	.allow_other = 0 , // for fuse

	.temp_scale = temp_celsius,
	.pressure_scale = pressure_mbar,
	.format = fdi,

	.uncached = 0,
	.unaliased = 0,
	
	.want_background = 1,
	.now_background = 0,

	.error_level = e_err_default,
	.error_print = e_err_print_mixed,
	.fatal_debug = 0,
	.fatal_debug_file = NULL,

	.concurrent_connections = 10,
	.readonly = 0,
	.max_clients = 250,

	.cache_size = 0,

	.one_device = 0,

	.altUSB = 0,
	.usb_flextime = 1,
	.serial_flextime = 1,
	.serial_reverse = 0,  // 1 is "reverse" polarity
	.serial_hardflow = 0, // hardware flow control

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
	.timeout_w1 = 30,
	.timeout_persistent_low = 600,
	.timeout_persistent_high = 3600,
	.clients_persistent_low = 10,
	.clients_persistent_high = 20,

	.usb_scan_interval = DEFAULT_USB_SCAN_INTERVAL,

	.pingcrazy = 0,
	.no_dirall = 0,
	.no_get = 0,
	.no_persistence = 0,
	.eightbit_serial = 0,
	.zero = zero_unknown ,
	.i2c_APU = 1 ,
	.i2c_PPM = 1 ,
	.baud = B9600 ,

	.templow = GLOBAL_UNTOUCHED_TEMP_LIMIT,
	.temphigh = GLOBAL_UNTOUCHED_TEMP_LIMIT,
};

// generic value for ignorable function returns 
int ignore_result ;

/* Statistics globals are stored in ow_stats.c */

/* State information, sent to remote or kept locally */
/* cacheenabled, presencecheck, tempscale, devform */
void SetLocalControlFlags( void )
{
	CONTROLFLAGSLOCK;
	// Clear
	LocalControlFlags = 0 ;
	// Device format
	LocalControlFlags |= (Globals.format) << DEVFORMAT_BIT ;
	// Pressure scale
	LocalControlFlags |= (Globals.pressure_scale) << PRESSURESCALE_BIT ;
	// Temperature scale
	LocalControlFlags |= (Globals.temp_scale) << TEMPSCALE_BIT ;
	// Uncached
	LocalControlFlags |= Globals.uncached ? UNCACHED : 0 ;
	// Unaliased
	LocalControlFlags |= Globals.unaliased ? 0 : ALIAS_REQUEST ;
	// OWNet flag or Presence check
	LocalControlFlags |= OWNET ;
	CONTROLFLAGSUNLOCK;
}
