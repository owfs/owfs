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

#define _FILE_OFFSET_BITS   64
#include <features.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h>
#include <signal.h>
#include <stdint.h> /* for bit twiddling */

#include <unistd.h>
#include <fcntl.h>
#ifndef __USE_XOPEN
#define __USE_XOPEN /* for strptime fuction */
#include <time.h>
#undef __USE_XOPEN /* for strptime fuction */
#else
#include <time.h>
#endif
#include <termios.h>
#include <errno.h>
#include <syslog.h>
#include <ctype.h>
#include <sys/file.h> /* for flock */
#include <sys/stat.h> /* for stat */
#include <sys/types.h> /* for stat */
#include <getopt.h> /* for long options */
#ifndef __USE_GNU
 #define __USE_GNU
 #include <search.h>
 #undef __USE_GNU
#else /* __USE_GNU */
 #include <search.h>
#endif /* __USE_GNU */

/* Parport enabled uses two flags (one a holdover from the embedded work) */
#ifdef USE_NO_PARPORT
    #undef OW_PARPORT
#endif

extern int multithreading ;
extern int maxslots ;
#ifdef OW_MT
    #include <pthread.h>
#ifdef HAVE_SEMAPHORE_H
    #include <semaphore.h>
#else
    #include "sem.h"
#endif
    extern pthread_mutex_t bus_mutex ;
    extern pthread_mutex_t stat_mutex ;
    extern pthread_mutex_t cache_mutex ;
    extern pthread_mutex_t store_mutex ;
    extern pthread_mutex_t fstat_mutex ;
    extern pthread_mutex_t simul_mutex ;
#ifdef __UCLIBC__
    extern pthread_mutex_t uclibc_mutex;
#endif
    #define STATLOCK       pthread_mutex_lock(  &stat_mutex) ;
    #define STATUNLOCK     pthread_mutex_unlock(&stat_mutex) ;
    #define CACHELOCK      pthread_mutex_lock(  &cache_mutex) ;
    #define CACHEUNLOCK    pthread_mutex_unlock(&cache_mutex) ;
    #define STORELOCK      pthread_mutex_lock(  &store_mutex) ;
    #define STOREUNLOCK    pthread_mutex_unlock(&store_mutex) ;
    #define FSTATLOCK      pthread_mutex_lock(  &fstat_mutex) ;
    #define FSTATUNLOCK    pthread_mutex_unlock(&fstat_mutex) ;
    #define SIMULLOCK      pthread_mutex_lock(  &simul_mutex) ;
    #define SIMULUNLOCK    pthread_mutex_unlock(&simul_mutex) ;
#ifdef __UCLIBC__
    #define UCLIBCLOCK     pthread_mutex_lock(  &uclibc_mutex) ;
    #define UCLIBCUNLOCK   pthread_mutex_unlock(&uclibc_mutex) ;
#else /* __UCLIBC__ */
    #define UCLIBCLOCK
    #define UCLIBCUNLOCK
#endif /* __UCLIBC__ */

#else /* OW_MT */
    #define STATLOCK
    #define STATUNLOCK
    #define CACHELOCK
    #define CACHEUNLOCK
    #define STORELOCK
    #define STOREUNLOCK
    #define FSTATLOCK
    #define FSTATUNLOCK
    #define SIMULLOCK
    #define SIMULUNLOCK
    #define UCLIBCLOCK
    #define UCLIBCUNLOCK
#endif /* OW_MT */
#define BUSLOCK    BUS_lock() ;
#define BUSUNLOCK  BUS_unlock() ;

#ifdef OW_USB
    #include <usb.h>
    extern usb_dev_handle * devusb ;
    int DS9490_detect( void ) ;
    void DS9490_close(void) ;

#else /* OW_USB */
    extern void * devusb ;
#endif /* OW_USB */
/*
    OW -- Onw Wire
    Global variables -- each invokation will have it's own data
*/

/* command line options */
/* These a re the owlib-specific options */
#include <fcntl.h>
#ifndef __USE_XOPEN
#define __USE_XOPEN /* for strptime fuction */
#include <time.h>
#undef __USE_XOPEN /* for strptime fuction */
#else
#include <time.h>
#endif
#include <termios.h>
#include <errno.h>
#include <syslog.h>
#include <ctype.h>
#include <sys/file.h> /* for flock */
#include <sys/stat.h> /* for stat */
#include <sys/types.h> /* for stat */
#include <getopt.h> /* for long options */

/*
    OW -- Onw Wire
    Global variables -- each invokation will have it's own data#define OWLIB_OPT "f:p:hu::d:t:CFRKVP:"CMD_ENV     "_FUSE_UNMOUNT_CMD"
*/
/* command line options */
/* These a re the owlib-specific options */
#define OWLIB_OPT "f:p:s:hu::d:t:CFRKVP:"
extern const struct option owopts_long[] ;
int owopt( const int c , const char * const arg ) ;

/* PID file nsme */
extern pid_t pid_num ;
extern char * pid_file ;
/* FUSE options */
extern char * fuse_opt ;

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

/* Floating point */
/* I hate to do this, making everything a double */
/* The compiler complains mercilessly, however */
/* 1-wire really is low precision -- float is more than enough */
#define FLOAT   double
#define DATE    time_t

/* Several different structures:
  device -- one for each type of 1-wire device
  filetype -- one for each type of file
  parsedname -- translates a path into usable form
*/

/* --------------------------------------------------------- */
/* Filetypes -- directory entries for each 1-wire chip found */
/*
Filetype is the most elaborate of the internal structures, though
simple in concept.

Actually a little misnamed. Each filetype corresponds to a device
property, and to a file in the file system (though there are Filetype
entries for some directory elements too)

Filetypes belong to a particular device. (i.e. each device has it's list
of filetypes) and have a name and pointers to processing functions. Filetypes
also have a data format (integer, ascii,...) and a data length, and an indication
of whether the property is static, changes only on command, or is volatile.

Some properties occur are arrays (pages of memory, logs of temperature
values). The "aggregate" structure holds to allowable size, and the method
of access. -- Aggregate properties are either accessed all at once, then
split, or accessed individually. The choice depends on the device hardware.
There is even a further wrinkle: mixed. In cases where the handling can be either,
mixed causes separate handling of individual items are querried, and combined
if ALL are requested. This is useful for the DS2450 quad A/D where volt and PIO functions
step on each other, but the conversion time for individual is rather costly.
 */

enum ag_index {ag_numbers, ag_letters, } ;
enum ag_combined { ag_separate, ag_aggregate, ag_mixed, } ;
/* aggregate defines array properties */
struct aggregate {
    int elements ; /* Maximum number of elements */
    enum ag_index letters ; /* name them with letters or numbers */
    enum ag_combined combined ; /* Combined bitmaps properties, or separately addressed */
} ;

/* file lengths that need special processing */
/* stored (as negative enum) in ft.suglen */
enum fl_funny { fl_zero, fl_type, fl_adap_name, fl_adap_port, fl_pidfile, fl_store, } ;

    /* property format, controls web display */
/* Some explanation of ft_format:
     Each file type is either a device (physical 1-wire chip or virtual statistics container).
     or a file (property).
     The devices act as directories of properties.
     The files are either properties of the device, or sometimes special directories themselves.
     If properties, they can be integer, text, etc or special directory types.
     There is also the directory type, ft_directory reflects a branch type, which restarts the parsing process.
*/
enum ft_format { ft_directory, ft_subdir, ft_integer, ft_unsigned, ft_float, ft_ascii, ft_binary, ft_yesno, ft_date, ft_bitfield, } ;
    /* property changability. Static unchanged, Stable we change, Volatile changes */
enum ft_change { ft_static, ft_stable, ft_Astable, ft_volatile, ft_Avolatile, ft_second, ft_statistic, ft_persistent, } ;

/* Predeclare parsedname */
struct parsedname ;

/* Maximum length of a file or directory name, and extension */
#define OW_NAME_MAX      (32)
#define OW_EXT_MAX       (6)
#define OW_FULLNAME_MAX  (OW_NAME_MAX+OW_EXT_MAX)

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
        int (*f) (FLOAT *, const struct parsedname *);
        int (*y) (int *, const struct parsedname *);
        int (*d) (DATE *, const struct parsedname *);
        int (*a) (char *, const size_t, const off_t, const struct parsedname *);
        int (*b) (unsigned char *, const size_t, const off_t, const struct parsedname *);
    } read ; // read callback function
    union {
        void * v ;
        int (*i) (const int *, const struct parsedname *);
        int (*u) (const unsigned int *, const struct parsedname *);
        int (*f) (const FLOAT *, const struct parsedname *);
        int (*y) (const int *, const struct parsedname *);
        int (*d) (const DATE *, const struct parsedname *);
        int (*a) (const char *, const size_t, const off_t, const struct parsedname *);
        int (*b) (const unsigned char *, const size_t, const off_t, const struct parsedname *);
    } write ; // write callback function
    void * data ; // extra data pointer (used for separating similar but differently name functions)
} ;
#define NFT(ft) ((int)(sizeof(ft)/sizeof(struct filetype)))

/* --------- end Filetype -------------------- */
/* ------------------------------------------- */

/* Internal properties -- used by some devices */
/* in passing to store state information        */
struct internal_prop {
    char * name ;
    enum ft_change change ;
} ;

/* -------------------------------- */
/* Devices -- types of 1-wire chips */
/*
device structure corresponds to 1-wire device
also to virtual devices, like statistical groupings
and interfaces (LINK, DS2408, ... )

devices have a list or properties that appear as
files under the device directory, they correspond
to device features (memory, name, temperature) and
bound the allowable files in a device directory
*/

/* Device tree for matching names */
/* Bianry tree implementation */
/* A separate root for each device type: real, statistics, settings, system, structure */
extern void * Tree[6] ;

    /* supports RESUME command */
#define DEV_resume  0x0001
    /* responds to simultaneous temperature convert 0x44 */
#define DEV_temp    0x8000
    /* responds to simultaneous voltage convert 0x3C */
#define DEV_volt    0x4000


struct device {
    char * code ;
    char * name ;
    uint16_t flags ;
    int nft ;
    struct filetype * ft ;
} ;

#define DeviceHeader( chip )    extern struct device d_##chip ;
//#define DeviceEntry( code , chip )  { #code, #chip, 0, NFT(chip), chip } ;
/* Entries for struct device */
/* Cannot set the 3rd element (number of filetypes) at compile time because
   filetype arrays aren;t defined at this point */
#define DeviceEntryExtended( code , chip , flags )  struct device d_##chip = { #code, #chip, flags ,  NFT(chip), chip } ;
#define DeviceEntry( code , chip )  DeviceEntryExtended( code, chip, 0 )

/* Bad bad C library */
/* implementation of tfind, tsearch returns an opaque structure */
/* you have to know that the first element is a pointer to your data */
struct device_opaque {
    struct device * key ;
    void * other ;
} ;

/* Must be sorted for bsearch */
//extern struct device * Devices[] ;
//extern size_t nDevices ;
extern struct device NoDevice ;
extern struct device * DeviceSimultaneous ;

/* ---- end device --------------------- */
/* ------------------------------------- */


/* ------------------------}
struct devlock {
    unsigned char sn[8] ;
    pthread_mutex_t lock ;
    int users ;
} ;
extern struct devlock DevLock[] ;
-------------------- */
/* Parsedname -- path converted into components */
/*
Parsed name is the primary structure interpreting a
owfs systrem call. It is the interpretation of the owfs
file name, or the owhttpd URL. It contains everything
but the operation requested. The operation (read, write
or directory is in the extended URL or the actual callback
function requested).
*/
/*
Parsed name has several components:
sn is the serial number of the device
dev and ft are pointers to device and filetype
  members corresponding to the element
buspath and pathlength interpret the route through
  DS2409 branch controllers
filetype and extension correspond to property
  (filetype) details
subdir points to in-device groupings
*/

struct buspath {
    unsigned char sn[8] ;
    unsigned char branch ;
} ;
struct stateinfo {
    int LastDiscrepancy ; // for search
    int LastFamilyDiscrepancy ; // for search
    int LastDevice ; // for search
    int AnyDevices ;
    int lock ; // place in dev lock array
} ;

enum pn_type { pn_real=0, pn_statistics, pn_system, pn_settings, pn_structure } ;
enum pn_state { pn_normal=1, pn_uncached=2, pn_alarm=4, pn_text=8} ;
struct parsedname {
    enum pn_type type ; // global branch
    enum pn_state state ; // global branch
    unsigned char sn[8] ; // 64-bit serial number
    struct device * dev ; // 1-wire device
    struct filetype * ft ; // device property
    int    extension ; // numerical extension (for array values) or -1
    struct filetype * subdir ; // in-device grouping
    int pathlength ; // DS2409 branching depth
    struct buspath * bp ; // DS2409 branching route
    struct stateinfo * si ;
} ;

enum simul_type { simul_temp, simul_volt, } ;

/* ---- end Parsedname ----------------- */
/* ------------------------------------- */

extern speed_t speed;        /* terminal speed constant */

/* Delay for clearing buffer */
#define    WASTE_TIME    (2)

/* Globals */
extern char * devport ;    /* Device name (COM port)*/
extern int  devfd     ; /*file descriptor for serial port*/
extern int useusb ; /* Which USB adapter to use (1-based index) */
extern int presencecheck ; /* check if present whenever opening a directory or static file */
extern int portnum ; /* TCP port (for owhttpd) */
extern char * portname ; /* TCP port (for owhttpd) */
extern char * servername ; /* TCP port (for owhttpd) */
extern int readonly ; /* readonly file system ? */

/* Gobal temperature scale */
enum temp_type { temp_celsius, temp_fahrenheit, temp_kelvin, temp_rankine, } ;
extern enum temp_type tempscale ;
FLOAT Temperature( FLOAT C) ;
FLOAT TemperatureGap( FLOAT C) ;

extern int cacheavailable ; /* is caching available */
extern int cacheenabled ; /* is caching enabled */
extern int background ; /* operate in background mode */

#define DEFAULT_TIMEOUT  (15)
void Timeout( const char * c ) ;
struct s_timeout {
    int second ; /* timeout for clocks */
    int vol ;/* timeout (in seconds) for the volatile cached values */
    int stable ;/* timeout for the stable cached values */
    int dir ;/* timeout for directory listings */
} ;
extern struct s_timeout timeout ;

/* Server (Socket-based) interface */
/* message to owserver */
struct server_msg {
    int32_t version ;
    int32_t size ;
    int32_t type ;
    int32_t tempscale ;
    int32_t extra ;
} ;
/* message from owserver */
struct client_msg {
    int32_t version ;
    int32_t size ;
    int32_t ret ;
    int32_t format ;
    int32_t extra ;
} ;
enum msg_type { msg_nop, msg_read, msg_write, msg_dir, msg_get, msg_put, msg_parse, msg_fstat, } ;

int FromServer( int fd, struct client_msg * cm, char * msg, int size ) ;
void * FromServerAlloc( int fd, struct client_msg * cm ) ;
int ToServer( int fd, enum msg_type type, const char * path, const char * data, int datasize, int tscale ) ;

/* device display format */
enum deviceformat { fdi, fi, fdidc, fdic, fidc, fic } ;
extern enum deviceformat devform ;

/* -------------------------------------------- */
/* Interface-specific routines ---------------- */
struct interface_routines {
    /* assymetric read (only input, no slots emitted */
    int (* read) ( unsigned char * const bytes , const size_t num ) ;
    /* assymetric write (only output, no response obtained */
    int (* write) (const unsigned char * const bytes , const size_t num ) ;
    /* reset the interface -- actually the 1-wire bus */
    int (* reset ) ( const struct parsedname * const pn ) ;
    /* Bulk of search routine, after set ups for first or alarm or family */
    int (* next_both) (unsigned char * serialnumber, unsigned char search, const struct parsedname * const pn) ;
    /* Set the electrical level of the 1-wire bus */
    int (* level) (int new_level) ;
    /* Send a byte with bus power to follow */
    int (* PowerByte) (const unsigned char byte, const unsigned int delay) ;
    /* Send a 12V 480msec oulse to program EEPROM */
    int (* ProgramPulse) ( void ) ;
    /* send and recieve data*/
    int (* sendback_data) ( const unsigned char * const data , unsigned char * const resp , const int len ) ;
    /* select a device */
    int (* select) ( const struct parsedname * const pn ) ;

} ;
extern struct interface_routines iroutines ;

/* -------------------------------------------- */
/* start of program -- for statistics amd file atrtributes */
extern time_t start_time ;
extern time_t dir_time ; /* time of last directory scan */

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

extern struct timeval bus_time ;
extern struct timeval bus_pause ;
extern unsigned int bus_locks ;
extern unsigned int bus_unlocks ;
extern unsigned int crc8_tries ;
extern unsigned int crc8_errors ;
extern unsigned int crc16_tries ;
extern unsigned int crc16_errors ;
extern unsigned int read_timeout ;

/// mode bit flags
#define MODE_NORMAL                    0x00
#define MODE_OVERDRIVE                 0x01
#define MODE_STRONG5                   0x02
#define MODE_PROGRAM                   0x04
#define MODE_BREAK                     0x08

/* Globals for DS2480B state */
extern int UMode ;
extern int ULevel ;
extern int USpeed ;
extern int ProgramAvailable ;
enum adapter_type { adapter_DS9097=0, adapter_DS1410=1, adapter_DS9097U=3, adapter_LINK_Multi=6, adapter_LINK=7, adapter_DS9490=8, adapter_Bad=9, } ;
extern enum adapter_type Adapter ;
extern const char * adapter_name ;

/* Prototypes */
#define iREAD_FUNCTION( fname )  static int fname(int *, const struct parsedname *)
#define uREAD_FUNCTION( fname )  static int fname(unsigned int *, const struct parsedname * pn)
#define fREAD_FUNCTION( fname )  static int fname(FLOAT *, const struct parsedname * pn)
#define dREAD_FUNCTION( fname )  static int fname(DATE *, const struct parsedname * pn)
#define yREAD_FUNCTION( fname )  static int fname(int *, const struct parsedname * pn)
#define aREAD_FUNCTION( fname )  static int fname(char *buf, const size_t size, const off_t offset, const struct parsedname * pn)
#define bREAD_FUNCTION( fname )  static int fname(unsigned char *buf, const size_t size, const off_t offset, const struct parsedname * pn)

#define iWRITE_FUNCTION( fname )  static int fname(const int *, const struct parsedname * pn)
#define uWRITE_FUNCTION( fname )  static int fname(const unsigned int *, const struct parsedname * pn)
#define fWRITE_FUNCTION( fname )  static int fname(const FLOAT *, const struct parsedname * pn)
#define dWRITE_FUNCTION( fname )  static int fname(const DATE *, const struct parsedname * pn)
#define yWRITE_FUNCTION( fname )  static int fname(const int *, const struct parsedname * pn)
#define aWRITE_FUNCTION( fname )  static int fname(const char *buf, const size_t size, const off_t offset, const struct parsedname * pn)
#define bWRITE_FUNCTION( fname )  static int fname(const unsigned char *buf, const size_t size, const off_t offset, const struct parsedname * pn)

/* Prototypes for owlib.c -- libow overall control */
void LibSetup( void ) ;
int LibStart( void ) ;
int ComSetup( const char * busdev ) ;
int USBSetup( void ) ;
void LibClose( void ) ;

/* Initial sorting or the device and filetype lists */
void DeviceSort( void ) ;
  int devicecmp(const void * code , const void * dev ) ;
//  int filecmp(const void * name , const void * ex ) ;

/* Pasename processing -- URL/path comprehension */
int FS_ParsedName( const char * const fn , struct parsedname * const pn ) ;
  void FS_ParsedName_destroy( struct parsedname * const pn ) ;
  size_t FileLength( const struct parsedname * const pn ) ;
  size_t FullFileLength( const struct parsedname * const pn ) ;
int CheckPresence( const struct parsedname * const pn ) ;
void FS_devicename( char * const buffer, const size_t length, const unsigned char * const sn ) ;
void FS_devicefind( const char * code, struct parsedname * pn ) ;

const char * FS_dirname_state( const enum pn_state state ) ;
const char * FS_dirname_type( const enum pn_type type ) ;
void FS_DirName( char * buffer, const size_t size, const struct parsedname * const pn ) ;
int FS_FileName( char * name, const size_t size, const struct parsedname * pn ) ;


/* Utility functions */
unsigned char CRC8( const unsigned char * bytes , const int length ) ;
unsigned char CRC8seeded( const unsigned char * bytes , const int length , const int seed ) ;
  unsigned char CRC8compute( const unsigned char * bytes , const int length  ,const int seed ) ;
int CRC16( const unsigned char * bytes , const int length ) ;
int CRC16seeded( const unsigned char * bytes , const int length , const int seed ) ;
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

/* Serial port */
int COM_open( void ) ;
void COM_flush( void ) ;
void COM_close( void );
void COM_break( void ) ;

/* Cache  and Storage functions */
void Cache_Open( void ) ;
void Cache_Close( void ) ;
void Cache_Clear( void ) ;
int Cache_Add(          const void * data, const size_t datasize, const struct parsedname * const pn ) ;
int Cache_Add_Dir( const void * sn, const int dindex, const struct parsedname * const pn ) ;
int Cache_Add_Internal( const void * data, const size_t datasize, const struct internal_prop * ip, const struct parsedname * const pn ) ;
int Cache_Get(          void * data, size_t * dsize, const struct parsedname * const pn ) ;
int Cache_Get_Dir( void * sn, const int dindex, const struct parsedname * const pn ) ;
int Cache_Get_Internal( void * data, size_t * dsize, const struct internal_prop * ip, const struct parsedname * const pn ) ;
int Cache_Del(          const struct parsedname * const pn                                                                   ) ;
int Cache_Del_Dir( const int dindex, const struct parsedname * const pn ) ;
int Cache_Del_Internal( const struct internal_prop * ip, const struct parsedname * const pn ) ;
void FS_LoadPath( unsigned char * sn, const struct parsedname * const pn2 ) ;

int Simul_Test( const enum simul_type type, unsigned int msec, const struct parsedname * pn ) ;
int Simul_Clear( const enum simul_type type, const struct parsedname * pn ) ;

void LockSetup( void ) ;
void LockGet( const struct parsedname * const pn ) ;
void LockRelease( const struct parsedname * const pn ) ;

/* 1-wire lowlevel */
void UT_delay(const unsigned int len) ;
int LI_reset( const struct parsedname * const pn ) ;

/* High-level callback functions */
int FS_dir( void (* dirfunc)(const struct parsedname * const), const struct parsedname * const pn ) ;

int FS_write(const char *path, const char *buf, const size_t size, const off_t offset) ;
int FS_write_postparse(const char *path, const char *buf, const size_t size, const off_t offset, const struct parsedname * pn) ;

int FS_read(const char *path, char *buf, const size_t size, const off_t offset) ;
int FS_read_postparse(const char * path, char *buf, const size_t size, const off_t offset, const struct parsedname * pn ) ;

int FS_output_unsigned( unsigned int value, char * buf, const size_t size, const struct parsedname * pn ) ;
int FS_output_integer( int value, char * buf, const size_t size, const struct parsedname * pn ) ;
int FS_output_float( FLOAT value, char * buf, const size_t size, const struct parsedname * pn ) ;
int FS_output_date( DATE value, char * buf, const size_t size, const struct parsedname * pn ) ;

int FS_output_unsigned_array( unsigned int * values, char * buf, const size_t size, const struct parsedname * pn ) ;
int FS_output_integer_array( int * values, char * buf, const size_t size, const struct parsedname * pn ) ;
int FS_output_float_array( FLOAT * values, char * buf, const size_t size, const struct parsedname * pn ) ;
int FS_output_date_array( DATE * values, char * buf, const size_t size, const struct parsedname * pn ) ;

int FS_fstat(const char *path, struct stat *stbuf) ;

/* iteration functions for splitting writes to buffers */
int OW_read_paged( unsigned char * p, size_t size, size_t offset, const struct parsedname * const pn,
    size_t pagelen, int (*readfunc)(unsigned char *,const size_t,const size_t,const struct parsedname * const) ) ;
int OW_write_paged( const unsigned char * p, size_t size, size_t offset, const struct parsedname * const pn,
    size_t pagelen, int (*writefunc)(const unsigned char *,const size_t,const size_t,const struct parsedname * const) ) ;

/* Low-level functions
    slowly being abstracted and separated from individual
    interface type details
*/
int DS2480_baud( speed_t baud );
int DS2480_detect( void ) ;
#ifdef OW_PARPORT
int DS1410_detect( void ) ;
#endif /* OW_PARPORT */
int DS9097_detect( void ) ;
int BadAdapter_detect( void ) ;
int BUS_first(unsigned char * serialnumber, const struct parsedname * const pn) ;
int BUS_next(unsigned char * serialnumber, const struct parsedname * const pn) ;
int BUS_first_alarm(unsigned char * serialnumber, const struct parsedname * const pn) ;
int BUS_next_alarm(unsigned char * serialnumber, const struct parsedname * const pn) ;
int BUS_first_family(const unsigned char family, unsigned char * serialnumber, const struct parsedname * const pn ) ;

int BUS_select_low(const struct parsedname * const pn) ;
int BUS_sendout_cmd( const unsigned char * cmd , const int len ) ;
int BUS_send_cmd( const unsigned char * const cmd , const int len ) ;
int BUS_sendback_cmd( const unsigned char * const cmd , unsigned char * const resp , const int len ) ;
int BUS_send_data( const unsigned char * const data , const int len ) ;
int BUS_send_and_get( const unsigned char * const send, const size_t sendlength, unsigned char * const get, const size_t getlength ) ;
int BUS_readin_data( unsigned char * const data , const int len ) ;
int BUS_alarmverify(const struct parsedname * const pn) ;
int BUS_normalverify(const struct parsedname * const pn) ;

#define BUS_detect        DS2480_detect
#define BUS_changebaud    DS2480_changebaud
#define BUS_reset            (iroutines.reset)
#define BUS_read             (iroutines.read)
#define BUS_write            (iroutines.write)
#define BUS_sendback_data    (iroutines.sendback_data)
#define BUS_next_both        (iroutines.next_both)
#define BUS_level            (iroutines.level)
#define BUS_ProgramPulse     (iroutines.ProgramPulse)
#define BUS_PowerByte        (iroutines.PowerByte)
#define BUS_select           (iroutines.select)
#define BUS_databit       DS2480_databit
#define BUS_datablock     DS2480_datablock

void BUS_lock( void ) ;
void BUS_unlock( void ) ;

#endif /* OW_H */
