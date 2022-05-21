/*
    OW -- One-Wire filesystem
    version 0.4 7/2/2003

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
    libownet: GPL v2 or MIT
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

#ifdef __CYGWIN__
#define __BSD_VISIBLE 1 /* for strep and u_int */
#include <sys/select.h> /* for anything select on newer cygwin */
#endif /* __CYGWIN__ */

#ifdef HAVE_FEATURES_H
#include <features.h>
#endif							/* HAVE_FEATURES_H */

#ifdef HAVE_FEATURE_TESTS_H
#include <feature_tests.h>
#endif							/* HAVE_FEATURE_TESTS_H */

#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>			/* for stat */
#endif							/* HAVE_SYS_STAT_H */

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>			/* for stat */
#endif							/* HAVE_SYS_TYPES_H */

#include <sys/times.h>			/* for times */
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#ifdef __APPLE__
#include <strings.h> /* for strcasecmp */
#endif /* __APPLE__ */
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

#include <termios.h>
#include <errno.h>
#include <syslog.h>

#ifdef HAVE_GETOPT_H
#include <getopt.h>				/* for long options */
#endif							/* HAVE_GETOPT_H */

#include <sys/uio.h>
#include <sys/time.h>			/* for gettimeofday */

#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif							/* HAVE_SYS_SOCKET_H */

#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif							/* HAVE_NETINET_IN_H */

#include <netdb.h>				/* addrinfo */

/* for major() if sys/types.h is not enough */
#ifdef MAJOR_IN_MKDEV
#include <sys/mkdev.h>
#elif defined MAJOR_IN_SYSMACROS
#include <sys/sysmacros.h>
#endif

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
#endif							/* OW_ZERO */

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
#endif							/* EBADMSG */

#ifndef EPROTO
#define EPROTO EIO
#endif							/* EPROTO */

/* File descriptors */
typedef int FILE_DESCRIPTOR_OR_ERROR ;
#define  FILE_DESCRIPTOR_BAD     -1
#define  FILE_DESCRIPTOR_PERSISTENT_IN_USE    -2

/* Define our understanding of integers, floats, ... */
#include "ow_localtypes.h"

/* Directory blob (strings) separated out for readability */
#include "ow_charblob.h"

/* Many mutexes separated out for readability */
#include "rwlock.h"
#include "ow_mutexes.h"

/*
    OW -- One Wire
    Globals variables -- each invokation will have it's own data
*/

/* Several different structures:
  device -- one for each type of 1-wire device
  filetype -- one for each type of file
  parsedname -- translates a path into usable form
*/

/* --------------------------------------------------------- */
/* Filetypes -- directory entries for each 1-wire chip found */
/* predeclare connection_in/out */
struct connection_in;

/* Exposed connection info */
extern int count_inbound_connections;

/* Maximum length of a file or directory name, and extension */
#define OW_NAME_MAX      (32)
#define OW_EXT_MAX       (6)
#define OW_FULLNAME_MAX  (OW_NAME_MAX+OW_EXT_MAX)
#define OW_DEFAULT_LENGTH (128)

/* device display format */
enum deviceformat { fdi, fi, fdidc, fdic, fidc, fic };

void SetSignals(void);

/* Temperature scale handling */
#include "ow_temperature.h"

/* Pressure scale handling */
#include "ow_pressure.h"

/* OWSERVER messages */
#include "ow_message.h"

/* Globals information (for local control) */
/* Separated out into ow_global.h for readability */
#include "ow_global.h"

/* Prototypes */
/* Separated out to ow_functions.h for clarity */
#include "ow_functions.h"

#endif							/* OW_H */
