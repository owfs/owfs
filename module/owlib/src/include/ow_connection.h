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
#define DEFAULT_SERVER_PORT       "4304"
#define DEFAULT_FTP_PORT          "21"
#define DEFAULT_HA7_PORT          "80"
#define DEFAULT_ENET_PORT         "80"
#define DEFAULT_LINK_PORT         "10001"
#define DEFAULT_XPORT_PORT        "10001"
#define DEFAULT_ETHERWEATHER_POST "15862"


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
#define MAX_OWSERVER_PROTOCOL_PACKET_SIZE  100050

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

// For forward references
struct connection_in;

/* Debugging interface to showing all bus traffic
 * You need to configure compile with
 * ./configure --enable-owtraffic
 * */
#include "ow_traffic.h"

/* -------------------------------------------- */
/* Interface-specific routines ---------------- */
#include "ow_bus_routines.h"

enum bus_speed {
	bus_speed_slow,
	bus_speed_overdrive,
};

enum bus_flex { bus_no_flex, bus_yes_flex };

enum ds2480b_mode { ds2480b_data_mode, ds2480b_command_mode, } ;

struct connin_tcp {
	char *host;
	char *service;
	struct addrinfo *ai;
	struct addrinfo *ai_ok;
	char *type;					// for zeroconf
	char *domain;				// for zeroconf
	char *name;					// zeroconf name
	int no_dirall;				// flag that server doesn't support DIRALL
};

struct connin_serial {
	struct connin_tcp tcp;      // mirror connin.server
	enum ds2480b_mode mode ;
	int reverse_polarity ;
};

struct connin_fake {
	int index;
	_FLOAT templow;
	_FLOAT temphigh;
};

struct connin_usb {
#if OW_USB
	struct usb_device *dev;
	struct usb_dev_handle *usb;
	int usb_bus_number;
	int usb_dev_number;
	int datasampleoffset;
	int writeonelowtime;
	int pulldownslewrate;
	int timeout;
#endif /* OW_USB */
	int specific_usb_address ; // only a specific address requested?

	/* "Name" of the device, like "8146572300000051"
	 * This is set to the first DS1420 id found on the 1-wire adapter which
	 * exists on the DS9490 adapters. If user only have a DS2490 chip, there
	 * are no such DS1420 device available. It's used to find the 1-wire adapter
	 * if it's disconnected and later reconnected again.
	 */
	BYTE ds1420_address[SERIAL_NUMBER_SIZE];
};

#define CHANGED_USB_SPEED  0x001
#define CHANGED_USB_SLEW   0x002
#define CHANGED_USB_LOW    0x004
#define CHANGED_USB_OFFSET 0x008

struct connin_i2c {
	int channels;
	int index;
	int i2c_address;
	int i2c_index ;
	BYTE configreg;
	BYTE configchip;
	/* only one per chip, the bus entries for the other 7 channels point to the first one */
#if OW_MT
	pthread_mutex_t i2c_mutex;	// second level mutex for the entire chip */
#endif							/* OW_MT */
	int current;
	struct connection_in *head;
	struct connection_in *next;
};

struct connin_ha7 {
	struct connin_tcp tcp;		// mirror connin.server
	ASCII lock[10];
	int locked;
	int found;
};

struct connin_etherweather {
	struct connin_tcp tcp;
};

struct connin_link {
	struct connin_tcp tcp;      // mirror connin.server
};

struct connin_ha7e {
	unsigned char sn[SERIAL_NUMBER_SIZE] ;       /* last address */
};

struct connin_ha5 {
	unsigned char sn[SERIAL_NUMBER_SIZE] ;       /* last address */
	int checksum ;              /* flag to use checksum byte in communication */
	char channel ;
#if OW_MT
	pthread_mutex_t lock;	// second level mutex for the entire chip */
#endif							/* OW_MT */
	struct connection_in *head;
};

struct connin_w1 {
#if OW_W1
	// bus master name kept in name
	// netlink fd kept in file_descriptor
	SEQ_OR_ERROR seq ;
	int id ; // equivalent to the number part of w1_bus_master23
	FILE_DESCRIPTOR_OR_ERROR netlink_pipe[2] ;
#endif /* OW_W1 */
};

struct connin_usb_monitor {
	FILE_DESCRIPTOR_OR_ERROR shutdown_pipe[2] ;
};

struct connin_browse {
#if OW_ZERO
	DNSServiceRef bonjour_browse;

	AvahiServiceBrowser *avahi_browser ;
	AvahiSimplePoll *avahi_poll ;
	AvahiClient *avahi_client ;
#if __HAS_IPV6__
	char avahi_host[INET6_ADDRSTRLEN+1] ;
#else
	char avahi_host[INET_ADDRSTRLEN+1] ;
#endif
	char avahi_service[10] ;
#endif
};

//enum server_type { srv_unknown, srv_direct, srv_client, src_
/* Network connection structure */
enum bus_mode {
	bus_unknown = 0,
	bus_serial,
	bus_xport,
	bus_passive,
	bus_usb,
	bus_usb_monitor,
	bus_parallel,
	bus_server,
	bus_zero,
	bus_browse,
	bus_i2c,
	bus_ha7net,
	bus_ha5,
	bus_ha7e,
	bus_enet,
	bus_fake,
	bus_tester,
	bus_mock,
	bus_link,
	bus_elink,
	bus_etherweather,
	bus_bad,
	bus_w1,
	bus_w1_monitor,
};

enum adapter_type {
	adapter_DS9097 = 0,
	adapter_DS1410 = 1,
	adapter_DS9097U2 = 2,
	adapter_DS9097U = 3,
	adapter_LINK = 7,
	adapter_DS9490 = 8,
	adapter_tcp = 9,
	adapter_Bad = 10,
	adapter_LINK_10,
	adapter_LINK_11,
	adapter_LINK_12,
	adapter_LINK_13,
	adapter_LINK_14,
	adapter_LINK_other,
	adapter_LINK_E,
	adapter_DS2482_100,
	adapter_DS2482_800,
	adapter_HA7NET,
	adapter_HA5,
	adapter_HA7E,
	adapter_ENET,
	adapter_EtherWeather,
	adapter_fake,
	adapter_tester,
	adapter_mock,
	adapter_w1,
	adapter_w1_monitor,
	adapter_browse_monitor,
	adapter_xport,
	adapter_usb_monitor,
};

enum e_reconnect {
	reconnect_bad = -1,
	reconnect_ok = 0,
	reconnect_error = 2,
};

enum e_anydevices {
	anydevices_no = 0 ,
	anydevices_yes ,
	anydevices_unknown ,
};	

enum e_bus_stat {
	e_bus_reconnects,
	e_bus_reconnect_errors,
	e_bus_locks,
	e_bus_unlocks,
	e_bus_errors,
	e_bus_resets,
	e_bus_reset_errors,
	e_bus_short_errors,
	e_bus_program_errors,
	e_bus_pullup_errors,
	e_bus_timeouts,
	e_bus_read_errors,
	e_bus_write_errors,
	e_bus_detect_errors,
	e_bus_open_errors,
	e_bus_close_errors,
	e_bus_search_errors1,
	e_bus_search_errors2,
	e_bus_search_errors3,
	e_bus_status_errors,
	e_bus_select_errors,
	e_bus_try_overdrive,
	e_bus_failed_overdrive,
	e_bus_stat_last_marker
};

// flag to pn->selected_connection->branch.sn[0] to force select from root dir
#define BUSPATH_BAD	0xFF

struct connection_in {
	struct connection_in *next;
	INDEX_OR_ERROR index;
	char *name;
	FILE_DESCRIPTOR_OR_PERSISTENT file_descriptor;
	speed_t baud; // baud rate in the form of B9600
	struct termios oldSerialTio;    /*old serial port settings */
	// For adapters that maintain dir-at-once (or dirgulp):
	struct dirblob main;        /* main directory */
	struct dirblob alarm;       /* alarm directory */
#if OW_MT
	pthread_mutex_t bus_mutex;
	pthread_mutex_t dev_mutex;
	void *dev_db;				// dev-lock tree
#endif							/* OW_MT */
	enum e_reconnect reconnect_state;
	struct timeval last_lock;	/* statistics */
	struct timeval last_unlock;

	UINT bus_stat[e_bus_stat_last_marker];

	struct timeval bus_time;

	struct timeval bus_read_time;
	struct timeval bus_write_time;	/* for statistics */

	enum bus_mode busmode;
	struct interface_routines iroutines;
	enum adapter_type Adapter;
	char *adapter_name;
	enum e_anydevices AnyDevices;
	int ExtraReset;				// DS1994/DS2404 might need an extra reset
	size_t default_discard ; // linkhub-telnet escape chars
	enum bus_speed speed;
	enum bus_flex flex ;
	int changed_bus_settings;
	int ds2404_compliance;
	int ProgramAvailable;
	size_t last_root_devs;
	struct buspath branch;		// Branch currently selected

	/* Static buffer for conmmunication */
	/* Since only used during actual transfer to/from the adapter,
	   should be protected from contention even when multithreading allowed */
	size_t bundling_length;
	union {
		struct connin_serial serial;
		struct connin_link link;
		struct connin_tcp tcp;
		struct connin_usb usb;
		struct connin_i2c i2c;
		struct connin_fake fake;
		struct connin_fake tester;
		struct connin_fake mock;
		struct connin_ha5 ha5;
		struct connin_ha7 ha7;
		struct connin_ha7e ha7e ;
		struct connin_ha7e enet ;
		struct connin_etherweather etherweather;
		struct connin_w1 w1;
		struct connin_browse browse;
		struct connin_usb_monitor usb_monitor ;
	} connin;
};

extern struct inbound_control {
	int active ; // how many "bus" entries are currently in linked list
	int next_index ; // increasing sequence number
	struct connection_in * head ; // head of a linked list of "bus" entries
#if OW_MT
	my_rwlock_t monitor_lock; // allow monitor processes
	my_rwlock_t lock; // RW lock of linked list
#endif /* OW_MT */
	int next_fake ; // count of fake buses
	int next_tester ; // count tester buses
	int next_mock ; // count mock buses
#if OW_W1
	SEQ_OR_ERROR w1_seq ; // seq number to netlink
	FILE_DESCRIPTOR_OR_ERROR w1_file_descriptor ; // w1 kernel module for netlink communication
	int w1_pid ;
	struct timeval w1_last_read ;

#if OW_MT
	pthread_mutex_t w1_mutex;	// mutex for w1 sequence number */
	pthread_mutex_t w1_read_mutex;  // mutex for w1 netlink read time
#endif /* OW_MT */
#endif /* OW_W1 */
} Inbound_Control ; // Single global struct -- see ow_connect.c

/* Network connection structure */
struct connection_out {
	struct connection_out *next;
	void (*HandlerRoutine) (FILE_DESCRIPTOR_OR_ERROR file_descriptor);
	char *name;
	char *host;
	char *service;
	int index;
	struct addrinfo *ai;
	struct addrinfo *ai_ok;
	FILE_DESCRIPTOR_OR_ERROR file_descriptor;
	struct {
		char *type;					// for zeroconf
		char *domain;				// for zeroconf
		char *name;					// zeroconf name
	} zero ;
#if OW_MT
	pthread_t tid;
#endif							/* OW_MT */
#if OW_ZERO
	DNSServiceRef sref0;
	DNSServiceRef sref1;
#endif
};

extern struct outbound_control {
	int active ; // how many "bus" entries are currently in linked list
	int next_index ; // increasing sequence number
	struct connection_out * head ; // head of a linked list of "bus" entries
} Outbound_Control ; // Single global struct -- see ow_connect.c

/* This bug-fix/workaround function seem to be fixed now... At least on
 * the platforms I have tested it on... printf() in owserver/src/c/owserver.c
 * returned very strange result on c->busmode before... but not anymore */
enum bus_mode get_busmode(struct connection_in *c);
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
void COM_speed(speed_t new_baud, struct connection_in *in);
GOOD_OR_BAD COM_open(struct connection_in *in);
void COM_flush( const struct connection_in *in);
void COM_close(struct connection_in *in);
void COM_break(struct connection_in *in);
GOOD_OR_BAD COM_write( const BYTE * data, size_t length, struct connection_in *connection);
GOOD_OR_BAD COM_read( BYTE * data, size_t length, struct connection_in *connection);
void Slurp( FILE_DESCRIPTOR_OR_ERROR file_descriptor, unsigned long usec ) ;

GOOD_OR_BAD telnet_read(BYTE * buf, const size_t size, const struct parsedname *pn) ;

#define COM_slurp( file_descriptor ) Slurp( file_descriptor, 1000 )
#define TCP_slurp( file_descriptor ) Slurp( file_descriptor, 100000 )

void FreeInAll(void);
void RemoveIn( struct connection_in * conn ) ;

void FreeOutAll(void);
void DelIn(struct connection_in *in);

struct connection_in *AllocIn(const struct connection_in *in);
struct connection_in *LinkIn(struct connection_in *in);
struct connection_in *NewIn(const struct connection_in *in);

void Add_InFlight( GOOD_OR_BAD (*nomatch)(struct connection_in * trial,struct connection_in * existing), struct connection_in * new_in );
void Del_InFlight( GOOD_OR_BAD (*nomatch)(struct connection_in * trial,struct connection_in * existing), struct connection_in * new_in );

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
