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
#endif							/* HAVE_FEATURES_H */

#ifdef HAVE_FEATURE_TESTS_H
#include <feature_tests.h>
#endif							/* HAVE_FEATURE_TESTS_H */

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>			/* for stat */
#endif							/* HAVE_SYS_TYPES_H */

#ifdef HAVE_SYS_TIMES_H
#include <sys/times.h>			/* for times */
#endif							/* HAVE_SYS_TIMES_H */
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#ifdef STRING_WITH_STRINGS
# include <string.h>
# include <strings.h>			/* for strcasecmp */
#else
# ifdef HAVE_STRING_H
#  include <string.h>
# else
#  ifdef HAVE_STRINGS_H
#   include <strings.h>			/* for strcasecmp */
#  endif
# endif
#endif
#include <dirent.h>
#include <signal.h>

#ifdef HAVE_STDINT_H
#include <stdint.h>				/* for bit twiddling */
#if OW_CYGWIN
#define _MSL_STDINT_H
#endif							/* OW_CYGWIN */
#endif							/* HAVE_STDINT_H */

#include <unistd.h>
#include <fcntl.h>

#ifndef __USE_XOPEN
#define __USE_XOPEN				/* for strptime fuction */
#include <time.h>
#undef __USE_XOPEN				/* for strptime fuction */
#else							/* __USE_XOPEN */
#include <time.h>
#endif							/* __USE_XOPEN */

#ifdef HAVE_TERMIOS_H
#include <termios.h>
#endif							/* HAVE_TERMIOS_H */

#include <errno.h>

#ifdef HAVE_SYSLOG_H
#include <syslog.h>
#endif							/* HAVE_SYSLOG_H */

#ifdef HAVE_GETOPT_H
#include <getopt.h>				/* for long options */
#endif							/* HAVE_GETOPT_H */

#ifdef HAVE_SYS_UIO_H
#include <sys/uio.h>
#endif

#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>			/* for gettimeofday */
#endif							/* HAVE_SYS_TIME_H */

#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>			/* for stat */
#endif							/* HAVE_SYS_STAT_H */

#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif							/* HAVE_SYS_SOCKET_H */

#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif							/* HAVE_NETINET_IN_H */

#ifndef INET_ADDRSTRLEN
#define INET_ADDRSTRLEN 16
#endif

#ifdef HAVE_NETDB_H
#include <netdb.h>				/* for getaddrinfo */
#endif							/* HAVE_NETDB_H */

#ifdef HAVE_SYS_MKDEV_H
#include <sys/mkdev.h>			/* for major() */
#endif							/* HAVE_SYS_MKDEV_H */

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
#if !OW_CYGWIN
#include "ow_avahi.h"
#endif							/* OW_CYGWIN */
#endif							/* OW_ZERO */

/* Include some compatibility functions */
#include "compat.h"

/* Debugging and error messages separated out for readability */
#include "ow_debug.h"

#ifndef PATH_MAX
#define PATH_MAX 2048
#endif

/* Some errnos are not defined for MacOSX and gcc3.3 or openbsd */
#ifndef EBADMSG
#define EBADMSG ENOMSG
#endif							/* EBADMSG */

#ifndef EPROTO
#define EPROTO EIO
#endif							/* EPROTO */

#ifndef ENOTSUP
#define ENOTSUP EOPNOTSUP
#endif							/* ENOTSUP */

/* Allocation wrappers for debugging */
#include "ow_alloc.h"

/* BYTE manipulation macros */
#include "ow_bitwork.h"

/* Define our understanding of integers, floats, ... */
#include "ow_localtypes.h"

/* Include sone byte conversion convenience routines */
#include "ow_integer.h"

/* Directory blob separated out for readability */
#include "ow_dirblob.h"

/* Directory blob (strings) separated out for readability */
#include "ow_charblob.h"

/* memory blob used for bundled transactions */
#include "ow_memblob.h"

/* We use our own read-write locks */
#include "rwlock.h"
/* Many mutexes separated out for readability */
#include "ow_mutexes.h"

/*
    OW -- One Wire
    Globals variables -- each invokation will have it's own data
*/

/* command line options */
#include "ow_opt.h"

/* Several different structures:
  device -- one for each type of 1-wire device
  filetype -- one for each type of file
  parsedname -- translates a path into usable form
*/

/* --------------------------------------------------------- */
/* Filetypes -- directory entries for each 1-wire chip found */
/* predeclare connection_in/out */
struct connection_in;
struct connection_out;
struct connection_side;

/* Maximum length of a file or directory name, and extension */
#define OW_NAME_MAX      (32)
#define OW_EXT_MAX       (6)
#define OW_FULLNAME_MAX  (OW_NAME_MAX+OW_EXT_MAX)
#define OW_DEFAULT_LENGTH (128)

#include "ow_filetype.h"
/* ------------------------------------------- */

/* -------------------------------- */
/* Devices -- types of 1-wire chips */
/*                                  */
#include "ow_device.h"

/* Parsedname -- path converted into components */
#include "ow_parsedname.h"

/* "Object-type" structure for the anctual owfs query --
  holds name, flags, values, and path */
#include "ow_onewirequery.h"

/* Delay for clearing buffer */
#define    WASTE_TIME    (2)

/* device display format */
enum deviceformat { fdi, fi, fdidc, fdic, fidc, fic };
/* Gobal temperature scale */
enum temp_type { temp_celsius, temp_fahrenheit, temp_kelvin, temp_rankine,
};

extern void set_signal_handlers(void (*exit_handler)
	(int signo, siginfo_t * info, void *context));
void set_exit_signal_handlers(void (*exit_handler)
	(int signo, siginfo_t * info, void *context));

#ifndef SI_FROMUSER
#define SI_FROMUSER(siptr)      ((siptr)->si_code <= 0)
#endif
#ifndef SI_FROMKERNEL
#define SI_FROMKERNEL(siptr)    ((siptr)->si_code > 0)
#endif

/* OWSERVER messages */
#include "ow_message.h"

/* Globals information (for local control) */
/* Separated out into ow_global.h for readability */
#include "ow_global.h"

/* State information for the program */
/* Separated out into ow_stateinfo.h for readability */
#include "ow_stateinfo.h"

/* -------------------------------------------- */
/* Prototypes */
/* Separated out to ow_functions.h for clarity */
#include "ow_arg.h"
#include "ow_functions.h"

#endif							/* OW_H */
