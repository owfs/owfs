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

#ifndef OW_H  /* tedious wrapper */
#define OW_H

#ifndef OWFS_CONFIG_H
#error Please make sure owfs_config.h is included *before* this header file
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h>
#include <signal.h>

#include <unistd.h>
#include <fcntl.h>
#define __USE_XOPEN /* for strptime fuction */
#include <time.h>
#undef __USE_XOPEN /* for strptime fuction */
#include <termios.h>
#include <errno.h>
#include <syslog.h>
#include <ctype.h>
#include <sys/file.h> /* for flock */
#include <getopt.h> /* for long options */

#ifdef OW_USB
    #include <usb.h>
    extern usb_dev_handle * devusb ;
    int DS9490_detect( int useusb ) ;
    void DS9490_close(void) ;

#else /* OW_USB */
    extern void * devusb ;
#endif /* OW_USB */
/*
	OW -- Onw Wire
	Global variables -- each invokation will have it's own data
*/
/* Stuff from helper.h */
#define FUSE_MOUNTED_ENV        "_FUSE_MOUNTED"
#define FUSE_UMOUNT_CMD_ENV     "_FUSE_UNMOUNT_CMD"

/* command line options */
/* These a re the owlib-specific options */
#define OWLIB_OPT "f:p:hu::d:t:CFRKV"
extern const struct option owopts_long[] ;
void owopt( const char c , const char * const arg ) ;

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
extern unsigned char combuffer[] ;

/* Prototypes */
#define iREAD_FUNCTION( fname )  int fname(int *, const struct parsedname *)
#define uREAD_FUNCTION( fname )  int fname(unsigned int *, const struct parsedname * pn)
#define fREAD_FUNCTION( fname )  int fname(float *, const struct parsedname * pn)
#define yREAD_FUNCTION( fname )  int fname(int *, const struct parsedname * pn)
#define aREAD_FUNCTION( fname )  int fname(char *buf, const size_t size, const off_t offset, const struct parsedname * pn)
#define bREAD_FUNCTION( fname )  int fname(unsigned char *buf, const size_t size, const off_t offset, const struct parsedname * pn)

#define iWRITE_FUNCTION( fname )  int fname(const int *, const struct parsedname * pn)
#define uWRITE_FUNCTION( fname )  int fname(const unsigned int *, const struct parsedname * pn)
#define fWRITE_FUNCTION( fname )  int fname(const float *, const struct parsedname * pn)
#define yWRITE_FUNCTION( fname )  int fname(const int *, const struct parsedname * pn)
#define aWRITE_FUNCTION( fname )  int fname(const char *buf, const size_t size, const off_t offset, const struct parsedname * pn)
#define bWRITE_FUNCTION( fname )  int fname(const unsigned char *buf, const size_t size, const off_t offset, const struct parsedname * pn)

struct parsedname ;
struct device ;
struct filetype ;

void LibSetup( void ) ;
int LibStart( void ) ;
int ComSetup( const char * busdev ) ;
int USBSetup( int useusb ) ;
void LibClose( void ) ;

void DeviceSort( void ) ;
  int devicecmp(const void * code , const void * dev ) ;
  int filecmp(const void * name , const void * ex ) ;
  int devicesort( const void * a , const void * b ) ;
  int filesort( const void * a , const void * b ) ;

int FS_ParsedName( const char * const fn , struct parsedname * const pn ) ;
  void FS_ParsedName_destroy( struct parsedname * const pn ) ;
int NamePart( const char * filename, const char ** next, struct parsedname * pn ) ;
int FilePart( const char * const filename, const char ** next, struct parsedname * const pn ) ;
  void FS_parse_dir( char * const dest , const char * const buf ) ;
  size_t FileLength( const struct parsedname * const pn ) ;
  size_t FullFileLength( const struct parsedname * const pn ) ;
int CheckPresence( struct parsedname * const pn ) ;
void FS_devicename( char * const buffer, const int length, const unsigned char * const sn ) ;

unsigned char CRC8( const unsigned char * bytes , const int length ) ;
  unsigned char CRC8compute( const unsigned char * bytes , const int length ) ;
int CRC16( const unsigned char * bytes , const int length ) ;
unsigned char char2num( const char * s ) ;
unsigned char string2num( const char * s ) ;
char num2char( const unsigned char n ) ;
void num2string( char * s , const unsigned char n ) ;
void COM_speed(speed_t new_baud) ;
void string2bytes( const char * str , unsigned char * b , const int bytes ) ;
void bytes2string( char * str , const unsigned char * b , const int bytes ) ;
int UT_getbit(const unsigned char * buf, const int loc) ;
int UT_get2bit(const unsigned char * buf, const int loc) ;
void UT_setbit( unsigned char * buf, const int loc , const int bit ) ;
void UT_set2bit( unsigned char * buf, const int loc , const int bits ) ;

int COM_open( const char * port ) ;
void COM_flush( void ) ;
void COM_close( void );
void COM_break( void ) ;

int OW_first( char * str ) ;
int OW_next( char * str ) ;
int OW_first_alarm( char * str ) ;
int OW_next_alarm( char * str ) ;

void UT_delay(const int len) ;
int LI_reset( void ) ;

int FS_dir( void (* dirfunc)(void *,const struct parsedname * const), void * const data, struct parsedname * const pn ) ;

int FS_write(const char *path, const char *buf, const size_t size, const off_t offset) ;

int FS_read(const char *path, char *buf, const size_t size, const off_t offset) ;
  int FS_read_return( char *buf, const size_t size, const off_t offset , const char * src, const size_t len ) ;

int DS2480_baud( speed_t baud );
int DS2480_detect( void ) ;
int DS9097_detect( void ) ;
int BUS_first(unsigned char * serialnumber) ;
int BUS_next(unsigned char * serialnumber) ;
int BUS_first_alarm(unsigned char * serialnumber) ;
int BUS_next_alarm(unsigned char * serialnumber) ;
int BUS_first_family(const unsigned char family, unsigned char * serialnumber ) ;

int BUS_select(const struct parsedname * const pn) ;
int BUS_sendout_cmd( const unsigned char * cmd , const int len ) ;
int BUS_send_cmd( const unsigned char * const cmd , const int len ) ;
int BUS_sendback_cmd( const unsigned char * const cmd , unsigned char * const resp , const int len ) ;
int BUS_send_data( const unsigned char * const data , const int len ) ;
int BUS_send_and_get( const unsigned char * const send, const int sendlength, unsigned char * const get, const int getlength ) ;
int BUS_readin_data( unsigned char * const data , const int len ) ;
int BUS_alarmverify(struct parsedname * const pn) ;
int BUS_normalverify(struct parsedname * const pn) ;

#define BUS_detect		DS2480_detect
#define BUS_changebaud		DS2480_changebaud
#define BUS_reset		(iroutines.reset)
#define BUS_read		(iroutines.read)
#define BUS_write		(iroutines.write)
#define BUS_sendback_data	(iroutines.sendback_data)
#define BUS_next_both		(iroutines.next_both)
#define BUS_level		(iroutines.level)
#define BUS_ProgramPulse	(iroutines.ProgramPulse)
#define BUS_PowerByte		(iroutines.PowerByte)
#define BUS_databit		DS2480_databit
#define BUS_datablock		DS2480_datablock

void BUS_lock( void ) ;
void BUS_unlock( void ) ;

/* Several different structures:
  device -- one for each type of 1-wire device
  filetype -- one for each type of file
  parsedname -- translates a path into usable form
*/

/* --------------------------------------------------------- */
/* Filetypes -- directory entries for each 1-wire chip found */

enum ag_index {ag_numbers, ag_letters, } ;
enum ag_combined { ag_separate, ag_aggregate, } ;
/* aggregate defines array properties */
struct aggregate {
    int elements ; /* Maximum number of elements */
	enum ag_index letters ; /* name them with letters or numbers */
	enum ag_combined combined ; /* Combined bitmaps properties, or separately addressed */
} ;

#define ft_len_type (-1)
    /* property format, controls web display */
/* Some explanation of ft_format:
     Each file type is either a device (physical 1-wire chip or virtual statistics container).
     or a file (property).
     The devices act as directories of properties.
     The files are either properties of the device, or sometimes special directories themselves.
     If properties, they can be integer, text, etc or special directory types.
     There is also the directory type, ft_directory reflects a branch type, which restarts the parsing process.
*/
enum ft_format { ft_directory, ft_integer, ft_unsigned, ft_float, ft_ascii, ft_binary, ft_yesno } ;
    /* property changability. Static unchanged, Stable we change, Volatile changes */
enum ft_change { ft_static, ft_stable, ft_volatile, ft_second, ft_statistic, } ;

/* filetype gives -file types- for chip properties */
struct filetype {
    char * name ;
    int suglen ; // length of field
    struct aggregate * ag ; // struct pointer for aggregate
    enum ft_format format ; // type of data
    enum ft_change change ; // volatility
    union {
		void * v ;
    	int (*i) (int *, const struct parsedname *);
    	int (*u) (unsigned int *, const struct parsedname *);
    	int (*f) (float *, const struct parsedname *);
    	int (*y) (int *, const struct parsedname *);
    	int (*a) (char *, const size_t, const off_t, const struct parsedname *);
    	int (*b) (unsigned char *, const size_t, const off_t, const struct parsedname *);
	} read ;
    union {
		void * v ;
    	int (*i) (const int *, const struct parsedname *);
    	int (*u) (const unsigned int *, const struct parsedname *);
    	int (*f) (const float *, const struct parsedname *);
    	int (*y) (const int *, const struct parsedname *);
    	int (*a) (const char *, const size_t, const off_t, const struct parsedname *);
    	int (*b) (const unsigned char *, const size_t, const off_t, const struct parsedname *);
	} write ;
//    int (*write) (const char *, const size_t, const off_t, const struct parsedname *);
	void * data ;
} ;
#define NFT(ft) ((int)(sizeof(ft)/sizeof(struct filetype)))

/* -------------------------------- */
/* Devices -- types of 1-wire chips */
enum dev_type { dev_1wire, dev_interface, dev_status, dev_statistic, } ;

struct device {
    char * code ;
    char * name ;
    enum dev_type type ;
    int nft ;
    struct filetype * ft ;
} ;

/* #define DeviceEntry( code , chip )  { #code, #chip, NFT(chip), chip } */
/* Entries for struct device */
/* Cannot set the 3rd element (number of filetypes) at compile time because
   filetype arrays aren;t defined at this point */
#define DeviceHeader( chip )  extern struct device d_##chip ;
#define DeviceEntry( code , chip )  struct device d_##chip = { #code, #chip, dev_1wire, NFT(chip), chip } ;

/* Must be sorted for bsearch */
extern struct device * Devices[] ;
extern size_t nDevices ;
extern struct device NoDevice ;

/* -------------------------------------------- */
/* Parsedname -- path converted into components */
struct buspath {
    unsigned char sn[8] ;
	unsigned char branch ;
} ;

enum pn_type { pn_normal, pn_uncached, pn_alarm, } ;
struct parsedname {
    enum pn_type type ;
    unsigned char sn[8] ;
    struct device * dev ;
    struct filetype * ft ;
    int    extension ; // numerical extension (for array values) or -1
	int pathlength ;
	struct buspath * bp ;
} ;

extern speed_t speed;		/* terminal speed constant */

/* Delay for clearing buffer */
#define	WASTE_TIME	(2)

struct termios oldSerialTio;	/*old serial port settings*/

/* Globals */
extern char devport[] ;	/* Device name (COM port)*/
extern int  devfd     ; /*file descriptor for serial port*/
extern int presencecheck ; /* check if present whenever opening a directory or static file */
extern int portnum ; /* TCP port (for owhttpd) */

/* Gobal temperature scale */
enum temp_type { temp_celsius, temp_fahrenheit, temp_kelvin, temp_rankine, } ;
extern enum temp_type tempscale ;
float Temperature( float C) ;
float TemperatureGap( float C) ;

#define DEFAULT_TIMEOUT  (15)
extern int cacheavailable ; /* is caching available */
extern int background ; /* operate in background mode */
void Timeout( const char * c ) ;
extern int timeout ; /* timeout (in seconds) for the volatile cached values */
extern int timeout_slow ; /* timeout for the stable cached values */

/* device display format */
enum deviceformat { fdi, fi, fdidc, fdic, fidc, fic } ;
extern enum deviceformat devform ;

/* -------------------------------------------- */
/* Interface-specific routines ---------------- */
struct interface_routines {
    /* assymetric read (only input, no slots emitted */
    int (* read) ( unsigned char * const bytes , const int num ) ;
    /* assymetric write (only output, no response obtained */
    int (* write) (const unsigned char * const bytes , const int num ) ;
    /* reset the interface -- actually the 1-wire bus */
    int (* reset ) ( void ) ;
    /* Bulk of search routine, after set ups for first or alarm or family */
    int (* next_both) (unsigned char * serialnumber, unsigned char search) ;
    /* Set the electrical level of the 1-wire bus */
    int (* level) (int new_level) ;
    /* Send a byte with bus power to follow */
    int (* PowerByte) (const unsigned char byte, const unsigned int delay) ;
    /* Send a 12V 480msec oulse to program EEPROM */
    int (* ProgramPulse) ( void ) ;
    /* send and recieve data*/
    int (* sendback_data) ( const unsigned char * const data , unsigned char * const resp , const int len ) ;

} ;
extern struct interface_routines iroutines ;

void DS2480_setroutines( struct interface_routines * const f ) ;
void DS9097_setroutines( struct interface_routines * const f ) ;

/* -------------------------------------------- */
/* start of program -- for statistics amd file atrtributes */
extern time_t start_time ;

/* ----------------- */
/* -- Statistics --- */
/* ----------------- */
extern unsigned int cache_tries ;
extern unsigned int cache_hits ;
extern unsigned int cache_misses ;
extern unsigned int cache_flips ;
extern unsigned int cache_dels ;
extern unsigned int cache_adds ;
extern unsigned int cache_expired ;

extern unsigned int read_calls ;
extern unsigned int read_cache ;
extern unsigned int read_cachebytes ;
extern unsigned int read_bytes ;
extern unsigned int read_array ;
extern unsigned int read_aggregate ;
extern unsigned int read_arraysuccess ;
extern unsigned int read_aggregatesuccess ;
extern unsigned int read_tries[3] ;
extern unsigned int read_success ;

extern unsigned int write_calls ;
extern unsigned int write_bytes ;
extern unsigned int write_array ;
extern unsigned int write_aggregate ;
extern unsigned int write_arraysuccess ;
extern unsigned int write_aggregatesuccess ;
extern unsigned int write_tries[3] ;
extern unsigned int write_success ;

extern unsigned int dir_tries[3] ;
extern unsigned int dir_success ;
extern unsigned int dir_depth ;

extern struct timeval bus_time ;
extern struct timeval bus_pause ;
extern unsigned int bus_locks ;
extern unsigned int crc8_tries ;
extern unsigned int crc8_errors ;
extern unsigned int crc16_tries ;
extern unsigned int crc16_errors ;
extern unsigned int read_timeout ;

/* Cache code -- conditional */
#include "ow_cache.h"

/// mode bit flags
#define MODE_NORMAL                    0x00
#define MODE_OVERDRIVE                 0x01
#define MODE_STRONG5                   0x02
#define MODE_PROGRAM                   0x04
#define MODE_BREAK                     0x08

/* Globals for DS2480B state */
int UMode ;
int ULevel ;
int USpeed ;
int ProgramAvailable ;
int LastDiscrepancy ; // for search
int LastFamilyDiscrepancy ; // for search
int LastDevice ; // for search
int Version2480 ;
int AnyDevices ;

#endif /* OW_H */
