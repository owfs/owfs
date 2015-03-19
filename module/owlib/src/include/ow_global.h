/*
    OW -- One-Wire filesystem
    version 0.4 7/2/2003

     Written 2003 Paul H Alfille
        Fuse code based on "fusexmp" {GPL} by Miklos Szeredi, mszeredi@inf.bme.hu
        Serial code based on "xt" {GPL} by David Querbach, www.realtime.bc.ca
        in turn based on "miniterm" by Sven Goldt, goldt@math.tu.berlin.de
    GPL license
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License
    as published by the Free Software Foundation; either version 2
    of the License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    Other portions based on Dallas Semiconductor Public Domain Kit,
    ---------------------------------------------------------------------------
    Copyright (C) 2000 Dallas Semiconductor Corporation, All Rights Reserved.
        Permission is hereby granted, free of charge, to any person obtaining a
        copy of this software and associated documentation files (the "Software"),
        to deal in the Software without restriction, including without limitation
        the rights to use, copy, modify, merge, publish, distribute, sublicense,
        and/or sell copies of the Software, and to permit persons to whom the
        Software is furnished to do so, subject to the following conditions:
        The above copyright notice and this permission notice shall be included
        in all copies or substantial portions of the Software.
    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
    OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
    MERCHANTABILITY,  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
    IN NO EVENT SHALL DALLAS SEMICONDUCTOR BE LIABLE FOR ANY CLAIM, DAMAGES
    OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
    ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
    OTHER DEALINGS IN THE SOFTWARE.
        Except as contained in this notice, the name of Dallas Semiconductor
        shall not be used except as stated in the Dallas Semiconductor
        Branding Policy.
    ---------------------------------------------------------------------------
    Implementation:
    25-05-2003 iButtonLink device
*/

/* Can stand alone -- separated out of ow.h for clarity */

#ifndef OW_GLOBAL_H				/* tedious wrapper */
#define OW_GLOBAL_H

#include "ow_temperature.h"
#include "ow_pressure.h"

// some improbably sub-absolute-zero number
#define GLOBAL_UNTOUCHED_TEMP_LIMIT	(-999.)

#define DEFAULT_USB_SCAN_INTERVAL 10 /* seconds */
#define DEFAULT_ENET_SCAN_INTERVAL 60 /* seconds */
#define DEFAULT_MASTERHUB_SCAN_INTERVAL 60 /* seconds */

enum zero_support { zero_unknown, zero_none, zero_bonjour, zero_avahi, } ;

enum enum_program_type { 
	program_type_filesystem, program_type_server, program_type_httpd, program_type_ftpd, program_type_external, 
	program_type_tcl, program_type_swig, program_type_clibrary, 
	program_option_sensor, program_option_property,
};

/* Daemon status
 * can be running as foreground or background
 * this is for a state machine implementation
 * sd is systemd which is foreground
 * sd cannot be changed
 * want_bg is default state
 * goes to bg if proper program type and can daemonize
 * */
enum enum_daemon_status {
	e_daemon_want_bg, e_daemon_bg, e_daemon_sd, e_daemon_sd_done, e_daemon_fg, e_daemon_unknown,
} ;

/* Globals information (for local control) */
struct global {
	int announce_off;			// use zeroconf?
	ASCII *announce_name;
	enum temp_type temp_scale;
	enum pressure_type pressure_scale ;
	enum deviceformat format ;
	enum enum_program_type program_type;
	enum enum_daemon_status daemon_status ;
	int allow_external ; // allow this program to call external programs for read/write -- dangerous
	int allow_other ;
	struct antiloop Token;
	int uncached ; // all requests are from /uncached directory
	int unaliased ; // all requests are from /unaliased (no alias substitution on results)
	int error_level;
	int error_level_restore;
	int error_print;
	int fatal_debug;
	ASCII *fatal_debug_file;
	int readonly;
	int max_clients;			// for ftp
	size_t cache_size;			// max cache size (or 0 for no max) ;
	int one_device;				// Single device, use faster ROM comands
	/* Special parameter to trigger William Robison <ibutton@n952.dyndns.ws> timings */
	int altUSB;
	int usb_flextime;
	int serial_flextime;
	int serial_reverse; // reverse polarity ?
	int serial_hardflow ; // hardware flow control
	/* timeouts -- order must match ow_opt.c values for correct indexing */
	int timeout_volatile;
	int timeout_stable;
	int timeout_directory;
	int timeout_presence;
	int timeout_serial; // serial read and write use the same timeout currently
	int timeout_usb;
	int timeout_network;
	int timeout_server;
	int timeout_ftp;
	int timeout_ha7;
	int timeout_w1;
	int timeout_persistent_low;
	int timeout_persistent_high;
	int clients_persistent_low;
	int clients_persistent_high;
	int pingcrazy;
	int no_dirall;
	int no_get;
	int no_persistence;
	int eightbit_serial;
	int trim;
	enum zero_support zero ;
	int i2c_APU ;
	int i2c_PPM ;
	int baud ;
	int traffic ; // show bus traffic
	int locks ; // show mutexes
	_FLOAT templow ;
	_FLOAT temphigh ;
#if OW_USB
	libusb_context * luc ;
#endif /* OW_USB */
	int argc;
	char ** argv ;
	enum e_inet_type inet_type ;
	enum { exit_early, exit_normal, exit_exec, } exitmode ; // is this an execpe on config file change?
	int restart_seconds ; // time after close before execve
};
extern struct global Globals;

// generic value for ignorable function returns 
extern int ignore_result ;

void SetLocalControlFlags( void ) ;


#endif							/* OW_GLOBAL_H */
