/*
    OW -- One-Wire filesystem
    version 0.4 7/2/2003

    All the error and statistics counters are declared here,
   including handling macros
*/


/*
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

#ifndef OW_COUNTERS_H			/* tedious wrapper */
#define OW_COUNTERS_H

/* ----------------- */
/* -- Statistics --- */
/* ----------------- */
struct average {
	UINT max;
	UINT sum;
	UINT count;
	UINT current;
};

struct cache_stats {
	UINT tries;
	UINT hits;
	UINT adds;
	UINT expires;
	UINT deletes;
};

struct directory {
	UINT calls;
	UINT entries;
};

#define AVERAGE_IN(pA)  ++(pA)->current; ++(pA)->count; (pA)->sum+=(pA)->current; if ((pA)->current>(pA)->max)++(pA)->max;
#define AVERAGE_OUT(pA) --(pA)->current;
#define AVERAGE_MARK(pA)  ++(pA)->count; (pA)->sum+=(pA)->current;
#define AVERAGE_CLEAR(pA)  (pA)->current=0;

extern UINT cache_flips;
extern UINT cache_adds;
extern struct average new_avg;
extern struct average old_avg;
extern struct average store_avg;
extern struct cache_stats cache_ext;
extern struct cache_stats cache_int;
extern struct cache_stats cache_dir;
extern struct cache_stats cache_dev;
extern struct cache_stats cache_pst;

extern UINT read_calls;
extern UINT read_cache;
extern UINT read_cachebytes;
extern UINT read_bytes;
extern UINT read_array;
extern UINT read_tries[3];
extern UINT read_success;
extern struct average read_avg;

extern UINT write_calls;
extern UINT write_bytes;
extern UINT write_array;
extern UINT write_tries[3];
extern UINT write_success;
extern struct average write_avg;

extern struct directory dir_main;
extern struct directory dir_dev;
extern UINT dir_depth;
extern struct average dir_avg;

extern struct average all_avg;

extern struct timeval max_delay;

// ow_locks.c
extern UINT total_bus_locks;	// total number of locks
extern UINT total_bus_unlocks;	// total number of unlocks

// ow_crc.c
extern UINT CRC8_tries;
extern UINT CRC8_errors;
extern UINT CRC16_tries;
extern UINT CRC16_errors;

// ow_net.c
extern UINT NET_accept_errors;
extern UINT NET_connection_errors;
extern UINT NET_read_errors;

// ow_bus.c
extern UINT BUS_readin_data_errors;
extern UINT BUS_level_errors;
extern UINT BUS_next_errors;
extern UINT BUS_next_alarm_errors;
extern UINT BUS_detect_errors;
extern UINT BUS_bit_errors;
extern UINT BUS_byte_errors;
extern UINT BUS_echo_errors;
extern UINT BUS_tcsetattr_errors;
extern UINT BUS_status_errors;

// ow_ds9097U.c
extern UINT DS2480_read_fd_isset;
extern UINT DS2480_read_null;
extern UINT DS2480_read_read;
extern UINT DS2480_level_docheck_errors;
extern UINT DS2480_databit_errors;

#define STAT_ADD1(x)    STATLOCK ; ++x ; STATUNLOCK

#endif							/* OW_COUNTERS_H */
