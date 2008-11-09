/*
$Id$
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

/* CAnn stand alone -- separated out of ow.h for clarity */

#ifndef OW_GLOBAL_H				/* tedious wrapper */
#define OW_GLOBAL_H

enum zero_support { zero_unknown, zero_none, zero_bonjour, zero_avahi, } ;

/* Globals information (for local control) */
struct global {
	int announce_off;			// use zeroconf?
	ASCII *announce_name;
#if OW_ZERO
	DNSServiceRef browse;
#endif
	enum opt_program opt;
	ASCII *progname;
	union antiloop Token;
	int want_background;
	int now_background;
	int error_level;
	int error_print;
	int readonly;
	int max_clients;			// for ftp
	int autoserver;
	size_t cache_size;			// max cache size (or 0 for no max) ;
	int one_device;				// Single device, use faster ROM comands
	/* Special parameter to trigger William Robison <ibutton@n952.dyndns.ws> timings */
	int altUSB;
	int usb_flextime;
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
	int timeout_persistent_low;
	int timeout_persistent_high;
	int clients_persistent_low;
	int clients_persistent_high;
	int pingcrazy;
	int no_dirall;
	int no_get;
	int no_persistence;
	int eightbit_serial;
	enum zero_support zero ;
};
extern struct global Globals;

#endif							/* OW_GLOBAL_H */
