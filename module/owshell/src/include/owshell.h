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

#ifndef OWSHELL_H  /* tedious wrapper */
#define OWSHELL_H

#include "config.h"
#include "owfs_config.h"

// Define this to avoid some VALGRIND warnings... (just for testing)
// Warning: This will partially remove the multithreaded support since ow_net.c
// will wait for a thread to complete before executing a new one.
//#define VALGRIND 1

#define _FILE_OFFSET_BITS   64
#ifdef HAVE_FEATURES_H
    #include <features.h>
#endif
#ifdef HAVE_FEATURE_TESTS_H
    #include <feature_tests.h>
#endif
#ifdef HAVE_SYS_STAT_H
    #include <sys/stat.h> /* for stat */
#endif
#ifdef HAVE_SYS_TYPES_H
    #include <sys/types.h> /* for stat */
#endif
#include <sys/times.h> /* for times */
#include <ctype.h>
#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <signal.h>
#ifdef HAVE_STDINT_H
 #include <stdint.h> /* for bit twiddling */
 #if OW_CYGWIN
  #define _MSL_STDINT_H
 #endif
#endif

#include <unistd.h>
#ifdef HAVE_GETOPT_H
    #ifdef __GNU_LIBRARY__
        #include <getopt.h>
    #else /* __GNU_LIBRARY__ */
        #define __GNU_LIBRARY__
            #include <getopt.h>
        #undef __GNU_LIBRARY__
    #endif /* __GNU_LIBRARY__ */
#endif /* HAVE_GETOPT_H */
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

#include <sys/uio.h>
#include <sys/time.h> /* for gettimeofday */
#ifdef HAVE_SYS_SOCKET_H
 #include <sys/socket.h>
#endif
#ifdef HAVE_NETINET_IN_H
 #include <netinet/in.h>
#endif
#include <netdb.h> /* addrinfo */

/* Can't include search.h when compiling owperl on Fedora Core 1. */
#ifndef SKIP_SEARCH_H
 #ifndef __USE_GNU
  #define __USE_GNU
  #include <search.h>
  #undef __USE_GNU
 #else /* __USE_GNU */
  #include <search.h>
 #endif /* __USE_GNU */
#endif /* SKIP_SEARCH_H */

#if OW_ZERO
/* Zeroconf / Bonjour */
#include "ow_dl.h"
#include "ow_dnssd.h"
#endif

/* Include some compatibility functions */
#include "compat.h"

/* Debugging and error messages separated out for readability */
#include "ow_debug.h"


/* Floating point */
/* I hate to do this, making everything a double */
/* The compiler complains mercilessly, however */
/* 1-wire really is low precision -- float is more than enough */
typedef double          _FLOAT ;
typedef time_t          _DATE ;
typedef unsigned char   BYTE ;
typedef char            ASCII ;
typedef unsigned int    UINT ;
typedef int             INT ;

/*
    OW -- One Wire
    Global variables -- each invokation will have it's own data
*/

/* command line options */
/* These are the owlib-specific options */
#define OWLIB_OPT "m:c:f:p:s:hu::d:t:CFRKVP:"
extern const struct option owopts_long[] ;
enum opt_program { opt_owfs, opt_server, opt_httpd, opt_ftpd, opt_tcl, opt_swig, opt_c, } ;
void owopt( const int c , const char * arg ) ;

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

/* predeclare connection_in/out */
struct connection_in ;

/* Exposed connection info */
extern int indevices ;

/* Maximum length of a file or directory name, and extension */
#define OW_NAME_MAX      (32)
#define OW_EXT_MAX       (6)
#define OW_FULLNAME_MAX  (OW_NAME_MAX+OW_EXT_MAX)
#define OW_DEFAULT_LENGTH (128)


/* Semi-global information (for remote control) */
    /* bit0: cacheenabled  bit1: return bus-list */
    /* presencecheck */
    /* tempscale */
    /* device format */
extern int32_t SemiGlobal ;

/* Unique token for owserver loop checks */
union antiloop {
    struct {
        pid_t pid ;
        clock_t clock ;
    } simple ;
    BYTE uuid[16] ;
} ;

/* Global information (for local control) */
struct global {
    int announce_off ; // use zeroconf?
    ASCII * announce_name ;
    enum opt_program opt ;
    ASCII * progname ;
    union antiloop Token ;
    int want_background ;
    int now_background ;
    int error_level ;
    int error_print ;
    int readonly ;
    char * SimpleBusName ;
    int max_clients ; // for ftp
    int autoserver ;
    /* Special parameter to trigger William Robison <ibutton@n952.dyndns.ws> timings */
    int altUSB ;
    /* timeouts -- order must match ow_opt.c values for correct indexing */
    int timeout_volatile ;
    int timeout_stable ;
    int timeout_directory ;
    int timeout_presence ;
    int timeout_serial ;
    int timeout_usb ;
    int timeout_network ;
    int timeout_server ;
    int timeout_ftp ;

} ;
extern struct global Global ;


/* device display format */
enum deviceformat { fdi, fi, fdidc, fdic, fidc, fic } ;
/* Gobal temperature scale */
enum temp_type { temp_celsius, temp_fahrenheit, temp_kelvin, temp_rankine, } ;
const char *TemperatureScaleName(enum temp_type t) ;

/* Server (Socket-based) interface */
enum msg_classification {
    msg_error,
    msg_nop,
    msg_read,
    msg_write,
    msg_dir,
    msg_size, // No longer used, leave here to compatibility
    msg_presence,
} ;
/* message to owserver */
struct server_msg {
    int32_t version ;
    int32_t payload ;
    int32_t type ;
    int32_t sg ;
    int32_t size ;
    int32_t offset ;
} ;

/* message to client */
struct client_msg {
    int32_t version ;
    int32_t payload ;
    int32_t ret ;
    int32_t sg ;
    int32_t size ;
    int32_t offset ;
} ;

struct serverpackage {
    ASCII * path ;
    BYTE * data ;
    size_t datasize ;
    BYTE * tokenstring ;
    size_t tokens ;
} ;

#define Servermessage       (((int32_t)1)<<16)
#define isServermessage( version )    (((version)&Servermessage)!=0)
#define Servertokens(version)    ((version)&0xFFFF)
/* -------------------------------------------- */
/* start of program -- for statistics amd file atrtributes */
extern time_t start_time ;
extern time_t dir_time ; /* time of last directory scan */

ssize_t tcp_read(int fd, void *vptr, size_t n, const struct timeval * ptv ) ;
int ClientAddr(  char * sname, struct connection_in * in ) ;
int ClientConnect( void ) ;
void FreeClientAddr(  struct connection_in * in ) ;

void OW_ArgNet( const char * arg ) ;
void Setup(void) ;
void Cleanup( void ) ;
void ow_help( void ) ;
void OW_Browse( void ) ;

void Server_detect( void  ) ;
int ServerRead( ASCII * path ) ;
int ServerWrite( ASCII * path, ASCII * data ) ;
int ServerDir( ASCII * path ) ;
int ServerPresence( ASCII * path ) ;

#define CACHE_MASK     ( (UINT) 0x00000001 )
#define CACHE_BIT      0
#define BUSRET_MASK    ( (UINT) 0x00000002 )
#define BUSRET_BIT     1
#define TEMPSCALE_MASK ( (UINT) 0x00FF0000 )
#define TEMPSCALE_BIT  16
#define DEVFORMAT_MASK ( (UINT) 0xFF000000 )
#define DEVFORMAT_BIT  24
#define set_semiglobal(s, mask, bit, val) do { *(s) = (*(s) & ~(mask)) | ((val)<<bit); } while(0)

struct connection_in ;

/* placed in iroutines.flags */
#define ADAP_FLAG_overdrive     0x00000001
#define ADAP_FLAG_2409path      0x00000010
#define ADAP_FLAG_dirgulp       0x00000100

struct connin_server {
    char * host ;
    char * service ;
    struct addrinfo * ai ;
    struct addrinfo * ai_ok ;
    char * type ; // for zeroconf
    char * domain ; // for zeroconf
    char * fqdn ;
} ;

enum e_reconnect {
    reconnect_bad = -1 ,
    reconnect_ok = 0 ,
    reconnect_error = 5 ,
} ;

struct device_search {
    int LastDiscrepancy ; // for search
    int LastDevice ; // for search
    BYTE sn[8] ;
    BYTE search ;
} ;

struct connection_in {
    struct connection_in * next ;
    int index ;
    char * name ;
    int fd ;
  
    char * adapter_name ;

    union {
        struct connin_server server ;
    } connin ;
} ;

extern struct connection_in * indevice ;

void FreeIn( void ) ;
void DelIn( struct connection_in * in ) ;

struct connection_in * NewIn( void ) ;

#endif /* OWSHELL_H */
