dnl Process this file with autoconf to produce a configure script.

AC_REVISION($Id$)

AC_INIT
AC_CONFIG_SRCDIR([module/owlib/src/include/ow.h])

VERSION_MAJOR="1"
VERSION_MINOR="0"
PATCHLEVEL="0RC2"

AC_SUBST(VERSION_MAJOR)dnl
AC_SUBST(VERSION_MINOR)dnl
AC_SUBST(PATCHLEVEL)dnl

PACKAGE="owfs"
VERSION="${VERSION_MAJOR}.${VERSION_MINOR}p${PATCHLEVEL}"

AC_SUBST(PACKAGE)dnl
AC_SUBST(VERSION)dnl

AC_MSG_RESULT(Configuring ${PACKAGE}-${VERSION})

AC_PREFIX_DEFAULT(/opt/owfs)dnl
AC_PREREQ(2.59)dnl
AC_CONFIG_AUX_DIR(src/scripts/install)dnl

AM_INIT_AUTOMAKE(${PACKAGE}, ${VERSION})dnl

AC_PATH_PROG(TEST,test,$PATH)dnl
AC_PATH_PROG(RM,rm,$PATH)dnl

# Check for additional programs

AC_PROG_CXX
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

# Check if the modules are enabled

#Check owfs
AC_MSG_CHECKING(if owfs is enabled)
ENABLE_OWFS="true"
AC_ARG_ENABLE(owfs,
[  --enable-owfs         Enable owhttpd module (default true)],
[
    AC_MSG_RESULT([$enableval])
    if test ! "$enableval" = "yes" ; then
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

# We need fuse only if OWFS is enabled
if test "${ENABLE_OWFS}" = "true" ; then
    AC_CHECK_LIB(fuse,__fuse_process_cmd, ,[AC_MSG_ERROR([You have to have libfuse installed first - get it from http://www.sourceforge.net/projects/avf])])
fi

# Check if the cache is enabled.

ENABLE_CACHE=true

AC_MSG_CHECKING([if 1-Wire cache is enabled])

AC_ARG_ENABLE(cache,
[  --enable-cache          Enable 1-Wire network cache (default true)],
[

    AC_MSG_RESULT([$enableval])

    if ! test "$enableval" = "yes" ; then

        ENABLE_CACHE=false
    fi
],
[
    AC_MSG_RESULT([yes (default)])
])

AC_SUBST(ENABLE_CACHE)

# Include libdb if the cache is enabled
if test "${ENABLE_CACHE}" = "true" ; then
    AC_CHECK_LIB(db,__db_dbopen,[
        AC_MSG_CHECKING([>DB4.0?])
        AC_CHECK_LIB(db,__txn_reset,[ENABLE_CACHE41=true],[ENABLE_CACHE41=false])
        ],[
        AC_MSG_WARN([libdb1 not found, cache will be disabled])
        ENABLE_CACHE=false
        AC_SUBST(ENABLE_CACHE)
    ])
fi
AM_CONDITIONAL(ENABLE_CACHE, test "${ENABLE_CACHE}" = "true")
AM_CONDITIONAL(ENABLE_CACHE41, test "${ENABLE_CACHE41}" = "true")

# Check for USB enabled

ENABLE_USB=true

AC_MSG_CHECKING([if usb support is enabled])

AC_ARG_ENABLE(usb,
[  --enable-usb          Enable 1-Wire usb DS2490 support (default true)],
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

# Include libusb if the usb is enabled

if test "${ENABLE_USB}" = "true" ; then

    AC_CHECK_LIB(usb,usb_open, ,[
        AC_MSG_WARN([libusb not found, usb will be disabled])
        ENABLE_USB=false
        AC_SUBST(ENABLE_USB)
    ])
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

# Include libpthread if multithreading is enabled

if test "${ENABLE_MT}" = "true" ; then

    AC_CHECK_LIB(pthread,pthread_mutex_lock, ,[
        AC_MSG_WARN([libpthread not found, multithreading will be disabled])
        ENABLE_MT=false
        AC_SUBST(ENABLE_MT)
    ])
fi

AM_CONDITIONAL(ENABLE_MT, test "${ENABLE_MT}" = "true")

# Checks for header files.

AC_HEADER_DIRENT
AC_HEADER_STDC
AC_CHECK_HEADERS([fcntl.h netinet/in.h stdlib.h string.h sys/file.h syslog.h termios.h unistd.h limits.h])

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
AC_FUNC_MALLOC
AC_FUNC_MEMCMP
AC_FUNC_MKTIME
AC_FUNC_REALLOC
AC_FUNC_SELECT_ARGTYPES
AC_FUNC_STRFTIME
AC_FUNC_STRTOD
AC_TYPE_SIGNAL
AC_CHECK_FUNCS([gettimeofday memchr memset select socket strcasecmp strchr strdup strncasecmp strtol strtoul])

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
AC_MSG_RESULT()
