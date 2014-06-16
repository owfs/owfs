/*
$Id$
    OW -- One-Wire filesystem
    version 0.4 7/2/2003

    Function naming scheme:
    OW -- Generic call to interaface
    LI -- LINK commands
    L1 -- 2480B commands
    FS -- filesystem commands
    UT -- utility functions

    LICENSE (As of version 2.5p4 2-Oct-2006)
    owlib: GPL v2
    owfs, owhttpd, owftpd, owserver: GPL v2
    owshell(owdir owread owwrite owpresent): GPL v2
    owcapi (libowcapi): GPL v2
    owperl: GPL v2
    owtcl: LGPL v2
    owphp: GPL v2
    owpython: GPL v2
    owsim.tcl: GPL v2
    where GPL v2 is the "Gnu General License version 2"
    and "LGPL v2" is the "Lesser Gnu General License version 2"


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
*/

/* Cannot stand alone -- part of ow.h but separated for clarity */

#ifndef OW_OPT_H				/* tedious wrapper */
#define OW_OPT_H

/* command line options */
/* These are the owlib-specific options */
#define OWLIB_OPT "a:m:c:f:p:s:h::u::d:t:CFRKVP:rw:"
extern const struct option owopts_long[];

GOOD_OR_BAD owopt(const int c, const char *arg);
GOOD_OR_BAD owopt_packed(const char *params);

// All these command line arguments are after the printable ascii characters
enum e_long_option { e_error_print = 257, e_error_level, e_debug,
	e_cache_size,
	e_fuse_opt, e_fuse_open_opt,
	e_max_clients,
	e_safemode,
	e_ha7, e_fake, e_link, e_ha3, e_ha4b, e_ha5, e_ha7e, e_tester, e_mock, e_etherweather, e_passive, e_i2c, e_xport, 
	e_enet, e_pbm,
	e_want_background, e_want_foreground,
	e_w1_monitor, e_usb_monitor, e_browse,
	e_pressure_mbar, e_pressure_atm, e_pressure_mmhg, e_pressure_inhg, e_pressure_psi, e_pressure_Pa, e_pressure_6, e_pressure_7,
	e_announce,
	e_timeout_volatile, e_timeout_stable, e_timeout_directory, e_timeout_presence,
	e_timeout_serial, e_timeout_usb, e_timeout_network, e_timeout_server, e_timeout_ftp, e_timeout_ha7, e_timeout_w1,
	e_timeout_persistent_low, e_timeout_persistent_high, e_clients_persistent_low, e_clients_persistent_high,
	e_concurrent_connections,
	e_fatal_debug_file,
	e_baud,
	e_templow, e_temphigh,
	e_detail,
};

#endif							/* OW_OPT_H */
