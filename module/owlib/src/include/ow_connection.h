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
#define DEFAULT_SERVER_PORT       "4304"
#define DEFAULT_FTP_PORT          "21"
#define DEFAULT_HA7_PORT          "80"
#define DEFAULT_ENET_PORT         "8080"
#define DEFAULT_LINK_PORT         "10001"
#define DEFAULT_XPORT_CONTROL_PORT "9999"
#define DEFAULT_XPORT_PORT        "10001"
#define DEFAULT_ETHERWEATHER_PORT "15862"

#include "ow.h"
#include "ow_counters.h"
#include <sys/ioctl.h>
#include "ow_transaction.h"

/* reset types */
#include "ow_reset.h"

/* bus serach */
#include "ow_search.h"

/* connect to a bus master */
#include "ow_detect.h"

/* address for i2c or usb */
#include "ow_parse_address.h"

/* w1 sequence defines */
#include "ow_w1_seq.h"

#if OW_USB						/* conditional inclusion of USB */
/* Special libusb 0.1x search */
#include "ow_usb_cycle.h"
#endif /* OW_USB */

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
#define HA7_FIFO_SIZE 128
#define W1_FIFO_SIZE 128
#define LINK_FIFO_SIZE UART_FIFO_SIZE
#define HA7E_FIFO_SIZE UART_FIFO_SIZE
#define HA5_FIFO_SIZE UART_FIFO_SIZE
#define LINKE_FIFO_SIZE 1500
#define I2C_FIFO_SIZE 1

#if USB_FIFO_SIZE > UART_FIFO_SIZE
#define MAX_FIFO_SIZE USB_FIFO_SIZE
#else
#define MAX_FIFO_SIZE UART_FIFO_SIZE
#endif

/* Debugging interface to showing all bus traffic
 * You need to configure compile with
 * ./configure --enable-owtraffic
 * */
#include "ow_traffic.h"

/* -------------------------------------------- */
/* Interface-specific routines ---------------- */
#include "ow_bus_routines.h"

/* -------------------------------------------- */
/* Inbound connections (bus masters) ---------- */
#include "ow_port_in.h"
#include "ow_connection_in.h"

/* -------------------------------------------- */
/* Outbound connections (ownet clients) ---------- */
#include "ow_connection_out.h"

/* This bug-fix/workaround function seem to be fixed now... At least on
 * the platforms I have tested it on... printf() in owserver/src/c/owserver.c
 * returned very strange result on c->busmode before... but not anymore */
int BusIsServer(struct connection_in *in);

// mode bit flags for level
#define MODE_NORMAL                    0x00
#define MODE_STRONG5                   0x01
#define MODE_PROGRAM                   0x02
#define MODE_BREAK                     0x04

// 1Wire Bus Speed Setting Constants
#define ONEWIREBUSSPEED_REGULAR        0x00
#define ONEWIREBUSSPEED_FLEXIBLE       0x01	/* Only used for USB adapter */
#define ONEWIREBUSSPEED_OVERDRIVE      0x02

/* Serial port */
GOOD_OR_BAD COM_open(struct connection_in *in);
GOOD_OR_BAD serial_open(struct connection_in *in);
GOOD_OR_BAD tcp_open(struct connection_in *in);

GOOD_OR_BAD COM_test( struct connection_in * connection );

void COM_set_standard( struct connection_in *connection) ;

void COM_close(struct connection_in *in);

void COM_free(struct connection_in *in);
void serial_free(struct connection_in *in);
void tcp_free(struct connection_in *in);

void COM_flush( const struct connection_in *in);
void COM_break(struct connection_in *in);
GOOD_OR_BAD telnet_break(struct connection_in *in) ;

GOOD_OR_BAD COM_change( struct connection_in *connection) ;
GOOD_OR_BAD serial_change(struct connection_in *connection) ;
GOOD_OR_BAD telnet_change(struct connection_in *in) ;

GOOD_OR_BAD COM_write( const BYTE * data, size_t length, struct connection_in *connection);
GOOD_OR_BAD COM_write_simple( const BYTE * data, size_t length, struct connection_in *connection);
GOOD_OR_BAD telnet_write_binary( const BYTE * buf, const size_t size, struct connection_in *in);

GOOD_OR_BAD COM_read( BYTE * data, size_t length, struct connection_in *connection);
SIZE_OR_ERROR COM_read_with_timeout( BYTE * data, size_t length, struct connection_in *connection ) ;

void COM_slurp( struct connection_in *in);
GOOD_OR_BAD telnet_purge(struct connection_in *in) ;

GOOD_OR_BAD telnet_read(BYTE * buf, const size_t size, struct connection_in *in) ;


void FreeInAll(void);
void RemoveIn( struct connection_in * conn ) ;

void FreeOutAll(void);
void DelIn(struct connection_in *in);

struct connection_in *LinkIn(struct connection_in *in, struct port_in * head);

void Add_InFlight( GOOD_OR_BAD (*nomatch)(struct connection_in * trial,struct connection_in * existing), struct port_in * new_pin );
void Del_InFlight( GOOD_OR_BAD (*nomatch)(struct connection_in * trial,struct connection_in * existing), struct port_in * new_pin );

struct connection_in *find_connection_in(int nr);
int SetKnownBus( int bus_number, struct parsedname * pn) ;

struct connection_out *NewOut(void);

/* Bonjour registration */
void ZeroConf_Announce(struct connection_out *out);
void OW_Browse(struct connection_in *in);

GOOD_OR_BAD TestConnection(const struct parsedname *pn);

void ZeroAdd(const char * name, const char * type, const char * domain, const char * host, const char * service) ;
void ZeroDel(const char * name, const char * type, const char * domain ) ;

#define STAT_ADD1_BUS( err, in )     STATLOCK; ++((in)->bus_stat[err]) ; STATUNLOCK

#endif							/* OW_CONNECTION_H */
