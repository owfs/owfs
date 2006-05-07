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

#ifndef OW_CONNECTION_H  /* tedious wrapper */
#define OW_CONNECTION_H

#include "ow.h"
#include "ow_counters.h"


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
//extern unsigned char combuffer[] ;

/** USB bulk endpoint FIFO size
  Need one for each for read and write
  This is for alt setting "3" -- 64 bytes, 1msec polling
*/
#define USB_FIFO_EACH 64
#define USB_FIFO_READ 0
#define USB_FIFO_WRITE USB_FIFO_EACH
#define USB_FIFO_SIZE ( USB_FIFO_EACH + USB_FIFO_EACH )

#if USB_FIFO_SIZE > UART_FIFO_SIZE
    #define MAX_FIFO_SIZE USB_FIFO_SIZE
#else
    #define MAX_FIFO_SIZE UART_FIFO_SIZE
#endif

struct connection_in ;
struct device_search ;
/* -------------------------------------------- */
/* Interface-specific routines ---------------- */
struct interface_routines {
    /* Detect if adapter is present, and open -- usually called outside of this routine */
    int (* detect) ( struct connection_in * in ) ;
    /* reset the interface -- actually the 1-wire bus */
    int (* reset ) (const struct parsedname * pn ) ;
    /* Bulk of search routine, after set ups for first or alarm or family */
    int (* next_both) (struct device_search * ds, const struct parsedname * pn) ;
    /* Change speed between overdrive and normal on the 1-wire bus */
    int (* overdrive) (const unsigned int overdrive, const struct parsedname * pn) ;
    /* Change speed between overdrive and normal on the 1-wire bus */
    int (* testoverdrive) (const struct parsedname * pn) ;
    /* Send a byte with bus power to follow */
    int (* PowerByte) (const unsigned char byte, unsigned char * resp, const unsigned int delay, const struct parsedname * pn) ;
    /* Send a 12V 480msec oulse to program EEPROM */
    int (* ProgramPulse) (const struct parsedname * pn) ;
    /* send and recieve data -- byte at a time */
    int (* sendback_data) (const unsigned char * data , unsigned char * resp , const size_t len, const struct parsedname * pn ) ;
    /* send and recieve data -- bit at a time */
    int (* sendback_bits) (const unsigned char * databits , unsigned char * respbits , const size_t len, const struct parsedname * pn ) ;
    /* select a device */
    int (* select) ( const struct parsedname * pn ) ;
    /* reconnect with a balky device */
    int (* reconnect) ( const struct parsedname * pn ) ;
    /* Close the connection (port) */
    void (* close) ( struct connection_in * in ) ;
} ;
#define BUS_detect(in)                      (((in)->iroutines.detect(in)))
#define BUS_sendback_data(data,resp,len,pn) (((pn)->in->iroutines.sendback_data)((data),(resp),(len),(pn)))
#define BUS_sendback_bits(data,resp,len,pn) (((pn)->in->iroutines.sendback_bits)((data),(resp),(len),(pn)))
#define BUS_next_both(ds,pn)                (((pn)->in->iroutines.next_both)((ds),(pn)))
#define BUS_ProgramPulse(pn)                (((pn)->in->iroutines.ProgramPulse)(pn))
#define BUS_PowerByte(byte,resp,delay,pn)   (((pn)->in->iroutines.PowerByte)((byte),(resp),(delay),(pn)))
#define BUS_select(pn)                      (((pn)->in->iroutines.select)(pn))
#define BUS_overdrive(speed,pn)             (((pn)->in->iroutines.overdrive)((speed),(pn)))
#define BUS_testoverdrive(pn)               (((pn)->in->iroutines.testoverdrive)((pn)))
#define BUS_close(in)                       (((in)->iroutines.close(in)))

#ifdef OW_MT
#define DEVLOCK(pn)           pthread_mutex_lock( &(((pn)->in)->dev_mutex) )
#define DEVUNLOCK(pn)         pthread_mutex_unlock( &(((pn)->in)->dev_mutex) )
#define ACCEPTLOCK(out)       pthread_mutex_lock(  &((out)->accept_mutex) )
#define ACCEPTUNLOCK(out)     pthread_mutex_unlock(&((out)->accept_mutex) )
#else /* OW_MT */
#define DEVLOCK(pn)
#define DEVUNLOCK(pn)
#define ACCEPTLOCK(out)
#define ACCEPTUNLOCK(out)
#endif /* OW_MT */

struct connin_serial {
    speed_t speed;
    int USpeed ;
    int ULevel ;
    int UMode ;
    struct termios oldSerialTio;    /*old serial port settings*/
} ;
struct connin_link {
    speed_t speed;
    int USpeed ;
    int ULevel ;
    int UMode ;
    struct termios oldSerialTio;    /*old serial port settings*/
} ;
struct connin_server {
    char * host ;
    char * service ;
    struct addrinfo * ai ;
    struct addrinfo * ai_ok ;
} ;
struct connin_usb {
    struct usb_device * dev ;
    struct usb_dev_handle * usb ;
    int usb_nr ;
    int USpeed ;
    int ULevel ;
    int UMode ;
    char ds1420_address[8];
  /* "Name" of the device, like "8146572300000051"
   * This is set to the first DS1420 id found on the 1-wire adapter which
   * exists on the DS9490 adapters. If user only have a DS2490 chip, there
   * are no such DS1420 device available. It's used to find the 1-wire adapter
   * if it's disconnected and later reconnected again.
   */
} ;
struct connin_i2c {
    int channels ;
    int index ;
    /* only one per chip, the bus entries for the other 7 channels point to the firt one */
#ifdef OW_MT
    pthread_mutex_t i2c_mutex ; // second level mutex for the entire chip */
#endif /* OW_MT */
    int current ;
    int fd ;
    struct connection_in * head ;
} ;

//enum server_type { srv_unknown, srv_direct, srv_client, src_
/* Network connection structure */
/* bus_remote is owserver and bus_tcp is Link-E */
enum bus_mode { 
    bus_unknown=0, 
    bus_remote, 
    bus_serial, 
    bus_usb, 
    bus_parallel, 
    bus_tcp, 
    bus_i2c, 
} ;

enum adapter_type {
    adapter_DS9097   =  0 ,
    adapter_DS1410   =  1 ,
    adapter_DS9097U2 =  2 ,
    adapter_DS9097U  =  3 ,
    adapter_LINK     =  7 ,
    adapter_DS9490   =  8 ,
    adapter_tcp      =  9 ,
    adapter_Bad      = 10 ,
    adapter_LINK_10       ,
    adapter_LINK_11       ,
    adapter_LINK_12       ,
    adapter_LINK_E        ,
    adapter_DS2482_100    ,
    adapter_DS2482_800    ,
} ;

extern unsigned long int usec_read ; /* how long to waait for serial reads */


extern int LINK_mode ; /* flag to use LINKs in ascii mode */
enum e_reconnect {
    reconnect_ok = 0 ,
    reconnect_error = 5 ,
} ;

struct device_search {
    int LastDiscrepancy ; // for search
    int LastDevice ; // for search
    unsigned char sn[8] ;
    unsigned char search ;
} ;

struct connection_in {
    struct connection_in * next ;
    int index ;
    char * name ;
    int fd ;
#ifdef OW_MT
    pthread_mutex_t bus_mutex ;
    pthread_mutex_t dev_mutex ;
    void * dev_db ; // dev-lock tree
#endif /* OW_MT */
    enum e_reconnect reconnect_state ;
    struct timeval last_lock ; /* statistics */
    struct timeval last_unlock ;
    unsigned int bus_reconnect ;
    unsigned int bus_reconnect_errors ;
    unsigned int bus_locks ;
    unsigned int bus_unlocks ;
    unsigned int bus_errors ;
    struct timeval bus_time ;

    struct timeval bus_read_time ;
    struct timeval bus_write_time ; /* for statistics */
  
    enum bus_mode busmode ;
    struct interface_routines iroutines ;
    enum adapter_type Adapter ;
    char * adapter_name ;
    int AnyDevices ;
    int ExtraReset ; // DS1994/DS2404 might need an extra reset
    int use_overdrive_speed ;
    int ds2404_compliance ;
    int ProgramAvailable ;
    size_t last_root_devs ;
    struct buspath branch ; // Branch currently selected

    /* Static buffer for serial conmmunications */
    /* Since only used during actual transfer to/from the adapter,
        should be protected from contention even when multithreading allowed */
    unsigned char combuffer[MAX_FIFO_SIZE] ;
    union {
        struct connin_serial serial ;
        struct connin_link   link   ;
        struct connin_server server ;
        struct connin_usb    usb    ;
        struct connin_i2c    i2c    ;
    } connin ;
} ;
/* Network connection structure */
struct connection_out {
    struct connection_out * next ;
    char * name ;
    char * host ;
    char * service ;
    int index ;
    struct addrinfo * ai ;
    struct addrinfo * ai_ok ;
    int fd ;
#ifdef OW_MT
    pthread_mutex_t accept_mutex ;
#endif /* OW_MT */
} ;
extern struct connection_out * outdevice ;
extern struct connection_in * indevice ;

/* This bug-fix/workaround function seem to be fixed now... At least on
 * the platforms I have tested it on... printf() in owserver/src/c/owserver.c
 * returned very strange result on c->busmode before... but not anymore */
enum bus_mode get_busmode(struct connection_in *c);

// mode bit flags for level
#define MODE_NORMAL                    0x00
#define MODE_STRONG5                   0x01
#define MODE_PROGRAM                   0x02
#define MODE_BREAK                     0x04

// 1Wire Bus Speed Setting Constants
#define ONEWIREBUSSPEED_REGULAR        0x00
#define ONEWIREBUSSPEED_FLEXIBLE       0x01 /* Only used for USB adapter */
#define ONEWIREBUSSPEED_OVERDRIVE      0x02

/* Serial port */
void COM_speed(speed_t new_baud, const struct parsedname * pn) ;
int COM_open( struct connection_in * in  ) ;
void COM_flush( const struct parsedname * pn  ) ;
void COM_close( struct connection_in * in  );
void COM_break( const struct parsedname * pn  ) ;

void FreeIn( void ) ;
void FreeOut( void ) ;
void DelIn( struct connection_in * in ) ;

struct connection_in * NewIn( const struct connection_in * in ) ;
struct connection_out * NewOut( void ) ;
struct connection_in *find_connection_in(int nr);

enum transaction_type { trxn_select, trxn_match, trxn_read, trxn_power, trxn_program, trxn_reset, trxn_end, } ;
struct transaction_log {
    const unsigned char * out ;
    unsigned char * in ;
    size_t  size ;
    enum transaction_type type ;
} ;
#define TRXN_START  { NULL, NULL, 0, trxn_select }
#define TRXN_END    { NULL, NULL, 0, trxn_end }
/* Low-level functions
    slowly being abstracted and separated from individual
    interface type details
*/
int DS2480_baud( speed_t baud, const struct parsedname * const pn );

int Server_detect( struct connection_in * in  ) ;
int DS2480_detect( struct connection_in * in ) ;

#ifdef OW_PARPORT
int DS1410_detect( struct connection_in * in ) ;
#endif /* OW_PARPORT */

int DS9097_detect( struct connection_in * in ) ;
int LINK_detect( struct connection_in * in ) ;
int BadAdapter_detect( struct connection_in * in ) ;
int LINKE_detect( struct connection_in * in ) ;
int DS2482_detect( struct connection_in * in ) ;

#ifdef OW_USB
int DS9490_enumerate( void ) ;
int DS9490_detect( struct connection_in * in ) ;
void DS9490_close( struct connection_in * in ) ;
#endif /* OW_USB */

int BUS_reset(const struct parsedname * pn) ;
int BUS_first(struct device_search * ds, const struct parsedname * pn) ;
int BUS_next(struct device_search * ds, const struct parsedname * pn) ;
int BUS_first_alarm(struct device_search * ds, const struct parsedname * pn) ;
int BUS_first_family(const unsigned char family, struct device_search * ds, const struct parsedname * pn ) ;

int BUS_select_low(const struct parsedname * const pn) ;
int BUS_select_branch( const struct parsedname * pn) ;

int BUS_sendout_cmd(const unsigned char * cmd , const size_t len, const struct parsedname * pn  ) ;
int BUS_send_cmd(const unsigned char * const cmd , const size_t len, const struct parsedname * pn  ) ;
int BUS_sendback_cmd(const unsigned char * const cmd , unsigned char * const resp , const size_t len, const struct parsedname * pn  ) ;
int BUS_send_data(const unsigned char * const data , const size_t len, const struct parsedname * pn  ) ;
int BUS_readin_data(unsigned char * const data , const size_t len, const struct parsedname * pn ) ;
int BUS_alarmverify(const struct parsedname * pn) ;
int BUS_normalverify(const struct parsedname * pn) ;

int BUS_PowerByte_low(const unsigned char byte, unsigned char * resp, unsigned int delay, const struct parsedname * pn) ;
int BUS_next_both_low(struct device_search * ds, const struct parsedname * pn) ;
int BUS_sendback_data_low( const unsigned char * data, unsigned char * resp , const size_t len, const struct parsedname * pn ) ;

int TestConnection( const struct parsedname * pn ) ;

int BUS_transaction( const struct transaction_log * tl, const struct parsedname * pn ) ;

#define STAT_ADD1_BUS( err, in )     STATLOCK; ++err; ++(in->bus_errors) ; STATUNLOCK ;

#endif /* OW_CONNECTION_H */
