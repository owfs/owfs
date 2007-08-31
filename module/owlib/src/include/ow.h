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

    LICENSE (As of version 2.5p4 2-Oct-2006)
    owlib: GPL v2
    owfs, owhttpd, owftpd, owserver: GPL v2
    owshell(owdir owread owwrite owpresent): GPL v2
    owcapi (libowcapi): GPL v2
    owperl: GPL v2
    owtcl: LGPL v2
    owphp: GPL v2
    owpython: GPL v2
    owsim.tcl: GPL v2
    where GPL v2 is the "Gnu General License version 2"
    and "LGPL v2" is the "Lesser Gnu General License version 2"


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

#ifndef OW_H					/* tedious wrapper */
#define OW_H

#ifndef OWFS_CONFIG_H
#error Please make sure owfs_config.h is included *before* this header file
#endif

// Define this to avoid some VALGRIND warnings... (just for testing)
// Warning: This will partially remove the multithreaded support since ow_net.c
// will wait for a thread to complete before executing a new one.
//#define VALGRIND 1

#define _FILE_OFFSET_BITS   64

#ifdef HAVE_FEATURES_H
#include <features.h>
#endif /* HAVE_FEATURES_H */

#ifdef HAVE_FEATURE_TESTS_H
#include <feature_tests.h>
#endif /* HAVE_FEATURE_TESTS_H */

#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>			/* for stat */
#endif /* HAVE_SYS_STAT_H */

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>			/* for stat */
#endif /* HAVE_SYS_TYPES_H */

#include <sys/times.h>			/* for times */
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <signal.h>

#ifdef HAVE_STDINT_H
#include <stdint.h>				/* for bit twiddling */
#if OW_CYGWIN
#define _MSL_STDINT_H
#endif /* OW_CYGWIN */
#endif /* HAVE_STDINT_H */

#include <unistd.h>
#include <fcntl.h>

#ifndef __USE_XOPEN
#define __USE_XOPEN				/* for strptime fuction */
#include <time.h>
#undef __USE_XOPEN				/* for strptime fuction */
#else /* __USE_XOPEN */
#include <time.h>
#endif /* __USE_XOPEN */

#include <termios.h>
#include <errno.h>
#include <syslog.h>

#ifdef HAVE_GETOPT_H
#include <getopt.h>				/* for long options */
#endif /* HAVE_GETOPT_H */

#include <sys/uio.h>
#include <sys/time.h>			/* for gettimeofday */

#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif /* HAVE_SYS_SOCKET_H */

#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif /* HAVE_NETINET_IN_H */

#include <netdb.h>				/* addrinfo */

#ifdef HAVE_SYS_MKDEV_H
#include <sys/mkdev.h>			/* for major() */
#endif /* HAVE_SYS_MKDEV_H */

/* Can't include search.h when compiling owperl on Fedora Core 1. */
#ifndef SKIP_SEARCH_H
#ifndef __USE_GNU
#define __USE_GNU
#include <search.h>
#undef __USE_GNU
#else							/* __USE_GNU */
#include <search.h>
#endif							/* __USE_GNU */
#endif							/* SKIP_SEARCH_H */

/* Parport enabled uses two flags (one a holdover from the embedded work) */
#ifdef USE_NO_PARPORT
#undef OW_PARPORT
#endif							/* USE_NO_PARPORT */

#if OW_ZERO
/* Zeroconf / Bonjour */
#include "ow_dl.h"
#include "ow_dnssd.h"
#endif /* OW_ZERO */

/* Include some compatibility functions */
#include "compat.h"

/* Debugging and error messages separated out for readability */
#include "ow_debug.h"

#ifndef PATH_MAX
#define PATH_MAX 2048
#endif

/* Some errnos are not defined for MacOSX and gcc3.3 */
#ifndef EBADMSG
#define EBADMSG ENOMSG
#endif /* EBADMSG */

#ifndef EPROTO
#define EPROTO EIO
#endif /* EPROTO */


/* Floating point */
/* I hate to do this, making everything a double */
/* The compiler complains mercilessly, however */
/* 1-wire really is low precision -- float is more than enough */
/* FLOAT and DATE collides with cygwin windows include-files. */
typedef double _FLOAT;
typedef time_t _DATE;
typedef unsigned char BYTE;
typedef char ASCII;
typedef unsigned int UINT;
typedef int INT;

/* Include sone byte conversion convenience routines */
#include "ow_integer.h"

/* Directory blob separated out for readability */
#include "ow_dirblob.h"

/* Directory blob (strings) separated out for readability */
#include "ow_charblob.h"

#if OW_MT
#include <pthread.h>

extern pthread_mutex_t stat_mutex;
extern pthread_mutex_t cache_mutex;
extern pthread_mutex_t store_mutex;
extern pthread_mutex_t fstat_mutex;
extern pthread_mutex_t simul_mutex;
extern pthread_mutex_t dir_mutex;
extern pthread_mutex_t libusb_mutex;
extern pthread_mutex_t connin_mutex;

extern pthread_mutexattr_t *pmattr;
#ifdef __UCLIBC__
extern pthread_mutexattr_t mattr;
extern pthread_mutex_t uclibc_mutex;
#endif							/* __UCLIBC__ */
#define STATLOCK          pthread_mutex_lock(  &stat_mutex   )
#define STATUNLOCK        pthread_mutex_unlock(&stat_mutex   )
#define CACHELOCK         pthread_mutex_lock(  &cache_mutex  )
#define CACHEUNLOCK       pthread_mutex_unlock(&cache_mutex  )
#define STORELOCK         pthread_mutex_lock(  &store_mutex  )
#define STOREUNLOCK       pthread_mutex_unlock(&store_mutex  )
#define FSTATLOCK         pthread_mutex_lock(  &fstat_mutex  )
#define FSTATUNLOCK       pthread_mutex_unlock(&fstat_mutex  )
#define SIMULLOCK         pthread_mutex_lock(  &simul_mutex  )
#define SIMULUNLOCK       pthread_mutex_unlock(&simul_mutex  )
#define DIRLOCK           pthread_mutex_lock(  &dir_mutex    )
#define DIRUNLOCK         pthread_mutex_unlock(&dir_mutex    )
#define LIBUSBLOCK        pthread_mutex_lock(  &libusb_mutex    )
#define LIBUSBUNLOCK      pthread_mutex_unlock(&libusb_mutex    )
#define CONNINLOCK        pthread_mutex_lock(  &connin_mutex    )
#define CONNINUNLOCK      pthread_mutex_unlock(&connin_mutex    )
#define BUSLOCK(pn)       BUS_lock(pn)
#define BUSUNLOCK(pn)     BUS_unlock(pn)
#define BUSLOCKIN(in)       BUS_lock_in(in)
#define BUSUNLOCKIN(in)     BUS_unlock_in(in)
#ifdef __UCLIBC__
#define UCLIBCLOCK     pthread_mutex_lock(  &uclibc_mutex)
#define UCLIBCUNLOCK   pthread_mutex_unlock(&uclibc_mutex)
#else							/* __UCLIBC__ */
#define UCLIBCLOCK
#define UCLIBCUNLOCK
#endif							/* __UCLIBC__ */

#else							/* OW_MT */
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
#define DIRLOCK
#define DIRUNLOCK
#define UCLIBCLOCK
#define UCLIBCUNLOCK
#define LIBUSBLOCK
#define LIBUSBUNLOCK
#define UCLIBCLOCK
#define UCLIBCUNLOCK
#define CONNINLOCK
#define CONNINUNLOCK
#define BUSLOCK(pn)
#define BUSUNLOCK(pn)
#define BUSLOCKIN(in)
#define BUSUNLOCKIN(in)
#endif							/* OW_MT */

/*
    OW -- One Wire
    Global variables -- each invokation will have it's own data
*/

/* command line options */
/* These are the owlib-specific options */
#define OWLIB_OPT "m:c:f:p:s:h::u::d:t:CFRKVP:"
extern const struct option owopts_long[];
enum opt_program { opt_owfs, opt_server, opt_httpd, opt_ftpd, opt_tcl,
	opt_swig, opt_c,
};
int owopt(const int c, const char *arg);
int owopt_packed(const char *params);

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

enum ag_index { ag_numbers, ag_letters, };
enum ag_combined { ag_separate, ag_aggregate, ag_mixed, };
/* aggregate defines array properties */
struct aggregate {
	int elements;				/* Maximum number of elements */
	enum ag_index letters;		/* name them with letters or numbers */
	enum ag_combined combined;	/* Combined bitmaps properties, or separately addressed */
};
extern struct aggregate Asystem;	/* for remote directories */

	/* property format, controls web display */
/* Some explanation of ft_format:
     Each file type is either a device (physical 1-wire chip or virtual statistics container).
     or a file (property).
     The devices act as directories of properties.
     The files are either properties of the device, or sometimes special directories themselves.
     If properties, they can be integer, text, etc or special directory types.
     There is also the directory type, ft_directory reflects a branch type, which restarts the parsing process.
*/
enum ft_format {
	ft_directory,
	ft_subdir,
	ft_integer,
	ft_unsigned,
	ft_float,
	ft_ascii,
	ft_vascii,					// variable length ascii -- must be read and measured.
	ft_binary,
	ft_yesno,
	ft_date,
	ft_bitfield,
	ft_temperature,
	ft_tempgap,
};
	/* property changability. Static unchanged, Stable we change, Volatile changes */
enum fc_change { fc_local, fc_static, fc_stable, fc_Astable, fc_volatile,
	fc_Avolatile, fc_second, fc_statistic, fc_persistent, fc_directory,
	fc_presence,
};

/* Predeclare parsedname */
struct parsedname;

/* Predeclare one_wire_query */
struct one_wire_query;

/* predeclare connection_in/out */
struct connection_in;
struct connection_out;

/* Exposed connection info */
extern int outdevices;
extern int indevices;

/* Maximum length of a file or directory name, and extension */
#define OW_NAME_MAX      (32)
#define OW_EXT_MAX       (6)
#define OW_FULLNAME_MAX  (OW_NAME_MAX+OW_EXT_MAX)
#define OW_DEFAULT_LENGTH (128)

#define PROPERTY_LENGTH_INTEGER   12
#define PROPERTY_LENGTH_UNSIGNED  12
#define PROPERTY_LENGTH_BITFIELD  12
#define PROPERTY_LENGTH_FLOAT     12
#define PROPERTY_LENGTH_TEMP      12
#define PROPERTY_LENGTH_TEMPGAP   12
#define PROPERTY_LENGTH_DATE      24
#define PROPERTY_LENGTH_YESNO      1
#define PROPERTY_LENGTH_STRUCTURE 30
#define PROPERTY_LENGTH_DIRECTORY  8
#define PROPERTY_LENGTH_SUBDIR     0

#define NO_READ_FUNCTION NULL
#define NO_WRITE_FUNCTION NULL
/* filetype gives -file types- for chip properties */
struct filetype {
	char *name;
	int suglen;					// length of field
	struct aggregate *ag;		// struct pointer for aggregate
	enum ft_format format;		// type of data
	enum fc_change change;		// volatility
	int (*read)  ( struct one_wire_query * ) ; // read callback function
	int (*write) ( struct one_wire_query * ) ; // write callback function
	union {
		void *v;
		int i;
		UINT u;
		_FLOAT f;
		size_t s;
		BYTE c;
	} data;						// extra data pointer (used for separating similar but differently name functions)
};
#define NFT(ft) ((int)(sizeof(ft)/sizeof(struct filetype)))

/* --------- end Filetype -------------------- */
/* ------------------------------------------- */

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
extern void *Tree[6];

	/* supports RESUME command */
#define DEV_resume  0x0001
	/* can trigger an alarm */
#define DEV_alarm   0x0002
	/* support OVERDRIVE */
#define DEV_ovdr    0x0004
	/* responds to simultaneous temperature convert 0x44 */
#define DEV_temp    0x8000
	/* responds to simultaneous voltage convert 0x3C */
#define DEV_volt    0x4000
	/* supports CHAIN command */
#define DEV_chain   0x2000


struct device {
	const char *code;
	char *name;
	uint32_t flags;
	int nft;
	struct filetype *ft;
};

#define DeviceHeader( chip )    extern struct device d_##chip
//#define DeviceEntry( code , chip )  { #code, #chip, 0, NFT(chip), chip } ;
/* Entries for struct device */
/* Cannot set the 3rd element (number of filetypes) at compile time because
   filetype arrays aren;t defined at this point */
#define DeviceEntryExtended( code , chip , flags )  struct device d_##chip = { #code, #chip, flags ,  NFT(chip), chip }
#define DeviceEntry( code , chip )  DeviceEntryExtended( code, chip, 0 )

/* Bad bad C library */
/* implementation of tfind, tsearch returns an opaque structure */
/* you have to know that the first element is a pointer to your data */
struct device_opaque {
	struct device *key;
	void *other;
};

/* Must be sorted for bsearch */
//extern struct device * Devices[] ;
//extern size_t nDevices ;
extern struct device NoDevice;
extern struct device *DeviceSimultaneous;
extern struct device *DeviceThermostat;

/* ---- end device --------------------- */
/* ------------------------------------- */


/* ------------------------}
struct devlock {
    BYTE sn[8] ;
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

/* Semi-global information (for remote control) */
	/* bit0: cacheenabled  bit1: return bus-list */
	/* presencecheck */
	/* tempscale */
	/* device format */
extern int32_t SemiGlobal;

/* Unique token for owserver loop checks */
union antiloop {
	struct {
		pid_t pid;
		clock_t clock;
	} simple;
	BYTE uuid[16];
};

/* Global information (for local control) */
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
	char *SimpleBusName;
	int max_clients;			// for ftp
	int autoserver;
	size_t cache_size;			// max cache size (or 0 for no max) ;
	/* Special parameter to trigger William Robison <ibutton@n952.dyndns.ws> timings */
	int altUSB;
	/* timeouts -- order must match ow_opt.c values for correct indexing */
	int timeout_volatile;
	int timeout_stable;
	int timeout_directory;
	int timeout_presence;
	int timeout_serial;
	int timeout_usb;
	int timeout_network;
	int timeout_server;
	int timeout_ftp;
	int timeout_persistent_low;
	int timeout_persistent_high;
	int clients_persistent_low;
	int clients_persistent_high;
	int pingcrazy;
	int no_dirall;
	int no_get;
	int no_persistence;
	int eightbit_serial;
};
extern struct global Global;

extern int shutdown_in_progress;

struct buspath {
	BYTE sn[8];
	BYTE branch;
};

struct devlock {
	BYTE sn[8];
	UINT users;
	pthread_mutex_t lock;
};

#define EXTENSION_BYTE	-2
#define EXTENSION_ALL	-1
#define EXTENSION_ALL_SEPARATE	-1
#define EXTENSION_ALL_MIXED		-1
#define EXTENSION_ALL_AGGREGATE	-1

enum pn_type { pn_real =
		0, pn_statistics, pn_system, pn_settings, pn_structure
};
enum pn_state { pn_normal = 0, pn_uncached = 1, pn_alarm = 2, pn_text =
		4, pn_bus = 8, pn_buspath = 16,
};
struct parsedname {
	char *path;					// text-more device name
	char *path_busless;			// pointer to path without first bus
	int bus_nr;
	enum pn_type type;			// global branch
	enum pn_state state;		// global branch
	BYTE sn[8];					// 64-bit serial number
	struct device *dev;			// 1-wire device
	struct filetype *ft;		// device property
	int extension;				// numerical extension (for array values) or -1
	struct filetype *subdir;	// in-device grouping
	UINT pathlength;			// DS2409 branching depth
	struct buspath *bp;			// DS2409 branching route
	struct connection_in *indevice;	// Global indevice at definition time
	struct connection_in *in;
	uint32_t sg;				// more state info, packed for network transmission
	struct devlock **lock;		// need to clear dev lock?
	int tokens;					/* for anti-loop work */
	BYTE *tokenstring;			/* List of tokens from owservers passed */
};

enum simul_type { simul_temp, simul_volt, };

/* ---- end Parsedname ----------------- */
/* ------------------------------------- */

#include "ow_onewirequery.h"

/* Delay for clearing buffer */
#define    WASTE_TIME    (2)

/* device display format */
enum deviceformat { fdi, fi, fdidc, fdic, fidc, fic };
/* Gobal temperature scale */
enum temp_type { temp_celsius, temp_fahrenheit, temp_kelvin, temp_rankine,
};
_FLOAT Temperature(_FLOAT C, const struct parsedname *pn);
_FLOAT TemperatureGap(_FLOAT C, const struct parsedname *pn);
_FLOAT fromTemperature(_FLOAT T, const struct parsedname *pn);
_FLOAT fromTempGap(_FLOAT T, const struct parsedname *pn);
const char *TemperatureScaleName(enum temp_type t);

extern void set_signal_handlers(void (*exit_handler)
								 (int signo, siginfo_t * info,
								  void *context));

#ifndef SI_FROMUSER
#define SI_FROMUSER(siptr)      ((siptr)->si_code <= 0)
#endif
#ifndef SI_FROMKERNEL
#define SI_FROMKERNEL(siptr)    ((siptr)->si_code > 0)
#endif


/* Server (Socket-based) interface */
enum msg_classification {
	msg_error,
	msg_nop,
	msg_read,
	msg_write,
	msg_dir,
	msg_size,					// No longer used, leave here to compatibility
	msg_presence,
	msg_dirall,
	msg_get,
};
/* message to owserver */
struct server_msg {
	int32_t version;
	int32_t payload;
	int32_t type;
	int32_t sg;
	int32_t size;
	int32_t offset;
};

/* message to client */
struct client_msg {
	int32_t version;
	int32_t payload;
	int32_t ret;
	int32_t sg;
	int32_t size;
	int32_t offset;
};

struct serverpackage {
	ASCII *path;
	BYTE *data;
	size_t datasize;
	BYTE *tokenstring;
	size_t tokens;
};

#define Servermessage       (((int32_t)1)<<16)
#define isServermessage( version )    (((version)&Servermessage)!=0)
#define Servertokens(version)    ((version)&0xFFFF)
/* -------------------------------------------- */
/* start of program -- for statistics amd file atrtributes */
extern time_t start_time;
extern time_t dir_time;			/* time of last directory scan */

/* Prototypes */
#define  READ_FUNCTION( fname )  static int fname( struct one_wire_query * owq )
#define  WRITE_FUNCTION( fname )  static int fname( struct one_wire_query * owq )

/* Prototypes for owlib.c -- libow overall control */
void LibSetup(enum opt_program op);
int LibStart(void);
void LibClose(void);

/* Initial sorting or the device and filetype lists */
void DeviceSort(void);
void DeviceDestroy(void);
//  int filecmp(const void * name , const void * ex ) 
/* Pasename processing -- URL/path comprehension */
int filecmp(const void *name, const void *ex);
int FS_ParsedNamePlus(const char *path, const char *file,
					  struct parsedname *pn);
int FS_ParsedName(const char *fn, struct parsedname *pn);
int FS_ParsedName_BackFromRemote(const char *fn, struct parsedname *pn);
void FS_ParsedName_destroy(struct parsedname *pn);
size_t FileLength(const struct parsedname *pn);
size_t FullFileLength(const struct parsedname *pn);
int CheckPresence(struct parsedname *pn);
void FS_devicename(char *buffer, const size_t length, const BYTE * sn,
				   const struct parsedname *pn);
void FS_devicefind(const char *code, struct parsedname *pn);
void FS_devicefindhex(BYTE f, struct parsedname *pn);

int FS_dirname_state(char *buffer, const size_t length,
					 const struct parsedname *pn);
int FS_dirname_type(char *buffer, const size_t length,
					const struct parsedname *pn);
void FS_DirName(char *buffer, const size_t size,
				const struct parsedname *pn);
int FS_FileName(char *name, const size_t size,
				const struct parsedname *pn);

/* Utility functions */
BYTE CRC8(const BYTE * bytes, const size_t length);
BYTE CRC8seeded(const BYTE * bytes, const size_t length, const UINT seed);
BYTE CRC8compute(const BYTE * bytes, const size_t length, const UINT seed);
int CRC16(const BYTE * bytes, const size_t length);
int CRC16seeded(const BYTE * bytes, const size_t length, const UINT seed);
BYTE char2num(const char *s);
BYTE string2num(const char *s);
char num2char(const BYTE n);
void num2string(char *s, const BYTE n);
void string2bytes(const char *str, BYTE * b, const int bytes);
void bytes2string(char *str, const BYTE * b, const int bytes);
int UT_getbit(const BYTE * buf, const int loc);
int UT_get2bit(const BYTE * buf, const int loc);
void UT_setbit(BYTE * buf, const int loc, const int bit);
void UT_set2bit(BYTE * buf, const int loc, const int bits);
void UT_fromDate(const _DATE D, BYTE * data);
_DATE UT_toDate(const BYTE * date);
int FS_busless(char *path);

/* Cache  and Storage functions */
#include "ow_cache.h"
void FS_LoadPath(BYTE * sn, const struct parsedname *pn);

int Simul_Test(const enum simul_type type, const struct parsedname *pn);
int FS_poll_convert(const struct parsedname *pn);

// ow_locks.c
void LockSetup(void);
int LockGet(const struct parsedname *pn);
void LockRelease(const struct parsedname *pn);

// ow_api.c
int OWLIB_can_init_start(void);
void OWLIB_can_init_end(void);
int OWLIB_can_access_start(void);
void OWLIB_can_access_end(void);
int OWLIB_can_finish_start(void);
void OWLIB_can_finish_end(void);

/* 1-wire lowlevel */
void UT_delay(const UINT len);
void UT_delay_us(const unsigned long len);

ssize_t tcp_read(int fd, void *vptr, size_t n, const struct timeval *ptv);
void tcp_read_flush(int fd) ;
int tcp_wait(int fd, const struct timeval *ptv);
int ClientAddr(char *sname, struct connection_in *in);
int ClientConnect(struct connection_in *in);
void ServerProcess(void (*HandlerRoutine) (int fd),
				   void (*Exit) (int errcode));
void FreeClientAddr(struct connection_in *in);
int ServerOutSetup(struct connection_out *out);

int OW_ArgNet(const char *arg);
int OW_ArgServer(const char *arg);
int OW_ArgUSB(const char *arg);
int OW_ArgDevice(const char *arg);
int OW_ArgGeneric(const char *arg);

void ow_help(const char *arg);

void update_max_delay(const struct parsedname *pn);

int ServerPresence(const struct parsedname *pn);
int ServerRead(struct one_wire_query * owq);
int ServerWrite(struct one_wire_query * owq);
int ServerDir(void (*dirfunc) (void *, const struct parsedname *),
			  void *v, const struct parsedname *pn, uint32_t * flags);

/* High-level callback functions */
int FS_dir(void (*dirfunc) (void *, const struct parsedname *),
		   void *v, const struct parsedname *pn);
int FS_dir_remote(void (*dirfunc) (void *, const struct parsedname *),
				  void *v, const struct parsedname *pn, uint32_t * flags);

int FS_write(const char *path, const char *buf, const size_t size,
			 const off_t offset);
int FS_write_postparse(struct one_wire_query * owq);

int FS_read(const char *path, char *buf, const size_t size,
			const off_t offset);
//int FS_read_postparse(char *buf, const size_t size, const off_t offset,
//					  const struct parsedname *pn);
int FS_read_postparse(struct one_wire_query * owq) ;
int FS_read_distribute(struct one_wire_query * owq);
int FS_read_fake(struct one_wire_query * owq);
int FS_read_tester(struct one_wire_query * owq);
int FS_r_aggregate_all(struct one_wire_query * owq) ;
int Fowq_output_offset_and_size(char * string, size_t length, struct one_wire_query * owq) ;
int Fowq_output_offset_and_size_z(char * string, struct one_wire_query * owq) ;

int FS_fstat(const char *path, struct stat *stbuf);
int FS_fstat_postparse(struct stat *stbuf, const struct parsedname *pn);

/* iteration functions for splitting writes to buffers */
int OW_readwrite_paged(struct one_wire_query * owq, size_t page, size_t pagelen,
				  int (*readwritefunc) (BYTE *, size_t, off_t, struct parsedname * )) ;
int OWQ_readwrite_paged(struct one_wire_query * owq, size_t page, size_t pagelen,
                        int (*readwritefunc) (struct one_wire_query *, size_t, size_t )) ;

int OW_r_mem_simple(struct one_wire_query * owq, size_t page, size_t pagesize ) ;
int OW_r_mem_crc16_A5(struct one_wire_query * owq, size_t page, size_t pagesize) ;
int OW_r_mem_crc16_AA(struct one_wire_query * owq, size_t page, size_t pagesize) ;
int OW_r_mem_crc16_F0(struct one_wire_query * owq, size_t page, size_t pagesize) ;
int OW_r_mem_toss8(struct one_wire_query * owq, size_t page, size_t pagesize) ;
int OW_r_mem_counter_bytes(BYTE * extra, size_t page, size_t pagesize, struct parsedname * pn) ;

void BUS_lock(const struct parsedname *pn);
void BUS_unlock(const struct parsedname *pn);
void BUS_lock_in(struct connection_in *in);
void BUS_unlock_in(struct connection_in *in);

#define CACHE_MASK     ( (UINT) 0x00000001 )
#define CACHE_BIT      0
#define BUSRET_MASK    ( (UINT) 0x00000002 )
#define BUSRET_BIT     1
#define PERSISTENT_MASK    ( (UINT) 0x00000004 )
#define PERSISTENT_BIT     2
#define TEMPSCALE_MASK ( (UINT) 0x00FF0000 )
#define TEMPSCALE_BIT  16
#define DEVFORMAT_MASK ( (UINT) 0xFF000000 )
#define DEVFORMAT_BIT  24
#define IsLocalCacheEnabled(ppn)  ( ((ppn)->sg &  CACHE_MASK) )
#define IsPersistent(ppn)         ( ((ppn)->sg & PERSISTENT_MASK) )
#define SetPersistent(ppn,b)      UT_Setbit(((ppn)->sg),PERSISTENT_BIT,(b))
#define TemperatureScale(ppn)     ( (enum temp_type) (((ppn)->sg & TEMPSCALE_MASK) >> TEMPSCALE_BIT) )
#define SGTemperatureScale(sg)    ( (enum temp_type)(((sg) & TEMPSCALE_MASK) >> TEMPSCALE_BIT) )
#define DeviceFormat(ppn)         ( (enum deviceformat) (((ppn)->sg & DEVFORMAT_MASK) >> DEVFORMAT_BIT) )
#define set_semiglobal(s, mask, bit, val) do { *(s) = (*(s) & ~(mask)) | ((val)<<bit); } while(0)

#define IsDir( pn )    ( ((pn)->dev)==NULL \
                      || ((pn)->ft)==NULL  \
                      || ((pn)->ft)->format==ft_subdir \
                      || ((pn)->ft)->format==ft_directory )
#define NotUncachedDir(pn)    ( (((pn)->state)&pn_uncached) == 0 )
#define  IsUncachedDir(pn)    ( ! NotUncachedDir(pn) )
#define    NotAlarmDir(pn)    ( (((pn)->state)&pn_alarm) == 0 )
#define     IsAlarmDir(pn)    ( ! NotAlarmDir(pn) )
#define     NotRealDir(pn)    ( ((pn)->type) != pn_real )
#define      IsRealDir(pn)    ( ((pn)->type) == pn_real )

#define KnownBus(pn)          ((((pn)->state) & pn_bus) != 0 )
#define SetKnownBus(bus_number,pn)  do { (pn)->state |= pn_bus; \
                                        (pn)->bus_nr=(bus_number); \
                                        (pn)->in=find_connection_in(bus_number); \
                                    } while(0)
#define UnsetKnownBus(pn)           do { (pn)->state &= ~pn_bus; \
                                        (pn)->bus_nr=-1; \
                                        (pn)->in=NULL; \
                                    } while(0)

#define ShouldReturnBusList(ppn)  ( ((ppn)->sg & BUSRET_MASK) )
#define SpecifiedBus(pn)          ((((pn)->state) & pn_buspath) != 0 )
#define SetSpecifiedBus(bus_number,pn) do { (pn)->state |= pn_buspath ; SetKnownBus(bus_number,pn); } while(0)

#define RootNotBranch(pn)         (((pn)->pathlength)==0)
#define BYTE_MASK(x)        ((x)&0xFF)
#define BYTE_INVERSE(x)     BYTE_MASK((x)^0xFF)
#define LOW_HIGH_ADDRESS(x)         BYTE_MASK(x),BYTE_MASK((x)>>8)

#endif							/* OW_H */
