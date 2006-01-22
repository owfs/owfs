/*
$Id$
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

#ifndef OW_COUNTERS  /* tedious wrapper */
#define OW_COUNTERS

/* ----------------- */
/* -- Statistics --- */
/* ----------------- */
struct average {
    unsigned int max ;
    unsigned int sum ;
    unsigned int count ;
    unsigned int current ;
} ;

struct cache {
    unsigned int tries ;
    unsigned int hits ;
    unsigned int adds ;
    unsigned int expires ;
    unsigned int deletes ;
} ;

struct directory {
    unsigned int calls ;
    unsigned int entries ;
} ;

#define AVERAGE_IN(pA)  ++(pA)->current; ++(pA)->count; (pA)->sum+=(pA)->current; if ((pA)->current>(pA)->max)++(pA)->max;
#define AVERAGE_OUT(pA) --(pA)->current;
#define AVERAGE_MARK(pA)  ++(pA)->count; (pA)->sum+=(pA)->current;
#define AVERAGE_CLEAR(pA)  (pA)->current=0;

extern unsigned int cache_flips ;
extern unsigned int cache_adds ;
extern struct average new_avg ;
extern struct average old_avg ;
extern struct average store_avg ;
extern struct cache cache_ext ;
extern struct cache cache_int ;
extern struct cache cache_dir ;
extern struct cache cache_dev ;
extern struct cache cache_sto ;

extern unsigned int read_calls ;
extern unsigned int read_cache ;
extern unsigned int read_cachebytes ;
extern unsigned int read_bytes ;
extern unsigned int read_array ;
extern unsigned int read_tries[3] ;
extern unsigned int read_success ;
extern struct average read_avg ;

extern unsigned int write_calls ;
extern unsigned int write_bytes ;
extern unsigned int write_array ;
extern unsigned int write_tries[3] ;
extern unsigned int write_success ;
extern struct average write_avg ;

extern struct directory dir_main ;
extern struct directory dir_dev ;
extern unsigned int dir_depth ;
extern struct average dir_avg ;

extern struct average all_avg ;

extern struct timeval max_delay ;

// ow_locks.c
extern struct timeval total_bus_time ;  // total bus_time
//extern struct timeval bus_pause ; 
extern unsigned int total_bus_locks ;   // total number of locks
extern unsigned int total_bus_unlocks ; // total number of unlocks

// ow_crc.c
extern unsigned int CRC8_tries ;
extern unsigned int CRC8_errors ;
extern unsigned int CRC16_tries ;
extern unsigned int CRC16_errors ;

// ow_net.c
extern unsigned int NET_accept_errors ;
extern unsigned int NET_connection_errors ;
extern unsigned int NET_read_errors ;

// ow_bus.c
extern unsigned int BUS_reconnect ;         // sum from all adapters
extern unsigned int BUS_reconnect_errors ;  // sum from all adapters
extern unsigned int BUS_send_data_errors ;
extern unsigned int BUS_send_data_memcmp_errors ;
extern unsigned int BUS_readin_data_errors ;
extern unsigned int BUS_select_low_errors ;
extern unsigned int BUS_select_low_branch_errors ;
extern unsigned int BUS_level_errors ;
extern unsigned int BUS_write_errors ;
extern unsigned int BUS_write_interrupt_errors ;
extern unsigned int BUS_read_errors ;
extern unsigned int BUS_read_interrupt_errors ;
extern unsigned int BUS_read_select_errors ;
extern unsigned int BUS_read_timeout_errors ;
extern unsigned int BUS_next_errors ;
extern unsigned int BUS_next_alarm_errors ;
extern unsigned int BUS_detect_errors ;
extern unsigned int BUS_open_errors ;
extern unsigned int BUS_PowerByte_errors ;
extern unsigned int BUS_reset_errors ;
extern unsigned int BUS_short_errors ;

// ow_ds9490.c
extern unsigned int DS9490_wait_errors ;
extern unsigned int DS9490_sendback_data_errors ;

// ow_ds9097.c
extern unsigned int DS9097_read_bits_errors ;
extern unsigned int DS9097_sendback_data_errors ;
extern unsigned int DS9097_sendback_bits_errors ;
extern unsigned int DS9097_reset_tcsetattr_errors ;

// ow_ds1410.c
extern unsigned int DS1410_read_bits_errors ;
extern unsigned int DS1410_sendback_data_errors ;
extern unsigned int DS1410_sendback_bits_errors ;
extern unsigned int DS1410_reset_tcsetattr_errors ;

// ow_ds9097U.c
extern unsigned int DS2480_send_cmd_errors ;
extern unsigned int DS2480_send_cmd_memcmp_errors ;
extern unsigned int DS2480_sendout_data_errors ;
extern unsigned int DS2480_sendout_cmd_errors ;
extern unsigned int DS2480_sendback_data_errors ;
extern unsigned int DS2480_sendback_cmd_errors ;
extern unsigned int DS2480_write_interrupted ;
extern unsigned int DS2480_read_fd_isset ;
extern unsigned int DS2480_read_null ;
extern unsigned int DS2480_read_read ;
extern unsigned int DS2480_level_docheck_errors ;
extern unsigned int DS2480_databit_errors ;
extern unsigned int DS2480_ProgramPulse_errors ;

#define STAT_ADD1(x)    STATLOCK ; ++x ; STATUNLOCK

#endif /* OW_COUNTERS */
