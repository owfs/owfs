dnl Process this file with autoconf to produce a configure script.

AC_REVISION($Id$)

AC_INIT
AC_CONFIG_SRCDIR([module/owlib/src/include/ow.h])

VERSION_MAJOR="1"
VERSION_MINOR="1"
PATCHLEVEL="0RC"

AC_SUBST(VERSION_MAJOR)dnl
AC_SUBST(VERSION_MINOR)dnl
AC_SUBST(PATCHLEVEL)dnl

PACKAGE="owfs"
VERSION="${VERSION_MAJOR}.${VERSION_MINOR}p${PATCHLEVEL}"

AC_SUBST(PACKAGE)dnl
AC_SUBST(VERSION)dnl

AC_MSG_RESULT(Configuring ${PACKAGE}-${VERSION})

AC_PREFIX_DEFAULT(/opt/owfs)dnl
AC_PREREQ(2.57)dnl
AC_CONFIG_AUX_DIR(src/scripts/install)dnl

AM_INIT_AUTOMAKE(${PACKAGE}, ${VERSION})dnl

AC_PATH_PROG(TEST,test,$PATH)dnl
AC_PATH_PROG(RM,rm,$PATH)dnl

# Check for additional programs

#AC_PROG_CXX
AC_PROG_CC
AC_PROG_CPP
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET
AC_PROG_RANLIB
AC_PROG_AWK
AC_LIBTOOL_DLOPEN
AM_PROG_LIBTOOL
AC_SUBST(LIBTOOL_DEPS)

AC_PATH_PROG(RPM,rpm,,$PATH)dnl
AC_PATH_PROG(RPMBUILD,rpmbuild,,$PATH)dnl


# Include libpthread if multithreading is enabled
#
# This stream of line noise will hopefully find out the best way to get
# to a threading kit. Logic came from APR, but this will still cross-compile
AC_DEFUN([PTHREADS_CHECK_COMPILE], [
AC_TRY_LINK( [
#include <pthread.h>
#include <stddef.h>
static void *doit(void *ignored) { return ignored; }
], [
    pthread_t t;
    pthread_mutex_t m;
    pthread_mutex_init(&m, (pthread_mutexattr_t *)NULL);
    pthread_mutex_lock(&m);
    pthread_create(&t, (pthread_attr_t *)NULL, doit, NULL);
], [
    pthreads_ok="yes"
], [
    pthreads_ok="no"
]) ])

# Check if the modules are enabled

#Check owfs
AC_MSG_CHECKING(if owfs is enabled)
ENABLE_OWFS="auto"
AC_ARG_ENABLE(owfs,
[  --enable-owfs         Enable owfs module (default auto)],
[
    AC_MSG_RESULT([$enableval])
    if test "$enableval" != "yes" ; then
        ENABLE_OWFS=false
    fi
],
[
    AC_MSG_RESULT([yes (default)])
])
AC_SUBST(ENABLE_OWFS)

#Check owhttpd
AC_MSG_CHECKING(if owhttpd is enabled)
ENABLE_OWHTTPD="true"
AC_ARG_ENABLE(owhttpd,
[  --enable-owhttpd      Enable owhttpd module (default true)],
[
    AC_MSG_RESULT([$enableval])
    if test ! "$enableval" = "yes" ; then
        ENABLE_OWHTTPD=false
    fi
],
[
    AC_MSG_RESULT([yes (default)])
])
AC_SUBST(ENABLE_OWHTTPD)

#Check owftpd
AC_MSG_CHECKING(if owftpd is enabled)
ENABLE_OWFTPD="false"
AC_ARG_ENABLE(owftpd,
[  --enable-owftpd       Enable owftpd module (default false)],
[
    AC_MSG_RESULT([$enableval])
    if test "$enableval" = "yes" ; then
        ENABLE_OWFTPD=true
    fi
],
[
    AC_MSG_RESULT([false (default)])
])
AC_SUBST(ENABLE_OWFTPD)

#Check owperl
AC_MSG_CHECKING(if owperl is enabled)
ENABLE_OWPERL="false"
AC_ARG_ENABLE(owperl,
[  --enable-owperl       Enable owperl module (default false)],
[
    AC_MSG_RESULT([$enableval])
    if test ! "$enableval" = "yes" ; then
        ENABLE_OWPERL=false
    fi
],
[
    AC_MSG_RESULT([false (default)])
])
AC_SUBST(ENABLE_OWPERL)

# We need fuse only if OWFS is enabled
if test "${ENABLE_OWFS}" != "false" ; then
    AC_CHECK_LIB(fuse,__fuse_process_cmd,, [
	if test "${ENABLE_OWFS}" = "true"; then
		AC_MSG_ERROR([You have to have libfuse installed first - get it from http://fuse.sourceforge.net])
	else
		AC_MSG_WARN([OWFS is disabled because libfuse is not installed.])
        	ENABLE_OWFS="false"
	fi
    ])
    if test "${ENABLE_OWFS}" != "false"; then
        ENABLE_OWFS="true"
    fi
else
    ENABLE_OWFS="false"
fi

AM_CONDITIONAL(ENABLE_OWFS, test "${ENABLE_OWFS}" = "true")

# Check if the cache is enabled.
AC_MSG_CHECKING(if caching is enabled)
ENABLE_CACHE="true"
AC_ARG_ENABLE(cache,
[  --enable-cache      Enable caching (default true)],
[
    AC_MSG_RESULT([$enableval])
    if test ! "$enableval" = "yes" ; then
        ENABLE_CACHE=false
    fi
],
[
    AC_MSG_RESULT([yes (default)])
])
AC_SUBST(ENABLE_CACHE)


# Check for USB enabled

ENABLE_USB=auto

AC_MSG_CHECKING([if usb support is enabled])

AC_ARG_ENABLE(usb,
[  --enable-usb          Enable 1-Wire usb DS2490 support (default auto)],
[

    AC_MSG_RESULT([$enableval])

    if ! test "$enableval" = "yes" ; then

        ENABLE_USB=false
    fi
],
[
    AC_MSG_RESULT([yes (default)])
])

AC_SUBST(ENABLE_USB)
AM_CONDITIONAL(ENABLE_CACHE, test "${ENABLE_CACHE}" = "true")

# Include libusb if the usb is enabled

if test "${ENABLE_USB}" != "false" ; then

    AC_CHECK_LIB(usb,usb_open, ,[
	if test "${ENABLE_USB}" = "true" ; then
	    AC_MSG_ERROR([libusb must be installed to use a USB adapter])
	else
            AC_MSG_WARN([libusb not found, usb will be disabled])
            ENABLE_USB=false
            AC_SUBST(ENABLE_USB)
	fi
    ])
    if test "${ENABLE_USB}" != "false" ; then
        ENABLE_USB=true
        AC_SUBST(ENABLE_USB)
    fi
fi

AM_CONDITIONAL(ENABLE_USB, test "${ENABLE_USB}" = "true")

# Check for multithreading

ENABLE_MT=true

AC_MSG_CHECKING([if multithreading is enabled])

AC_ARG_ENABLE(mt,
[  --enable-mt          Enable multithreading support (default true)],
[

    AC_MSG_RESULT([$enableval])

    if ! test "$enableval" = "yes" ; then

        ENABLE_MT=false
    fi
],
[
    AC_MSG_RESULT([yes (default)])
])

AC_SUBST(ENABLE_MT)

if test "${ENABLE_MT}" = "true" ; then
    PTHREADS_CHECK_COMPILE
    AC_CACHE_CHECK(for pthreads, ac_cv_pthreads_flags, [
	ac_cv_pthreads_flags=""
	if test "$pthreads_ok" != "yes"; then
	    for flag in -kthread -pthread -pthreads -mthreads -Kthread -threads -mt; do
	        ac_save="$CFLAGS"
	        CFLAGS="$CFLAGS $flag"
		PTHREADS_CHECK_COMPILE
		CFLAGS="$ac_save"

		if test "$pthreads_ok" = "yes"; then
		    ac_cv_pthreads_cflags="$flag"
		    break
		fi
            done
	fi
])
    if test "$pthreads_ok" = "yes"; then
	ENABLE_MT=true
	CFLAGS="$CFLAGS $ac_cv_pthreads_cflags"
    else
        AC_CHECK_LIB(pthread,pthread_mutex_lock, ,[
            AC_MSG_WARN([libpthread not found, multithreading will be disabled])
            ENABLE_MT=false
            AC_SUBST(ENABLE_MT)
        ])
        if test "${ENABLE_MT}" = "true" ; then
	    PTHREADS_CHECK_COMPILE
	    if test "$pthreads_ok" != "yes"; then
                AC_MSG_WARN([libpthread not found, multithreading will be disabled])
		ENABLE_MT=false
	    fi
        fi
    fi
fi

AM_CONDITIONAL(ENABLE_MT, test "${ENABLE_MT}" = "true")

#Check parallel port
AC_MSG_CHECKING(if parport DS1410E is enabled)
ENABLE_PARPORT="false"
AC_ARG_ENABLE(parport,
[  --enable-parport         Enable parport (DS1410E) module (default parport)],
[
    AC_MSG_RESULT([$enableval])
    if test "$enableval" = "ues" ; then
        ENABLE_PARPORT=true
    fi
],
[
    AC_MSG_RESULT([no (default)])
])
AC_SUBST(ENABLE_PARPORT)

AM_CONDITIONAL(ENABLE_PARPORT, test "${ENABLE_PARPORT}" = "true")

# Checks for header files.

AC_HEADER_DIRENT
AC_HEADER_STDC
AC_CHECK_HEADERS([sys/time.h sys/types.h fcntl.h netinet/in.h stdlib.h string.h sys/file.h syslog.h termios.h unistd.h limits.h])

# owftpd section
AC_SUBST(HAVE_NEW_SS_FAMILY)
AC_EGREP_HEADER(__ss_family,
    [sys/socket.h],
    [HAVE_NEW_SS_FAMILY=false],
    [HAVE_NEW_SS_FAMILY=true] )
AC_MSG_WARN([SS is ${HAVE_NEW_SS_FAMILY}])
AM_CONDITIONAL(HAVE_NEW_SS_FAMILY, test "${HAVE_NEW_SS_FAMILY}" = "true")
# end owftpd section


# conditionals for httpd/ftpd
AM_CONDITIONAL(ENABLE_OWHTTPD, test "${ENABLE_OWHTTPD}" = "true")
AM_CONDITIONAL(ENABLE_OWFTPD, test "${ENABLE_OWFTPD}" = "true")


# Checks for typedefs, structures, and compiler characteristics.

AC_C_CONST
AC_TYPE_OFF_T
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_HEADER_TIME
AC_HEADER_STDBOOL
AC_STRUCT_TM

# Checks for library functions.

AC_FUNC_FORK
#AC_FUNC_MALLOC
AC_FUNC_MEMCMP
AC_FUNC_MKTIME
#AC_FUNC_REALLOC
AC_FUNC_SELECT_ARGTYPES
AC_FUNC_STRFTIME
AC_FUNC_STRTOD
AC_TYPE_SIGNAL
AC_CHECK_FUNCS([gettimeofday memchr memset select socket strcasecmp strchr strdup strncasecmp strtol strtoul tsearch tfind tdelete tdestroy])

# Process config.h.in

AH_TOP([
#ifndef OWFS_CONFIG_H
#define OWFS_CONFIG_H
])
AH_BOTTOM([#endif])

AM_CONFIG_HEADER([src/include/owfs_config.h:src/include/owfs_config.h.in])

AC_CONFIG_FILES([
	Makefile
	module/Makefile
	module/owlib/Makefile
	module/owlib/src/Makefile
	module/owlib/src/include/Makefile
	module/owlib/src/c/Makefile
	module/owlib/src/man/Makefile
	module/owfs/Makefile
	module/owfs/src/Makefile
	module/owfs/src/include/Makefile
	module/owfs/src/c/Makefile
	module/owfs/src/man/Makefile
	module/owhttpd/Makefile
	module/owhttpd/src/Makefile
	module/owhttpd/src/include/Makefile
	module/owhttpd/src/c/Makefile
	module/owhttpd/src/man/Makefile
	module/owftpd/Makefile
	module/owftpd/src/Makefile
	module/owftpd/src/include/Makefile
	module/owftpd/src/c/Makefile
	module/owftpd/src/man/Makefile
	module/owperl/Makefile
	module/owperl/src/Makefile
	module/owperl/src/include/Makefile
	module/owperl/src/c/Makefile
	module/owperl/src/man/Makefile
	src/Makefile
	src/include/Makefile
	src/man/Makefile
	src/rpm/Makefile
	src/rpm/owfs.spec
	src/scripts/Makefile
])
AC_OUTPUT

# Now, let's tell them what is the current configuration. Only the items
# that are funny (like current default prefix) or configurable (like the
# cache) should be included.

AC_MSG_RESULT()
AC_MSG_RESULT([Current configuration:])
AC_MSG_RESULT()

AC_MSG_RESULT([    Deployment location: ${prefix}])

AC_MSG_RESULT()
AC_MSG_RESULT([Compile-time options:])

if test "${ENABLE_CACHE}" = "true"; then
    AC_MSG_RESULT([    Caching is enabled])
else
    AC_MSG_RESULT([    Caching is DISABLED])
fi
if test "${ENABLE_USB}" = "true"; then
    AC_MSG_RESULT([    USB is enabled])
else
    AC_MSG_RESULT([    USB is DISABLED])
fi
if test "${ENABLE_MT}" = "true"; then
    AC_MSG_RESULT([    Multithreading is enabled])
else
    AC_MSG_RESULT([    Multithreading is DISABLED])
fi
if test "${ENABLE_PARPORT}" = "true"; then
    AC_MSG_RESULT([    Parallel port DS1410E is enabled])
else
    AC_MSG_RESULT([    Parallel port DS1410E is DISABLED])
fi

AC_MSG_RESULT()
AC_MSG_RESULT([Module configuration:])

if test "${ENABLE_OWFS}" = "true"; then
    AC_MSG_RESULT([    owfs is enabled])
else
    AC_MSG_RESULT([    owfs is DISABLED])
fi
if test "${ENABLE_OWHTTPD}" = "true"; then
    AC_MSG_RESULT([    owhttpd is enabled])
else
    AC_MSG_RESULT([    owhttpd is DISABLED])
fi
if test "${ENABLE_OWFTPD}" = "true"; then
    AC_MSG_RESULT([    owftpd is enabled])
else
    AC_MSG_RESULT([    owftpd is DISABLED])
fi
if test "${ENABLE_OWPERL}" = "true"; then
    AC_MSG_RESULT([    owperl is enabled])
else
    AC_MSG_RESULT([    owperl is DISABLED])
fi
AC_MSG_RESULT()
