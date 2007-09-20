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

/* Many mutexes separated out for readability */
#include "ow_mutexes.h"

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
extern int count_outbound_connections;
extern int count_inbound_connections;

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
/* Separated out into ow_global.h for readability */
#include "ow_global.h"

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
	struct device *selected_device;			// 1-wire device
	struct filetype *selected_filetype;		// device property
	int extension;				// numerical extension (for array values) or -1
	struct filetype *subdir;	// in-device grouping
	UINT pathlength;			// DS2409 branching depth
	struct buspath *bp;			// DS2409 branching route
	struct connection_in *head_inbound_list;	// Global head_inbound_list at definition time
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
/* Separated out to ow_functions.h for clarity */
#include "ow_functions.h"

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

#define IsDir( pn )    ( ((pn)->selected_device)==NULL \
                      || ((pn)->selected_filetype)==NULL  \
                      || ((pn)->selected_filetype)->format==ft_subdir \
                      || ((pn)->selected_filetype)->format==ft_directory )
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
