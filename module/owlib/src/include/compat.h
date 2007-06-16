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
*/
#ifndef COMPAT_H
#define COMPAT_H

#include <config.h>
#include "owfs_config.h"

#ifdef HAVE_FEATURES_H
#include <features.h>
#endif

#include "compat_netdb.h"
#include "compat_getopt.h"

#ifndef HAVE_STRSEP
char *strsep(char **stringp, const char *delim);
#endif

#if defined(__UCLIBC__)
#if ((__UCLIBC_MAJOR__ << 16)+(__UCLIBC_MINOR__ << 8)+(__UCLIBC_SUBLEVEL__) <= 0x000913)
#undef HAVE_TDESTROY
#else							/* Older than 0.9.19 */
#define HAVE_TDESTROY 1
#endif							/* Older than 0.9.19 */
#else

#endif							/* __UCLIBC__ */

#ifndef HAVE_TDESTROY
void tdestroy(void *vroot, void (*freefct) (void *));
#endif

#ifndef __COMPAR_FN_T
# define __COMPAR_FN_T
typedef int (*__compar_fn_t) (__const void *, __const void *);
#endif

#ifndef HAVE_TSEARCH
void *tsearch (__const void *__key, void **__rootp,
                      __compar_fn_t __compar);
#endif

#ifndef HAVE_TFIND
void *tfind (__const void *__key, void *__const *__rootp,
                    __compar_fn_t __compar);
#endif

#ifndef HAVE_TDELETE
void *tdelete (__const void *__restrict __key,
                      void **__restrict __rootp,
                      __compar_fn_t __compar);
#endif

#ifndef __ACTION_FN_T
# define __ACTION_FN_T
#if OW_DARWIN == 0
/* VISIT is always defined in search.h on MacOSX. */
typedef enum
  {
    preorder,
    postorder,
    endorder,
  leaf
  }
VISIT;
#endif /* OW_DARWIN */

typedef void (*__action_fn_t) (__const void *__nodep, VISIT __value,
                               int __level);
#endif

#ifndef HAVE_TWALK
void twalk (__const void *__root, __action_fn_t __action);
#endif

#ifdef __UCLIBC__
#if defined(__UCLIBC_MAJOR__) && defined(__UCLIBC_MINOR__) && defined(__UCLIBC_SUBLEVEL__)
#if ((__UCLIBC_MAJOR__<<16) + (__UCLIBC_MINOR__<<8) + (__UCLIBC_SUBLEVEL__)) <= 0x000913

/* Since syslog will hang forever with uClibc-0.9.19 if syslogd is not
 * running, then we don't use it unless we really wants it
 * WRT45G usually have syslogd running. uClinux-dist might not have it. */
#ifndef USE_SYSLOG
#define syslog(a,...)	{ /* ignore_syslog */ }
#define openlog(a,...)	{ /* ignore_openlog */ }
#define closelog()		{ /* ignore_closelog */ }
#endif
#endif							/* __UCLIBC__ */
#endif							/* __UCLIBC__ */
#endif							/* __UCLIBC__ */



#endif
