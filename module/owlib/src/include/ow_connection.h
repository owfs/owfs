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
#include "ow_counters.h"
#include <sys/ioctl.h>
#include "ow_transaction.h"

/* Exposed connection info */
extern int count_outbound_connections;
extern int count_inbound_connections;
extern int count_sidebound_connections;

/* large enough for arrays of 2048 elements of ~49 bytes each */
#define MAX_OWSERVER_PROTOCOL_PACKET_SIZE  100050

/* Number of "fake" adapters */
extern int global_count_fake_busses;
extern int global_count_tester_busses;

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
#define LINK_FIFO_SIZE UART_FIFO_SIZE
#define LINKE_FIFO_SIZE 1500
#define I2C_FIFO_SIZE 1

#if USB_FIFO_SIZE > UART_FIFO_SIZE
#define MAX_FIFO_SIZE USB_FIFO_SIZE
#else
#define MAX_FIFO_SIZE UART_FIFO_SIZE
#endif

struct connection_in;
struct device_search;

/* -------------------------------------------- */
/* Interface-specific routines ---------------- */
struct interface_routines {
	/* Detect if adapter is present, and open -- usually called outside of this routine */
	int (*detect) (struct connection_in * in);
	/* reset the interface -- actually the 1-wire bus */
	int (*reset) (const struct parsedname * pn);
	/* Bulk of search routine, after set ups for first or alarm or family */
	int (*next_both) (struct device_search * ds, const struct parsedname * pn);
	/* Send a byte with bus power to follow */
	int (*PowerByte) (const BYTE data, BYTE * resp, const UINT delay, const struct parsedname * pn);
	/* Send a 12V 480msec oulse to program EEPROM */
	int (*ProgramPulse) (const struct parsedname * pn);
	/* send and recieve data -- byte at a time */
	int (*sendback_data) (const BYTE * data, BYTE * resp, const size_t len, const struct parsedname * pn);
	/* send and recieve data -- byte at a time */
	int (*select_and_sendback) (const BYTE * data, BYTE * resp, const size_t len, const struct parsedname * pn);
	/* send and recieve data -- bit at a time */
	int (*sendback_bits) (const BYTE * databits, BYTE * respbits, const size_t len, const struct parsedname * pn);
	/* select a device */
	int (*select) (const struct parsedname * pn);
	/* reconnect with a balky device */
	int (*reconnect) (const struct parsedname * pn);
	/* Close the connection (port) */
	void (*close) (struct connection_in * in);
	/* transaction */
	int (*transaction) (const struct transaction_log * tl, const struct parsedname * pn);
	/* capabilities flags */
	UINT flags;
};
#define BUS_detect(in)                      (((in)->iroutines.detect(in)))
#define BUS_sendback_bits(data,resp,len,pn) (((pn)->selected_connection->iroutines.sendback_bits)((data),(resp),(len),(pn)))
#define BUS_close(in)                       (((in)->iroutines.close(in)))

/* placed in iroutines.flags */
#define ADAP_FLAG_overdrive     0x00000001
#define ADAP_FLAG_2409path      0x00000010
#define ADAP_FLAG_dirgulp       0x00000100
#define ADAP_FLAG_bundle        0x00001000
#define ADAP_FLAG_dir_auto_reset 0x00002000

#define AdapterSupports2409(pn)	(((pn)->selected_connection->iroutines.flags&ADAP_FLAG_2409path)!=0)

#if OW_MT
#define DEVLOCK(pn)           pthread_mutex_lock( &(((pn)->selected_connection)->dev_mutex) )
#define DEVUNLOCK(pn)         pthread_mutex_unlock( &(((pn)->selected_connection)->dev_mutex) )
#define ACCEPTLOCK(out)       pthread_mutex_lock(  &((out)->accept_mutex) )
#define ACCEPTUNLOCK(out)     pthread_mutex_unlock(&((out)->accept_mutex) )
#define OUTLOCK(out)          pthread_mutex_lock(  &((out)->out_mutex) )
#define OUTUNLOCK(out)        pthread_mutex_unlock(&((out)->out_mutex) )
#define SIDELOCK(side)        pthread_mutex_lock(  &((side)->side_mutex) )
#define SIDEUNLOCK(side)      pthread_mutex_unlock(&((side)->side_mutex) )
#else							/* OW_MT */
#define DEVLOCK(pn)
#define DEVUNLOCK(pn)
#define ACCEPTLOCK(out)
#define ACCEPTUNLOCK(out)
#define OUTLOCK(out)
#define OUTUNLOCK(out)
#define SIDELOCK(side)
#define SIDEUNLOCK(side)
#endif							/* OW_MT */

enum bus_speed {
	bus_speed_slow,
	bus_speed_overdrive,
};

enum bus_flex { bus_no_flex, bus_yes_flex };

enum ds2480b_mode { ds2480b_data_mode, ds2480b_command_mode, } ;

enum ds2480b_baud {
	ds2480b_9600,
	ds2480b_19200,
	ds2480b_57600,
	ds2480b_115200,
	ds2480b_pol_9600,
	ds2480b_pol_19200,
	ds2480b_pol_57600,
	ds2480b_pol_115200,
} ;
	
struct connin_serial {
	enum ds2480b_mode mode ;
	enum ds2480b_baud baud ;
	struct termios oldSerialTio;	/*old serial port settings */
};

struct connin_fake {
	struct dirblob db;
	int bus_number_this_type;
};

struct connin_usb {
	struct usb_device *dev;
	struct usb_dev_handle *usb;
	int usb_nr;
	int datasampleoffset;
	int writeonelowtime;
	int pulldownslewrate;
	int timeout;
	/* "Name" of the device, like "8146572300000051"
	 * This is set to the first DS1420 id found on the 1-wire adapter which
	 * exists on the DS9490 adapters. If user only have a DS2490 chip, there
	 * are no such DS1420 device available. It's used to find the 1-wire adapter
	 * if it's disconnected and later reconnected again.
	 */
	BYTE ds1420_address[8];
	struct dirblob main;		/* main directory */
	struct dirblob alarm;		/* alarm directory */
};

struct connin_i2c {
	int channels;
	int index;
	int i2c_address;
	int i2c_index ;
	BYTE configreg;
	BYTE configchip;
	/* only one per chip, the bus entries for the other 7 channels point to the firt one */
#if OW_MT
	pthread_mutex_t i2c_mutex;	// second level mutex for the entire chip */
#endif							/* OW_MT */
	int current;
	int file_descriptor;
	struct connection_in *head;
};

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

struct connin_ha7 {
	struct connin_tcp tcp;		// mirror connin.server
	ASCII lock[10];
	int locked;
	int found;
	struct dirblob main;		/* main directory */
	struct dirblob alarm;		/* alarm directory */
};

struct connin_etherweather {
	struct connin_tcp tcp;
};

struct connin_link {
	struct connin_tcp tcp;		// mirror connin.server
	struct termios oldSerialTio;	/*old serial port settings */
	struct dirblob main;		/* main directory */
	struct dirblob alarm;		/* alarm directory */
};

struct connin_w1 {
	// bus master name kept in name
	// netlink fd kept in file_descriptor
};

//enum server_type { srv_unknown, srv_direct, srv_client, src_
/* Network connection structure */
enum bus_mode {
	bus_unknown = 0,
	bus_serial,
	bus_passive,
	bus_usb,
	bus_parallel,
	bus_server,
	bus_zero,
	bus_i2c,
	bus_ha7net,
	bus_ha5,
	bus_ha7e,
	bus_fake,
	bus_tester,
	bus_link,
	bus_elink,
	bus_etherweather,
	bus_w1,
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
	adapter_LINK_E,
	adapter_DS2482_100,
	adapter_DS2482_800,
	adapter_HA7NET,
	adapter_HA5,
	adapter_HA7E,
	adapter_EtherWeather,
	adapter_fake,
	adapter_tester,
	adapter_w1,
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
	e_bus_search_errors,
	e_bus_status_errors,
	e_bus_select_errors,
	e_bus_try_overdrive,
	e_bus_failed_overdrive,
	e_bus_stat_last_marker
};

struct connection_in {
	struct connection_in *next;
	int index;
	char *name;
	int file_descriptor;
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
	int AnyDevices;
	int ExtraReset;				// DS1994/DS2404 might need an extra reset
	enum bus_speed speed;
	speed_t baudrate ;
	enum bus_flex flex ;
	int changed_bus_settings;
	int ds2404_compliance;
	int ProgramAvailable;
	size_t last_root_devs;
	int buspath_bad;			// should the current DS2409 branches be cleared?
	struct buspath branch;		// Branch currently selected
	BYTE combuffer[MAX_FIFO_SIZE];

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
		struct connin_ha7 ha7;
		struct connin_etherweather etherweather;
		struct connin_w1 w1;
	} connin;
};
extern struct connection_in *head_inbound_list;

/* Network connection structure */
struct connection_out {
	struct connection_out *next;
	void (*HandlerRoutine) (int file_descriptor);
	void (*Exit) (int errcode);
	char *name;
	char *host;
	char *service;
	int index;
	struct addrinfo *ai;
	struct addrinfo *ai_ok;
	int file_descriptor;
#if OW_MT
	pthread_mutex_t accept_mutex;
	pthread_mutex_t out_mutex;
	pthread_t tid;
#endif							/* OW_MT */
#if OW_ZERO
	DNSServiceRef sref0;
	DNSServiceRef sref1;
#endif
};
extern struct connection_out *head_outbound_list;

/* Network connection structure */
struct connection_side {
	struct connection_side *next;
	char *name;
	char *host;
	char *service;
	int index;
	struct addrinfo *ai;
	struct addrinfo *ai_ok;
	int file_descriptor;
	int good_entry;
#if OW_MT
	pthread_mutex_t side_mutex;
#endif							/* OW_MT */
};
extern struct connection_side *head_sidebound_list;

#define  FD_PERSISTENT_IN_USE    -2
#define  FD_PERSISTENT_NONE      -1
#define  FD_CURRENT_BAD          -1
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
void COM_speed(speed_t new_baud, const struct parsedname *pn);
int COM_open(struct connection_in *in);
void COM_flush(const struct parsedname *pn);
void COM_close(struct connection_in *in);
void COM_break(const struct parsedname *pn);
int COM_write( const char * data, size_t length, const struct parsedname * pn ) ;
int COM_read( char * data, size_t length, const struct parsedname * pn ) ;

void FreeIn(void);
void FreeOut(void);
void FreeSide(void);
void DelIn(struct connection_in *in);

struct connection_in *NewIn(const struct connection_in *in);
struct connection_out *NewOut(void);
struct connection_side *NewSide(void);
struct connection_in *find_connection_in(int nr);

/* Bonjour registration */
void OW_Announce(struct connection_out *out);
void OW_Browse(void);

int Server_detect(struct connection_in *in);
int Zero_detect(struct connection_in *in);
int DS2480_detect(struct connection_in *in);

#if OW_PARPORT
int DS1410_detect(struct connection_in *in);
#endif							/* OW_PARPORT */

int DS9097_detect(struct connection_in *in);
int LINK_detect(struct connection_in *in);
int BadAdapter_detect(struct connection_in *in);
int LINKE_detect(struct connection_in *in);
int Fake_detect(struct connection_in *in);
int Tester_detect(struct connection_in *in);
int EtherWeather_detect(struct connection_in *in);

#if OW_HA7
int HA7_detect(struct connection_in *in);
int FS_FindHA7(void);
#endif							/* OW_HA7 */

#if OW_I2C
int DS2482_detect(struct connection_in *in);
#endif							/* OW_I2C */

#if OW_USB
int DS9490_enumerate(void);
int DS9490_detect(struct connection_in *in);
void DS9490_close(struct connection_in *in);
#endif							/* OW_USB */

#define BUS_RESET_OK    0
#define BUS_RESET_SHORT 1
int BUS_reset(const struct parsedname *pn);

int BUS_first(struct device_search *ds, const struct parsedname *pn);
int BUS_next(struct device_search *ds, const struct parsedname *pn);
int BUS_first_alarm(struct device_search *ds, const struct parsedname *pn);

int BUS_select(const struct parsedname *pn);

int BUS_sendout_cmd(const BYTE * cmd, const size_t len, const struct parsedname *pn);
int BUS_send_cmd(const BYTE * cmd, const size_t len, const struct parsedname *pn);
int BUS_sendback_cmd(const BYTE * cmd, BYTE * resp, const size_t len, const struct parsedname *pn);
int BUS_send_data(const BYTE * data, const size_t len, const struct parsedname *pn);
int BUS_readin_data(BYTE * data, const size_t len, const struct parsedname *pn);
int BUS_verify(BYTE search, const struct parsedname *pn);

int BUS_PowerByte(const BYTE data, BYTE * resp, UINT delay, const struct parsedname *pn);
int BUS_ProgramPulse(const struct parsedname *pn);
int BUS_next_both(struct device_search *ds, const struct parsedname *pn);
int BUS_sendback_data(const BYTE * data, BYTE * resp, const size_t len, const struct parsedname *pn);
int BUS_select_and_sendback(const BYTE * data, BYTE * resp, const size_t len, const struct parsedname *pn);

int TestConnection(const struct parsedname *pn);

#define STAT_ADD1_BUS( err, in )     STATLOCK; ++((in)->bus_stat[err]) ; STATUNLOCK

#endif							/* OW_CONNECTION_H */
