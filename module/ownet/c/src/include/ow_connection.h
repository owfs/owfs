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

#ifndef OW_CONNECTION_H			/* tedious wrapper */
#define OW_CONNECTION_H

/*
Jan 21, 2007 from the IANA:
owserver        4304/tcp   One-Wire Filesystem Server
owserver        4304/udp   One-Wire Filesystem Server
#                          Paul Alfille <paul.alfille@gmail.com> January 2007

See: http://www.iana.org/assignments/port-numbers
*/
#define DEFAULT_PORT  "4304"

#include "ow.h"
#include <sys/ioctl.h>
#include "ownetapi.h"

/* large enough for arrays of 2048 elements of ~49 bytes each */
#define MAX_OWSERVER_PROTOCOL_PAYLOAD_SIZE  100050

/* com port fifo info */
/* The UART_FIFO_SIZE defines the amount of bytes that are written before
 * reading the reply. Any positive value should work and 16 is probably low
 * enough to avoid losing bytes in even most extreme situations on all modern
 * UARTs, and values bigger than that will probably work on almost any
 * system... The value affects readout performance asymptotically, value of 1
 * should give equivalent performance with the old implementation.
 *   -- Jari Kirma
 *
 * Note: Each bit sent to the 1-wire net requeires 1 byte to be sent to
 *       the uart.
 */
#define UART_FIFO_SIZE 160

/** USB bulk endpoint FIFO size
  Need one for each for read and write
  This is for alt setting "3" -- 64 bytes, 1msec polling
*/
#define USB_FIFO_EACH 64
#define USB_FIFO_READ 0
#define USB_FIFO_WRITE USB_FIFO_EACH
#define USB_FIFO_SIZE ( USB_FIFO_EACH + USB_FIFO_EACH )
#define HA7_FIFO_SIZE 32000
#define LINK_FIFO_SIZE UART_FIFO_SIZE
#define LINKE_FIFO_SIZE 1500
#define I2C_FIFO_SIZE 1

#if USB_FIFO_SIZE > UART_FIFO_SIZE
#define MAX_FIFO_SIZE USB_FIFO_SIZE
#else
#define MAX_FIFO_SIZE UART_FIFO_SIZE
#endif

struct connection_in;

/* -------------------------------------------- */
/* Interface-specific routines ---------------- */
struct interface_routines {
	/* Detect if adapter is present, and open -- usually called outside of this routine */
	int (*detect) (struct connection_in * in);
	/* reset the interface -- actually the 1-wire bus */
	void (*close) (struct connection_in * in);
	/* capabilities flags */
	UINT flags;
};
#define BUS_detect(in)                      (((in)->iroutines.detect(in)))
#define BUS_close(in)                       (((in)->iroutines.close(in)))

#if OW_MT
#else							/* OW_MT */
#endif							/* OW_MT */

struct connin_tcp {
	char *host;
	char *service;
	struct addrinfo *ai;
	struct addrinfo *ai_ok;
	char *type;					// for zeroconf
	char *domain;				// for zeroconf
	char *fqdn;					// fully qualified domain name
	int no_dirall;				// flag that server doesn't support DIRALL
};

//enum server_type { srv_unknown, srv_direct, srv_client, src_
/* Network connection structure */
enum bus_mode {
	bus_unknown = 0,
	bus_server,
	bus_zero,
};

enum connection_state {
	connection_vacant,
	connection_pending,
	connection_active,
};

enum adapter_type {
	adapter_tcp = 9,
};

enum e_reconnect {
	reconnect_bad = -1,
	reconnect_ok = 0,
	reconnect_error = 5,
};

struct device_search {
	int LastDiscrepancy;		// for search
	int LastDevice;				// for search
	int index;
	BYTE sn[8];
	BYTE search;
};

struct connection_in {
	struct connection_in *next;
	int index;
	enum connection_state state;
	char *name;
	int file_descriptor;
#if OW_MT
	pthread_mutex_t bus_mutex;
#endif							/* OW_MT */

	enum bus_mode busmode;
	struct interface_routines iroutines;
	enum adapter_type Adapter;
	char *adapter_name;

	/* Static buffer for conmmunication */
	/* Since only used during actual transfer to/from the adapter,
	   should be protected from contention even when multithreading allowed */
	size_t bundling_length;
	union {
		struct connin_tcp tcp;
	} connin;
};

extern struct connection_in *head_inbound_list;

#define  FD_PERSISTENT_IN_USE    -2
#define  FD_PERSISTENT_NONE      -1
#define  FD_CURRENT_BAD          -1
/* This bug-fix/workaround function seem to be fixed now... At least on
 * the platforms I have tested it on... printf() in owserver/src/c/owserver.c
 * returned very strange result on c->busmode before... but not anymore */
enum bus_mode get_busmode(struct connection_in *c);
int BusIsServer(struct connection_in *in);

void FreeIn(struct connection_in *target);
void FreeInAll(void);

struct connection_in *NewIn(void);
struct connection_in *find_connection_in(OWNET_HANDLE handle);

/* Bonjour registration */
void OW_Browse(void);

/* Low-level functions
    slowly being abstracted and separated from individual
    interface type details
*/
int Server_detect(struct connection_in *in);
int Zero_detect(struct connection_in *in);
int BadAdapter_detect(struct connection_in *in);

#endif							/* OW_CONNECTION_H */
