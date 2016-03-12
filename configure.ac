dnl Process this file with autoconf to produce a configure script.

AC_INIT

# Making releases:
#   VERSION_PATCHLEVEL += 1;
#   OWFS_INTERFACE_AGE += 1;
#   OWFS_BINARY_AGE += 1;
# if any functions have been added, set OWFS_INTERFACE_AGE to 0.
# if backwards compatibility has been broken,
# set OWFS_BINARY_AGE _and_ OWFS_INTERFACE_AGE to 0.
# (In our case it's better to always keep AGE to 0, since nobody else will use our library)

VERSION_MAJOR=3
VERSION_MINOR=1
VERSION_PATCHLEVEL=1

OWFS_INTERFACE_AGE=0
OWFS_BINARY_AGE=0

AC_SUBST(VERSION_MAJOR)
AC_SUBST(VERSION_MINOR)
AC_SUBST(VERSION_PATCHLEVEL)
AC_SUBST(OWFS_INTERFACE_AGE)
AC_SUBST(OWFS_BINARY_AGE)

LT_CURRENT=`expr $VERSION_PATCHLEVEL - $OWFS_INTERFACE_AGE`
LT_REVISION=$OWFS_INTERFACE_AGE
LT_AGE=`expr $OWFS_BINARY_AGE - $OWFS_INTERFACE_AGE`

AC_SUBST(LT_CURRENT)dnl
AC_SUBST(LT_REVISION)dnl
AC_SUBST(LT_AGE)dnl

LT_RELEASE=$VERSION_MAJOR.$VERSION_MINOR
AC_SUBST(LT_RELEASE)dnl

VERSION="${VERSION_MAJOR}.${VERSION_MINOR}p${VERSION_PATCHLEVEL}"
AC_SUBST(VERSION)dnl

PACKAGE="owfs"
AC_SUBST(PACKAGE)dnl

AC_MSG_RESULT(Configuring ${PACKAGE}-${VERSION})

AC_CONFIG_AUX_DIR(src/scripts/install)dnl

AC_CONFIG_MACRO_DIR([src/scripts/m4])

AC_CANONICAL_TARGET
AM_INIT_AUTOMAKE(${PACKAGE}, ${VERSION})dnl

# Process config.h.in
AH_TOP([
#ifndef OWCONFIG_H
	#define OWCONFIG_H
])
AH_BOTTOM([#endif])
AM_CONFIG_HEADER(src/include/config.h)

AC_PREREQ(2.57)dnl
AC_PREFIX_DEFAULT(/opt/owfs)dnl

AC_PATH_PROG(ECHO,echo,,$PATH)dnl
AC_PATH_PROG(TEST,test,,$PATH)dnl
AC_PATH_PROG(RM,rm,,$PATH)dnl
AC_PATH_PROG(RPM,rpm,,$PATH)dnl
AC_PATH_PROG(RPMBUILD,rpmbuild,,$PATH)dnl
AC_PATH_PROG([SWIG],[swig],,$PATH)dnl
AC_PATH_PROG([SOELIM],[soelim])
AM_CONDITIONAL([SOELIM], [test -n "${SOELIM}"])
# This macro should appair before any other PKG_* macro
PKG_PROG_PKG_CONFIG

# Check for additional programs

# We don't need c++ right now.
#AC_PROG_CXX
AC_PROG_CC
AC_PROG_CPP
AC_PROG_INSTALL
AC_LIBTOOL_WIN32_DLL
AC_PROG_LN_S
AC_PROG_MAKE_SET
AC_PROG_AWK
AC_LIBTOOL_DLOPEN
AM_PROG_LIBTOOL
AC_SUBST(LIBTOOL_DEPS)

# Supposedly this helps OS X compiling
AC_DEFINE(_DARWIN_C_SOURCE, 1, [Define on Darwin to activate all library features])

HAVE_DEBIAN="false"
AC_MSG_CHECKING([if debian-system is used])
AC_ARG_ENABLE(debian,
[  --enable-debian         Enable debian-system (default false)],
[
	AC_MSG_RESULT([$enableval])
	if test "$enableval" = "yes" ; then
		HAVE_DEBIAN="true"
	fi
],
[
    AC_MSG_RESULT([no (default)])
])
AC_SUBST(HAVE_DEBIAN)
AM_CONDITIONAL(HAVE_DEBIAN, test "${HAVE_DEBIAN}" = "true")


HAVE_DARWIN="false"
HAVE_FREEBSD="false"
HAVE_CYGWIN="false"

AC_MSG_CHECKING([for special host])
case "$host" in
	*-*-cygwin*)
		HAVE_CYGWIN="true"
		CFLAGS="$CFLAGS -mwin32 -g"
		AC_MSG_RESULT([Cygwin])
		;;
	*-darwin*)
		HAVE_DARWIN="true"
		AC_MSG_RESULT([Darwin])
		;;
	*-freebsd*)
		HAVE_FREEBSD="true"
		AC_MSG_RESULT([FreeBSD])
		;;
	*)
		AC_MSG_RESULT([Other host])
		;;
esac
AC_SUBST(HAVE_DARWIN)
AM_CONDITIONAL(HAVE_DARWIN, test "${HAVE_DARWIN}" = "true")
AC_SUBST(HAVE_CYGWIN)
AM_CONDITIONAL(HAVE_CYGWIN, test "${HAVE_CYGWIN}" = "true")
AC_SUBST(HAVE_FREEBSD)
AM_CONDITIONAL(HAVE_FREEBSD, test "${HAVE_FREEBSD}" = "true")

# launchd using the new launch_activate_socket
AC_CHECK_FUNCS(launch_activate_socket)
# AC_CHECK_HEADERS(launch.h)

AM_CPPFLAGS=""
AC_SUBST([AM_CPPFLAGS])

PIC_FLAGS=""
if test "$lt_prog_compiler_pic_works" = yes; then
	PIC_FLAGS="$lt_prog_compiler_pic"
fi
AC_SUBST(PIC_FLAGS)

# Make sure tclConfig.sh is found under /usr/lib64/
# Should perhaps support cross-compiling to other cpu-type too?
LIBPOSTFIX=
case "${host_os}" in
	*linux* )
	case "${host_cpu}" in
		powerpc64 | s390x | x86_64 )
		LIBPOSTFIX="64"
		CFLAGS="$CFLAGS -m64"
		;;
	esac
	;;
esac
AC_SUBST(LIBPOSTFIX)



if test "$cross_compiling" != yes; then
	OW_CHECK_NESTED_FUNCTIONS
fi

OWFSROOT="`pwd`"
AC_SUBST(OWFSROOT)

m4_include([src/scripts/m4/acx_pthread.m4])

# Checks for header files.
AC_HEADER_DIRENT
AC_HEADER_STDC
AC_CHECK_HEADERS([asm/types.h arpa/inet.h sys/ioctl.h sys/mkdev.h sys/socket.h sys/time.h sys/times.h sys/types.h sys/param.h sys/uio.h feature_tests.h fcntl.h netinet/in.h stdlib.h string.h strings.h sys/file.h syslog.h termios.h unistd.h limits.h stdint.h features.h getopt.h resolv.h semaphore.h])
AC_CHECK_HEADERS([linux/limits.h linux/types.h netdb.h dlfcn.h])
AC_CHECK_HEADERS(sys/event.h sys/inotify.h)

# Test if debugging out enabled
ENABLE_DEBUG="true"
AC_MSG_CHECKING([if debug-output is enabled])
AC_ARG_ENABLE(debug,
[  --enable-debug          Enable debug-output (default true)],
[
	AC_MSG_RESULT([$enableval])
	if ! test "$enableval" = "yes" ; then
		ENABLE_DEBUG="false"
	fi
],
[
	AC_MSG_RESULT([yes (default)])
])
AC_SUBST(ENABLE_DEBUG)
AM_CONDITIONAL(ENABLE_DEBUG, test "${ENABLE_DEBUG}" = "true")

# Test if mutex-debugging is enabled. Should not be enabled for smaller embedded systems
ENABLE_MUTEX_DEBUG="true"
AC_MSG_CHECKING([if mutexdebug is enabled])
AC_ARG_ENABLE(mutexdebug,
[  --enable-mutexdebug     Enable mutexdebug-output (default true)],
[
	AC_MSG_RESULT([$enableval])
	if ! test "$enableval" = "yes" ; then
		ENABLE_MUTEX_DEBUG="false"
	fi
],
[
	AC_MSG_RESULT([yes (default)])
])
AC_SUBST(ENABLE_MUTEX_DEBUG)
AM_CONDITIONAL(ENABLE_MUTEX_DEBUG, test "${ENABLE_MUTEX_DEBUG}" = "true")

# Test if OWSHELL should be supported
ENABLE_OWSHELL="true"
AC_MSG_CHECKING([if OWSHELL support is enabled])
AC_ARG_ENABLE(owshell,
[  --enable-owshell        Enable owshell support (default true)],
[
	AC_MSG_RESULT([$enableval])
	if ! test "$enableval" = "yes" ; then
		ENABLE_OWSHELL="false"
	fi
],
[
	AC_MSG_RESULT([yes (default)])
])
AC_SUBST(ENABLE_OWSHELL)
AM_CONDITIONAL(ENABLE_OWSHELL, test "${ENABLE_OWSHELL}" = "true")

# Test if OWLIB should be supported
ENABLE_OWLIB="true"
AC_MSG_CHECKING([if OWLIB support is enabled])
AC_ARG_ENABLE(owlib,
[  --enable-owlib          Enable owlib support (default true)],
[
	AC_MSG_RESULT([$enableval])
	if ! test "$enableval" = "yes" ; then
		ENABLE_OWLIB="false"
	fi
],
[
    AC_MSG_RESULT([yes (default)])
])
AC_SUBST(ENABLE_OWLIB)
AM_CONDITIONAL(ENABLE_OWLIB, test "${ENABLE_OWLIB}" = "true")

# Test if OWNETLIB should be supported
ENABLE_OWNETLIB="true"
AC_MSG_CHECKING([if OWNETLIB support is enabled])
AC_ARG_ENABLE(ownetlib,
[  --enable-ownetlib       Enable ownetlib support (default true)],
[
	AC_MSG_RESULT([$enableval])
	if ! test "$enableval" = "yes" ; then
		ENABLE_OWNETLIB="false"
	fi
],
[
	AC_MSG_RESULT([yes (default)])
])
AC_SUBST(ENABLE_OWNETLIB)
AM_CONDITIONAL(ENABLE_OWNETLIB, test "${ENABLE_OWNETLIB}" = "true")

# Check for threading
ACX_PTHREAD
if test "$acx_pthread_ok" = "yes"; then
	#CFLAGS="$CFLAGS $PTHREAD_CFLAGS"
	# can't set pthread flags for all compilations, since owshell doesn't need it.
	if test "$cross_compiling" = yes; then
		# We need to link libow.so with -lpthread on some cross-platforms
		# since owtcl.so doesn't work otherwise. -pthread is not enough
		if test x"$PTHREAD_LIBS" = x -a "x$PTHREAD_CFLAGS" = "x-pthread"; then
			AC_MSG_RESULT([add -lpthread to PTHREAD_LIBS])
			PTHREAD_LIBS="-lpthread"
		fi
	fi

	ENABLE_MT="true"
else
	ENABLE_MT="false"
	PTHREAD_CFLAGS=""
	PTHREAD_LIBS=""
	PTHREAD_CC=""
fi

# Check for i2c support
ENABLE_I2C="true"
AC_MSG_CHECKING([if i2c(DS2482-x00) is enabled])
AC_ARG_ENABLE(i2c,
[  --enable-i2c            Enable i2c (DS2482-x00) support (default true)],
[
	AC_MSG_RESULT([$enableval])
	if ! test "$enableval" = "yes" ; then
		ENABLE_I2C="false"
	fi
],
[
	AC_MSG_RESULT([yes (default)])
])

AC_SUBST(ENABLE_I2C)
AM_CONDITIONAL(ENABLE_I2C, test "${ENABLE_I2C}" = "true")


# Check for W1 (linux kernel module)
ENABLE_W1="true"
AC_MSG_CHECKING([if W1 is enabled])
AC_ARG_ENABLE(w1,
[  --enable-w1             Enable w1 support (default true)],
[
	AC_MSG_RESULT([$enableval])
	if ! test "$enableval" = "yes" ; then
		ENABLE_W1="false"
	fi
],
[
	AC_MSG_RESULT([yes (default)])
])
AC_SUBST(ENABLE_W1)
AM_CONDITIONAL(ENABLE_W1, test "${ENABLE_W1}" = "true")


# Check if the modules are enabled

#Check owhttpd
ENABLE_OWHTTPD="true"
AC_MSG_CHECKING(if owhttpd is enabled)
AC_ARG_ENABLE(owhttpd,
[  --enable-owhttpd        Enable owhttpd module (default true)],
[
	AC_MSG_RESULT([$enableval])
	if test ! "$enableval" = "yes" ; then
		ENABLE_OWHTTPD="false"
	else
		ENABLE_OWLIB="true"
	fi
],
[
	ENABLE_OWLIB="true"
	AC_MSG_RESULT([yes (default)])
])
AC_SUBST(ENABLE_OWHTTPD)
AM_CONDITIONAL(ENABLE_OWHTTPD, test "${ENABLE_OWHTTPD}" = "true")

#Check owftpd
ENABLE_OWFTPD="true"
AC_MSG_CHECKING(if owftpd is enabled)
AC_ARG_ENABLE(owftpd,
[  --enable-owftpd         Enable owftpd module (default true)],
[
	AC_MSG_RESULT([$enableval])
	if test ! "$enableval" = "yes" ; then
		ENABLE_OWFTPD="false"
	else
#		if test ! "${ENABLE_MT}" = "true" ; then
#			AC_MSG_ERROR([owftpd needs multithreading])
#		fi
		ENABLE_OWLIB="true"
	fi
],
[
	if test "${ENABLE_MT}" = "true" ; then
		ENABLE_OWLIB="true"
		AC_MSG_RESULT([yes (default)])
	else
		ENABLE_OWFTPD="false"
		AC_MSG_RESULT([no (multithreading needed)])
	fi
])
AC_SUBST(ENABLE_OWFTPD)
AM_CONDITIONAL(ENABLE_OWFTPD, test "${ENABLE_OWFTPD}" = "true")


#Check owserver
AC_MSG_CHECKING(if owserver is enabled)
ENABLE_OWSERVER="true"
AC_ARG_ENABLE(owserver,
[  --enable-owserver       Enable owserver module (default true)],
[
	AC_MSG_RESULT([$enableval])
	if test ! "$enableval" = "yes" ; then
		ENABLE_OWSERVER="false"
	else
		ENABLE_OWLIB="true"
	fi
],
[
	ENABLE_OWLIB="true"
	AC_MSG_RESULT([yes (default)])
])
AC_SUBST(ENABLE_OWSERVER)
AM_CONDITIONAL(ENABLE_OWSERVER, test "${ENABLE_OWSERVER}" = "true")

#Check owexternal
AC_MSG_CHECKING(if owexternal is enabled)
ENABLE_OWEXTERNAL="true"
AC_ARG_ENABLE(owexternal,
[  --enable-owexternal     Enable owexternal module (default true)],
[
	AC_MSG_RESULT([$enableval])
	if test ! "$enableval" = "yes" ; then
		ENABLE_OWEXTERNAL="false"
	else
		ENABLE_OWLIB="true"
	fi
],
[
	ENABLE_OWLIB="true"
	AC_MSG_RESULT([yes (default)])
])
AC_SUBST(ENABLE_OWEXTERNAL)
AM_CONDITIONAL(ENABLE_OWEXTERNAL, test "${ENABLE_OWEXTERNAL}" = "true")

#Check ownet
ENABLE_OWNET="true"
AC_MSG_CHECKING(if ownet is enabled)
AC_ARG_ENABLE(ownet,
[  --enable-ownet          Enable ownet module (default true)],
[
	AC_MSG_RESULT([$enableval])
	if test ! "$enableval" = "yes" ; then
		ENABLE_OWNET="false"
	else
		ENABLE_OWLIB="true"
	fi
],
[
	ENABLE_OWLIB="true"
	AC_MSG_RESULT([yes (default)])
])
AC_SUBST(ENABLE_OWNET)
AM_CONDITIONAL(ENABLE_OWNET, test "${ENABLE_OWNET}" = "true")


#Check owtap
ENABLE_OWTAP="true"
AC_MSG_CHECKING(if owtap is enabled)
AC_ARG_ENABLE(owtap,
[  --enable-owtap          Enable owtap module (default true)],
[
	AC_MSG_RESULT([$enableval])
	if test ! "$enableval" = "yes" ; then
		ENABLE_OWTAP="false"
	fi
],
[
	AC_MSG_RESULT([yes (default)])
])
AC_SUBST(ENABLE_OWTAP)
AM_CONDITIONAL(ENABLE_OWTAP, test "${ENABLE_OWTAP}" = "true")


#Check owmalloc (special memory allocation routines)
ENABLE_OWMALLOC="false"
AC_MSG_CHECKING(if owmalloc is enabled)
AC_ARG_ENABLE(owmalloc,
[  --enable-owmalloc       Enable owmalloc checking (default false)],
[
	AC_MSG_RESULT([$enableval])
	if test "$enableval" = "yes" ; then
		ENABLE_OWMALLOC="true"
	fi
],
[
	AC_MSG_RESULT([no (default)])
])
AC_SUBST(ENABLE_OWMALLOC)
AM_CONDITIONAL(ENABLE_OWMALLOC, test "${ENABLE_OWMALLOC}" = "true")


#Check owmon
AC_MSG_CHECKING(if owmon is enabled)
ENABLE_OWMON="true"
AC_ARG_ENABLE(owmon,
[  --enable-owmon          Enable owmon module (default true)],
[
	AC_MSG_RESULT([$enableval])
	if test ! "$enableval" = "yes" ; then
		ENABLE_OWMON="false"
	fi
],
[
	AC_MSG_RESULT([yes (default)])
])
AC_SUBST(ENABLE_OWMON)
AM_CONDITIONAL(ENABLE_OWMON, test "${ENABLE_OWMON}" = "true")

#Check owcapi
AC_MSG_CHECKING(if owcapi is enabled)
ENABLE_OWCAPI="true"
AC_ARG_ENABLE(owcapi,
[  --enable-owcapi         Enable owcapi module (default true)],
[
	AC_MSG_RESULT([$enableval])
	if test ! "$enableval" = "yes" ; then
		ENABLE_OWCAPI="false"
	else
		ENABLE_OWLIB="true"
	fi
],
[
	ENABLE_OWLIB="true"
	AC_MSG_RESULT([yes (default)])
])
AC_SUBST(ENABLE_OWCAPI)
AM_CONDITIONAL(ENABLE_OWCAPI, test "${ENABLE_OWCAPI}" = "true")


#Check swig
AC_MSG_CHECKING(if swig is enabled)
if test -z "$SWIG"  ; then
	ENABLE_SWIG="false"
else
	ENABLE_SWIG="auto"
fi
AC_ARG_ENABLE(swig,
[  --enable-swig           Enable swig (default auto)],
[
	if test "$enableval" = "no"; then
		ENABLE_SWIG="false"
		SWIG=""
		AC_MSG_RESULT([no])
	else
		if test -z "$SWIG"; then
			AC_MSG_ERROR([Swig is not found and could not be enabled])
		fi
		ENABLE_OWLIB="true"
		ENABLE_SWIG="true"
		AC_MSG_RESULT([yes])
	fi
],
[
	AC_MSG_RESULT([auto (default)])
])
if test "${HAVE_CYGWIN}" = "true"; then
	if test ! "$ENABLE_SWIG" = "true"; then
		ENABLE_SWIG="false"
		SWIG=""
		AC_MSG_RESULT([(Disable swig by default in Cygwin)])
	fi
else
	if test "$ENABLE_SWIG" = "auto"; then
		ENABLE_SWIG="true"
	fi
fi
AC_SUBST(ENABLE_SWIG)
AM_CONDITIONAL(ENABLE_SWIG, test "${ENABLE_SWIG}" = "true")

#Check owperl
AC_MSG_CHECKING(if owperl is enabled)
ENABLE_OWPERL="true"
AC_ARG_ENABLE(owperl,
[  --enable-owperl         Enable owperl module (default true)],
[
	AC_MSG_RESULT([$enableval])
	if test ! "$enableval" = "yes" ; then
		ENABLE_OWPERL="false"
	else
		if test -z "$SWIG"  ; then
			AC_MSG_WARN([Cannot find swig program. Look for it at http://www.swig.org or use CPAN])
			AC_MSG_WARN([OWPERL is disabled because swig is not found])
			ENABLE_OWPERL="false"
		fi
	fi
],
[
	if test -z "$SWIG" ; then
		AC_MSG_RESULT([no (swig disabled)])
		ENABLE_OWPERL="false"
	else
		AC_MSG_RESULT([yes (default)])
	fi
])

PERL=""
if test "${ENABLE_OWPERL}" = "true"; then
	m4_include(module/swig/perl5/perl5.m4)
	AC_MSG_RESULT(Looking for location of Perl executable)
	SC_PATH_PERL5
	AC_SUBST(PERL5EXT)
	AC_SUBST(PERL5DYNAMICLINKING)
	AC_SUBST(PERL5LIB)
	AC_SUBST(PERL5DIR)
	AC_SUBST(PERL5NAME)
	AC_SUBST(PERL5CCFLAGS)
	if test -z "${PERL}" ; then
		AC_MSG_WARN([Cannot find perl binary.])
		AC_MSG_WARN([OWPERL is disabled because perl binary is not found])
		ENABLE_OWPERL="false"
	else
		if test -z "${PERL5NAME}" ; then
			AC_MSG_WARN([Cannot find perl library. Install perl-devel package.])
			AC_MSG_WARN([OWPERL is disabled because perl library is not found])
			ENABLE_OWPERL="false"
		fi
	fi
fi
AC_SUBST(PERL)
AC_SUBST(ENABLE_OWPERL)
AM_CONDITIONAL(ENABLE_OWPERL, test "${ENABLE_OWPERL}" = "true")
if test -z "$PERL" ; then
	ENABLE_PERL="false"
else
	ENABLE_PERL="true"
fi
AC_SUBST(ENABLE_PERL)
AM_CONDITIONAL(ENABLE_PERL, test "${ENABLE_PERL}" = "true")

#Check owphp
AC_MSG_CHECKING(if owphp is enabled)
ENABLE_OWPHP="true"
AC_ARG_ENABLE(owphp,
[  --enable-owphp          Enable owphp module (default true)],
[
	AC_MSG_RESULT([$enableval])
	if test "$enableval" = "no" ; then
		ENABLE_OWPHP="false"
	fi
	if test "$enableval" = "yes" ; then
		ENABLE_OWPHP="true"
	fi
	if test -z "$SWIG"  ; then
		AC_MSG_WARN([Cannot find swig program. Look for it at http://www.swig.org or use CPAN])
		AC_MSG_WARN([OWPHP is disabled because swig is not found])
		ENABLE_OWPHP="false"
	fi
],
[
	if test -z "$SWIG" ; then
		AC_MSG_RESULT([no (swig disabled)])
		ENABLE_OWPHP="false"
	else
		AC_MSG_RESULT([yes (default)])
	fi
])

PHP=""
if test "${ENABLE_OWPHP}" = "true" ; then
	m4_include(module/swig/php/php.m4)
	AC_MSG_RESULT(Looking for location of Php executable)
	SC_PATH_PHP 		
	AC_SUBST(PHPCONFIG)
	AC_SUBST(PHPINC)
	AC_SUBST(PHPEXT)
	AC_SUBST(PHPLIBDIR)
	if test -z "${PHP}" ; then
		AC_MSG_WARN([Cannot find php binary. Install php or php5 package])
		AC_MSG_WARN([OWPHP is disabled because php binary is not found])
		ENABLE_OWPHP="false"
	else
		if test -z "${PHPCONFIG}" ; then
			AC_MSG_WARN([Cannot find php-config binary. Install php-devel or php5-dev package])
			AC_MSG_WARN([include and library paths will be guessed])
		fi
		if test -z "${PHPINC}" ; then
			AC_MSG_WARN([Cannot find php include-file. Install php-devel or php5-dev package])
			AC_MSG_WARN([OWPHP is disabled because php include-file is not found])
			ENABLE_OWPHP="false"
		else
			if test -z "${PHPLIBDIR}" ; then
				AC_MSG_WARN([Cannot find php extension-dir. Install php-devel or php5-dev package])
				AC_MSG_WARN([OWPHP is disabled because php extension-dir is not found])
				ENABLE_OWPHP="false"
			fi
		fi
	fi
fi
AC_SUBST(PHP)
AC_SUBST(ENABLE_OWPHP)
AM_CONDITIONAL(ENABLE_OWPHP, test "${ENABLE_OWPHP}" = "true")
if test -z "$PHP" ; then
	ENABLE_PHP="false"
else
	ENABLE_PHP="true"
fi
AC_SUBST(ENABLE_PHP)
AM_CONDITIONAL(ENABLE_PHP, test "${ENABLE_PHP}" = "true")


#Check owpython
AC_MSG_CHECKING(if owpython is enabled)
ENABLE_OWPYTHON="true"
AC_ARG_ENABLE(owpython,
[  --enable-owpython       Enable owpython module (default true)],
[
	AC_MSG_RESULT([$enableval])
	if test "$enableval" = "no" ; then
		ENABLE_OWPYTHON="false"
	fi
	if test "$enableval" = "yes" ; then
		ENABLE_OWPYTHON="true"
		if test -z "$SWIG"  ; then
			AC_MSG_WARN([Cannot find swig program. Look for it at http://www.swig.org or use CPAN])
			AC_MSG_WARN([OWPYTHON is disabled because swig is not found])
			ENABLE_OWPYTHON="false"
		fi
	fi
],
[
	if test -z "$SWIG" ; then
		AC_MSG_RESULT([no (swig disabled)])
		ENABLE_OWPYTHON="false"
	else
		AC_MSG_RESULT([yes (default)])
	fi
])

PYTHON=""
if test "${ENABLE_OWPYTHON}" = "true" ; then
	m4_include(module/swig/python/python.m4)
	AC_MSG_RESULT(Looking for location of Python executable)
	SC_PATH_PYTHON
	AC_SUBST(PYVERSION)
	AC_SUBST(PYSITEDIR)
	AC_SUBST(PYTHONDYNAMICLINKING)
	if test -z "${PYTHON}" ; then
		AC_MSG_WARN([Cannot find python binary. Install python package.])
		AC_MSG_WARN([OWPYTHON is disabled because python binary is not found])
		ENABLE_OWPYTHON="false"
	else
		if test -z "${PYLDFLAGS}" ; then
			AC_MSG_WARN([Cannot find python library. Install python-devel package.])
			AC_MSG_WARN([OWPYTHON is disabled because python library is not found])
			ENABLE_OWPYTHON="false"
		else
			if test -z "${PYCFLAGS}" ; then
				AC_MSG_WARN([Cannot find python include-file. Install python-devel package.])
				AC_MSG_WARN([OWPYTHON is disabled because python include-file is not found])
				ENABLE_OWPYTHON="false"
			fi
		fi
	fi
fi
AC_SUBST(ENABLE_OWPYTHON)
AM_CONDITIONAL(ENABLE_OWPYTHON, test "${ENABLE_OWPYTHON}" = "true")
AC_SUBST(PYTHON)
if test -z "$PYTHON" ; then
	ENABLE_PYTHON="false"
else
	ENABLE_PYTHON="true"
fi
AC_SUBST(ENABLE_PYTHON)
AM_CONDITIONAL(ENABLE_PYTHON, test "${ENABLE_PYTHON}" = "true")

#Check owtcl
AC_MSG_CHECKING(if owtcl is enabled)
ENABLE_OWTCL="true"
AC_ARG_ENABLE(owtcl,
[  --enable-owtcl          Enable owtcl module (default true)],
[
	AC_MSG_RESULT([$enableval])
	if test "$enableval" = "no" ; then
		ENABLE_OWTCL="false"
	fi
	if test "$enableval" = "yes" ; then
		ENABLE_OWTCL="true"
	fi
],
[
	AC_MSG_RESULT([yes (default)])
])

if test "${ENABLE_OWTCL}" = "true" ; then
	m4_include(module/owtcl/tcl.m4)
	AC_MSG_RESULT(Looking for tclConfig.sh)
	SC_PATH_TCLCONFIG
	if test -z "$TCL_BIN_DIR" -o ! -f "$TCL_BIN_DIR/tclConfig.sh" ; then
		AC_MSG_WARN([OWTCL is disabled because tclConfig.sh is not found])
		ENABLE_OWTCL="false"
	else
		SC_LOAD_TCLCONFIG
		AC_SUBST(TCL_BIN_DIR)
		AC_SUBST(TCL_SHLIB_CFLAGS)
		AC_SUBST(TCL_SHLIB_LD)
		AC_SUBST(TCL_SHLIB_LD_LIBS)
		AC_SUBST(TCL_SHLIB_SUFFIX)
		AC_SUBST(TCL_VERSION)
		AC_SUBST(TCL_PREFIX)
		AC_SUBST(TCL_EXEC_PREFIX)
		AC_SUBST(TCL_CFLAGS)
		AC_SUBST(TCL_DEFS)
		AC_SUBST(TCL_LIB_SPEC)
		AC_SUBST(TCL_LIBS)
		AC_SUBST(TCL_LD_FLAGS)
		AC_SUBST(TCL_COMPAT_OBJS)
		AC_SUBST(TCL_LD_SEARCH_FLAGS)
		AC_SUBST(TCL_SRC_DIR)
		AC_SUBST(TCL_LIB_SPEC)
		AC_SUBST(TCL_STUB_LIB_SPEC)
		AC_SUBST(TCL_BUILD_LIB_SPEC)
		AC_SUBST(TCL_BUILD_STUB_LIB_SPEC)
		AC_SUBST(TCL_INCLUDE_SPEC)
		AC_SUBST(TCL_PACKAGE_PATH)
		# guess that tcl will install the files under the first path in
		# TCL_PACKAGE_PATH. TCL_BIN_DIR is usually correct, but this
		# might contain staging_dir prefix when cross-compiling.
		OWTCL_INSTALL_PATH="`echo ${TCL_PACKAGE_PATH} | cut -d' ' -f1`"
		case "${host_cpu}-${host_os}" in
			*-darwin*)
				OWTCL_INSTALL_PATH="`echo ${TCL_BIN_DIR} | cut -d' ' -f1`"
				;;
			*)
				OWTCL_INSTALL_PATH="`echo ${TCL_PACKAGE_PATH} | cut -d' ' -f1`"
				;;
		esac
		# Debian Hack: do not install in /usr/local/lib/tcltk
		# OWTCL_INSTALL_PATH="/usr/lib/tcltk"
		AC_SUBST(OWTCL_INSTALL_PATH)
	fi
fi
AC_SUBST(ENABLE_OWTCL)
AM_CONDITIONAL(ENABLE_OWTCL, test "${ENABLE_OWTCL}" = "true")


AC_DEFUN([ACX_EXPAND], [
	EXP_VAR=[$1]
	FROM_VAR=[$2]

	dnl first expand prefix and exec_prefix if necessary
	prefix_save=$prefix
	exec_prefix_save=$exec_prefix

	dnl if no prefix given, then use /usr/local, the default prefix
	if test "x$prefix" = "xNONE"; then
		prefix=$ac_default_prefix
	fi
	dnl if no exec_prefix given, then use prefix
	if test "x$exec_prefix" = "xNONE"; then
		exec_prefix=$prefix
	fi

	full_var="$FROM_VAR"
	dnl loop until it doesnt change anymore
	while true; do
		new_full_var="`eval echo $full_var`"
		if test "x$new_full_var"="x$full_var"; then break; fi
		full_var=$new_full_var
	done

	dnl clean up
	full_var=$new_full_var
	AC_SUBST([$1], "$full_var")

	dnl restore prefix and exec_prefix
	prefix=$prefix_save
	exec_prefix=$exec_prefix_save
]) dnl ACX_EXPAND

ACX_EXPAND(LIBDIR, $libdir)
ACX_EXPAND(BINDIR, $bindir)
ACX_EXPAND(DATADIR, $datadir)

EXTRACFLAGS="-D_FILE_OFFSET_BITS=64"
AC_MSG_CHECKING([if cflag _XOPEN_SOURCE is required])
case "${host_cpu}-${host_os}" in
	*-aix* | *solaris* | *-osf* | *-hpux*)
		CFLAGS="$CFLAGS -D_XOPEN_SOURCE=500"
		EXTRACFLAGS="$EXTRACFLAGS -D_XOPEN_SOURCE=500"
		AC_MSG_RESULT([-D_XOPEN_SOURCE=500])
	;;
	*-darwin* | *-netbsd*)
		AC_MSG_RESULT([no])
	;;
	*-freebsd* | *)
		# have to add this to support pthread_rwlock_t
		CFLAGS="$CFLAGS -D_XOPEN_SOURCE=600"
		EXTRACFLAGS="$EXTRACFLAGS -D_XOPEN_SOURCE=600"
		AC_MSG_RESULT([-D_XOPEN_SOURCE=600])
	;;
esac

AC_MSG_CHECKING([if cflag _BSD_SOURCE is required])
case "${host_cpu}-${host_os}" in
 	*-netbsd*)
 		CFLAGS="$CFLAGS -D_NETBSD_SOURCE=1"
 		EXTRACFLAGS="$EXTRACFLAGS -D_NETBSD_SOURCE=1"
 		{ $as_echo "$as_me:${as_lineno-$LINENO}: result: -D_NETBSD_SOURCE=1" >&5 $as_echo "-D_NETBSD_SOURCE=1" >&6; }
 	;;
	*)
		# have to add -D_BSD_SOURCE=1 to support major() in sys/sysmacros.h
		# have to add -D_ISOC99_SOURCE=1 to support lrint() in bits/mathcalls.h
                # have to add -D_POSIX_C_SOURCE=200112L for sem_timedwait
                # have to add -D_POSIX_C_SOURCE=199309L for clock_gettime()
		CFLAGS="$CFLAGS -D_BSD_SOURCE=1 -D_ISOC99_SOURCE=1 -D_POSIX_C_SOURCE=200112L"
		EXTRACFLAGS="$EXTRACFLAGS -D_BSD_SOURCE=1 -D_ISOC99_SOURCE=1 -D_POSIX_C_SOURCE=200112L"
		AC_MSG_RESULT([-D_BSD_SOURCE=1 -D_ISOC99_SOURCE=1 -D_POSIX_C_SOURCE=200112L])
	;;
esac

# should perhaps add _GNU_SOURCE=1 instead of the two defines above...
# That would define _BSD_SOURCE, _SVID_SOURCE, _LARGEFILE64_SOURCE,
# _XOPEN_SOURCE=600, _POSIX_C_SOURCE, _POSIX_SOURCE, _ISOC99_SOURCE


# Check if extra libs are needed. Should perhaps test if they are available too
# instead of hardcoding the libraries.
EXTRALIBS=""
AC_MSG_CHECKING([if extra libs are required])
case "${host_cpu}-${host_os}" in
	*-darwin*)
		# This will actually be duplicates if libusb is used.
		OSLIBS="-Wl,-framework -Wl,IOKit -Wl,-framework -Wl,CoreFoundation"
		EXTRALIBS=""
		AC_MSG_RESULT([${OSLIBS}])
		;;
	*-solaris*)
		OSLIBS=""
		EXTRALIBS=" -lsocket -lnsl "
		LIBS="$LIBS -lsocket -lnsl"
		AC_MSG_RESULT([${EXTRALIBS}])
		;;
	*)
		OSLIBS=""
		EXTRALIBS=""
		AC_MSG_RESULT([${EXTRALIBS}])
		;;
esac

#Check for profiling
AC_MSG_CHECKING(if profiling is enabled)
ENABLE_PROFILING="false"
AC_ARG_ENABLE(profiling,
[  --enable-profiling      Enable profiling (default false)],
[
	AC_MSG_RESULT([$enableval])
	if test "$enableval" = "yes" ; then
		ENABLE_PROFILING=true
		EXTRALIBS="$EXTRALIBS -pg"
		EXTRACFLAGS="$EXTRACFLAGS -pg"
	fi
],
[
	AC_MSG_RESULT([no (default)])
])
AC_SUBST(EXTRACFLAGS)
AC_SUBST(ENABLE_PROFILING)
AM_CONDITIONAL(ENABLE_PROFILING, test "${ENABLE_PROFILING}" = "true")
ACX_EXPAND(LD_EXTRALIBS, $EXTRALIBS)
AC_SUBST(LD_EXTRALIBS)
AC_SUBST(OSLIBS)
AC_MSG_WARN([LD_EXTRALIBS=${LD_EXTRALIBS} OSLIBS=${OSLIBS}])

AC_ARG_WITH(fuseinclude,
            [  --with-fuseinclude=DIR  FUSE-include from @<:@/usr/local/include@:>@],
            [fuse_include_path=$withval],
            [fuse_include_path='/usr/local/include'])
AC_SUBST(fuse_include_path)

AC_ARG_WITH(fuselib,
            [  --with-fuselib=DIR      FUSE-lib from @<:@/usr/local/lib@:>@],
            [fuse_lib_path=$withval],
            [fuse_lib_path='/usr/local/lib'])
AC_SUBST(fuse_lib_path)


#Check owfs
AC_MSG_CHECKING([if owfs is enabled])
ENABLE_OWFS="auto"
AC_ARG_ENABLE(owfs,
[  --enable-owfs           Enable owfs module (default auto)],
[
	AC_MSG_RESULT([$enableval])
	if test "$enableval" = "yes" ; then
		ENABLE_OWFS="true"
	fi
	if test "$enableval" = "no" ; then
		ENABLE_OWFS="false"
	fi
],
[
    AC_MSG_RESULT([auto (default)])
])

# We need fuse only if OWFS is enabled
if test "${ENABLE_OWFS}" != "false" ; then

	save_LD_EXTRALIBS="$LD_EXTRALIBS"
	save_CPPFLAGS="$CPPFLAGS"
	save_LDFLAGS="$LDFLAGS"
	FUSE_FLAGS="-DFUSE_USE_VERSION=26"
	FUSE_INCLUDES="-I${fuse_include_path}"
	FUSE_LIBS="-L${fuse_lib_path}"
	LD_EXTRALIBS="$save_LD_EXTRALIBS "
	CPPFLAGS="$save_CPPFLAGS -D_FILE_OFFSET_BITS=64 $FUSE_FLAGS $FUSE_INCLUDES"
	LDFLAGS="$save_LDFLAGS $FUSE_LIBS"

	AC_CHECK_HEADER(fuse.h,,[
	AC_MSG_WARN([
	Cannot find fuse.h - Add the search path with --with-fuseinclude])

	FUSE_FLAGS=""
	FUSE_INCLUDES=""
	FUSE_LIBS=""
	LD_EXTRALIBS="$save_LD_EXTRALIBS"
	CPPFLAGS="$save_CPPFLAGS -D_FILE_OFFSET_BITS=64"
	LDFLAGS="$save_LDFLAGS"

	AC_MSG_WARN([Install FUSE-2.2 or later to enable owfs - download it from http://fuse.sourceforge.net/])

	if test "${ENABLE_OWFS}" = "auto"; then
		AC_MSG_WARN([OWFS is disabled because fuse.h is not found.])
		ENABLE_OWFS="false"
	else
		AC_MSG_ERROR([Configure without --enable-owfs to detect fuse automatically.])
	fi
	])

	if test "${ENABLE_OWFS}" != "false"; then
	save_CFLAGS="$CFLAGS"
	save_LIBS="$LIBS"
	CFLAGS="$CFLAGS $PTHREAD_CFLAGS"
	LIBS="$LIBS $PTHREAD_LIBS"
	AC_CHECK_LIB(fuse,fuse_main, [FUSE_LIBS="$FUSE_LIBS -lfuse"],[
		AC_MSG_WARN([
	Cannot find libfuse.a - add the search path with --with-fuselib])
		AC_MSG_WARN([Running ldconfig or adding "/usr/local/lib" to /etc/ld.so.conf might also solve the problem, otherwise re-install fuse.])
		if test "${ENABLE_OWFS}" = "auto"; then
			AC_MSG_WARN([OWFS is disabled because libfuse.a is not found.])
			ENABLE_OWFS="false"
		else
			AC_MSG_ERROR([Cannot enable OWFS])
		fi
		],)
	CFLAGS="$save_CFLAGS"
	LIBS="$save_LIBS"
	fi


	if test "${ENABLE_OWFS}" != "false"; then
		# check for a supported FUSE_MAJOR_VERSION.
		AC_MSG_CHECKING([For supported FUSE API version])
		AC_COMPILE_IFELSE([
		AC_LANG_PROGRAM([[#include <fuse.h>]],
		[[
		#ifndef FUSE_MAJOR_VERSION
		#error "FUSE_MAJOR_VERSION not defined"
		#endif
		]])],
		[AC_MSG_RESULT([yes])],
		[AC_MSG_RESULT([no])
		AC_MSG_WARN([OWFS is disabled since fuse.h is too old])
		if test "${ENABLE_OWFS}" = "true"; then
			AC_MSG_ERROR([You have to install fuse first (fuse-2.2 or later recommended) - download it from http://fuse.sourceforge.net/])
		else
			ENABLE_OWFS="false"
		fi
		])
	fi


	# Use newest FUSE API if version is newer than 2.2
	if test "${ENABLE_OWFS}" != "false"; then
		# check for a supported FUSE_MAJOR_VERSION.
		AC_MSG_CHECKING([For FUSE version ])
		AC_COMPILE_IFELSE([
		AC_LANG_PROGRAM([[#include <fuse.h>]],
		[[
		#ifndef FUSE_VERSION
			#ifndef FUSE_MAJOR_VERSION
				#define FUSE_VERSION 11
			#else /* FUSE_MAJOR_VERSION */
				#undef FUSE_MAKE_VERSION
				#define FUSE_MAKE_VERSION(maj,min)  ((maj) * 10 + (min))
				#define FUSE_VERSION FUSE_MAKE_VERSION(FUSE_MAJOR_VERSION,FUSE_MINOR_VERSION)
			#endif /* FUSE_MAJOR_VERSION */
		#endif /* FUSE_VERSION */
		#if (FUSE_VERSION >= 22)
			/* Fuse > 2.2 is ok */
		#else
			#error "Fuse < 2.2"
		#endif
		]])],
		[AC_MSG_RESULT([2.2 or later])
		],
		[AC_MSG_RESULT([<2.2])
		FUSE_FLAGS=""
		])
	fi

	CPPFLAGS="$save_CPPFLAGS"
	LDFLAGS="$save_LDFLAGS"
	LD_EXTRALIBS="$save_LD_EXTRALIBS"

	if test "${ENABLE_OWFS}" != "false"; then
		ENABLE_OWLIB="true"
		ENABLE_OWFS="true"
	fi
else
    ENABLE_OWFS="false"
fi
AC_SUBST(FUSE_LIBS)
AC_SUBST(FUSE_FLAGS)
AC_SUBST(FUSE_INCLUDES)
AC_SUBST(ENABLE_OWFS)
AM_CONDITIONAL(ENABLE_OWFS, test "${ENABLE_OWFS}" = "true")

# Check if the zeroconf/bonjour is enabled.
AC_MSG_CHECKING(if zeroconf/bonjour is enabled)
ENABLE_ZERO="true"
AC_ARG_ENABLE(zero,
[  --enable-zero           Enable zeroconf/bonjour (default true)],
[
	AC_MSG_RESULT([$enableval])

	if test ! "$enableval" = "yes" ; then
		ENABLE_ZERO=false
	fi
],
[
	AC_MSG_RESULT([yes (default)])
])
AC_SUBST(ENABLE_ZERO)
AM_CONDITIONAL(ENABLE_ZERO, test "${ENABLE_ZERO}" = "true")


LIBUSB_CFLAGS=""
LIBUSB_LIBS=""


# Check for USB enabled
ENABLE_USB=auto
AC_MSG_CHECKING([if usb support is enabled])

AC_ARG_ENABLE(usb,
[  --enable-usb            Enable 1-Wire usb DS2490 support (default auto)],
[
	AC_MSG_RESULT([$enableval])

	if ! test "$enableval" = "yes" ; then
		ENABLE_USB=false
	fi
],
[
	AC_MSG_RESULT([auto (default)])
])

if test "${ENABLE_USB}" != "false"; then
	#LIBUSB_CONFIG
	PKG_CHECK_MODULES([LIBUSB], [libusb-1.0 >= 0.9.1], [ENABLE_USB=true],[ENABLE_USB=false])
fi

AC_SUBST(LIBUSB_CFLAGS)
AC_SUBST(LIBUSB_LIBS)
AC_SUBST(ENABLE_USB)
AM_CONDITIONAL(ENABLE_USB, test "${ENABLE_USB}" = "true")

AM_CPPFLAGS="$AM_CPPFLAGS $LIBUSB_CFLAGS"

# Check for Avahi enabled
ENABLE_AVAHI=auto
AC_MSG_CHECKING([if Avahi support is enabled])

AC_ARG_ENABLE(avahi,
[  --enable-avahi          Enable zero-config autodiscovery (default auto)],
[
	AC_MSG_RESULT([$enableval])

	if ! test "$enableval" = "yes" ; then
		ENABLE_AVAHI=false
	fi
],
[
	AC_MSG_RESULT([auto (default)])
])

if test "${ENABLE_AVAHI}" != "false"; then

	# Include libavahi if the avahi is enabled
	if test "X${LIBAVAHI_CONFIG}" != "X" ; then

		LIBAVAHI_CFLAGS=`$LIBAVAHI_CONFIG --cflags`
		LIBAVAHI_LIBS=`$LIBAVAHI_CONFIG --libs`

		save_CPPFLAGS="$CPPFLAGS"
		save_LDFLAGS="$LDFLAGS"
		CPPFLAGS="$save_CPPFLAGS $LIBAVAHI_CFLAGS"
		LDFLAGS="$save_LDFLAGS $LIBAVAHI_LIBS"

		AC_MSG_CHECKING([if libavahi compiles with includes+lib ])
		AC_COMPILE_IFELSE([
		AC_LANG_PROGRAM([[#include <avahi-chient/client.h>]],
		[[
			avahi_client_new(NULL,0,NULL);
		]])],
		[AC_MSG_RESULT([ok])],
		[AC_MSG_RESULT([compilation error])
		LIBAVAHI_CFLAGS=""
		LIBAVAHI_LDFLAGS=""
		LIBAVAHI_CONFIG=""
		])

		CPPFLAGS="$save_CPPFLAGS"
		LDFLAGS="$save_LDFLAGS"

		#LIBAVAHI_CONFIG
	fi

	# ultimate test to see if libavahi is found after AC_CHECK_LIB
	if test "X${LIBAVAHI_CONFIG}" == "X"; then

	    PKG_CHECK_MODULES([LIBAVAHI], [avahi-client], [ dnl ok, avahi found with pkg-config
	    ],[ dnl else trying basic detection
		AC_CHECK_LIB(avahi-common,avahi_threaded_poll_new,[
			AC_CHECK_LIB(avahi-client,avahi_client_new,[
				LIBAVAHI_LIBS="-lavahi-client -lavahi-common"],[
				AC_MSG_WARN([Cannot find libavahi-client])
				if test "${ENABLE_AVAHI}" = "true" ; then
					AC_MSG_ERROR([libavahi must be installed to use a Avahi zero-configuration])
				else
					AC_MSG_WARN([libavahi not found, avahi will be disabled])
					ENABLE_AVAHI=false
				fi
			],)],[
			AC_MSG_WARN([Cannot find libavahi-common])
			if test "${ENABLE_AVAHI}" = "true" ; then
				AC_MSG_ERROR([libavahi must be installed to use a Avahi zero-configuration])
			else
				AC_MSG_WARN([libavahi not found, avahi will be disabled])
				ENABLE_AVAHI=false
			fi
		],)
	    ])

	fi

	if test "${ENABLE_AVAHI}" != "false" ; then
	ENABLE_AVAHI=true
	fi

	# ENABLE_AVAHI
fi

AC_SUBST(LIBAVAHI_CFLAGS)
AC_SUBST(LIBAVAHI_LIBS)
AC_SUBST(ENABLE_AVAHI)
AM_CONDITIONAL(ENABLE_AVAHI, test "${ENABLE_AVAHI}" = "true")

# Check for Parallel port enabled
ENABLE_PARPORT=auto
AC_MSG_CHECKING([if parallel port support is enabled])

AC_ARG_ENABLE(parport,
[  --enable-parport        Enable 1-Wire parallel port DS1410E support (default auto)],
[
	AC_MSG_RESULT([$enableval])

	if ! test "$enableval" = "yes" ; then
		ENABLE_PARPORT=false
	fi
],
[
	AC_MSG_RESULT([yes (default)])
])

# Include linux/ppdev.h if the parallel port is enabled
if test "${ENABLE_PARPORT}" != "false" ; then

	AC_CHECK_HEADER(linux/ppdev.h,,[
		if test "${ENABLE_PARPORT}" = "true" ; then
			AC_MSG_ERROR([ppdev.h must be installed to use parallel port adapter])
		else
			AC_MSG_WARN([ppdev.h not found, parallel port will be disabled])
			ENABLE_PARPORT=false
		fi
	])
	if test "${ENABLE_PARPORT}" != "false" ; then
		ENABLE_PARPORT=true
	fi
fi

AC_SUBST(ENABLE_PARPORT)
AM_CONDITIONAL(ENABLE_PARPORT, test "${ENABLE_PARPORT}" = "true")


dnl We support both libftdi 1.x and libftdi 0.x
if test "$cross_compiling" != yes; then
AC_CHECK_PROGS(LIBFTDI_CONFIG, libftdi1-config libftdi-config)
else
LIBFTDI_CONFIG=""
fi
AC_SUBST(LIBFTDI_CONFIG)

AC_ARG_WITH(libftdi-config,
            [  --with-libftdi-config=PATH Specify full path to libftdi-config or libftdi1-config])
dnl Check if user passed a specific libftdi-config program.
if test "X$with_libftdi_config" != "X" ; then
	LIBFTDI_CONFIG=$with_libftdi_config
fi

if test "$cross_compiling" != yes; then
	if test "X$LIBFTDI_CONFIG" == "X" ; then
		dirs="/usr/bin /usr/local/bin /opt/local/bin"
		for i in $dirs; do
			for prog in libftdi1-config libftdi-config; do
				echo "Testing $i/$prog"
				if test -x $i/$prog; then
					AC_MSG_RESULT($i/$prog is found)
					LIBFTDI_CONFIG="$i/$prog"
					break 2;
				fi
			done
		done
	fi
fi

# Check if FTDI should be enabled
ENABLE_FTDI=auto
AC_MSG_CHECKING([if libftdi is available])

AC_ARG_ENABLE(ftdi,
[  --enable-ftdi          Enable LinkUSB support via libftdi (default auto)],
[
	AC_MSG_RESULT([$enableval])

	if ! test "$enableval" = "yes" ; then
		ENABLE_FTDI=false
	fi
],
[
	AC_MSG_RESULT([auto (default)])
])

# Include ftdi.h if enabled
if test "${ENABLE_FTDI}" != "false" ; then

	LIBFTDI_FOUND=false
	if test "X${LIBFTDI_CONFIG}" != "X" ; then
		LIBFTDI_CFLAGS=`$LIBFTDI_CONFIG --cflags`
		LIBFTDI_LIBS=`$LIBFTDI_CONFIG --libs`

		save_CPPFLAGS="$CPPFLAGS"
		save_LDFLAGS="$LDFLAGS"
		CPPFLAGS="$save_CPPFLAGS $LIBFTDI_CFLAGS"
		LDFLAGS="$save_LDFLAGS $LIBFTDI_LIBS"

		AC_MSG_CHECKING([if libftdi compiles with includes+lib ])
		AC_COMPILE_IFELSE([
		AC_LANG_PROGRAM([[#include <ftdi.h>]],
		[[
			struct ftdi_context ftdic;
			ftdi_init(&ftdic);
		]])],
		[AC_MSG_RESULT([ok])
		FTDI_FOUND=true
		],
		[AC_MSG_RESULT([compilation error])
		LIBFTDI_CFLAGS=""
		LIBFTDI_LDFLAGS=""
		LIBFTDI_CONFIG=""
		])

		CPPFLAGS="$save_CPPFLAGS"
		LDFLAGS="$save_LDFLAGS"
	fi

	if test "${FTDI_FOUND}" = "true"; then
		ENABLE_FTDI=true
	else
		if test "${ENABLE_FTDI}" = "true" ; then
			AC_MSG_ERROR([libftdi must be installed to use LinkUSB natively])
		else
			AC_MSG_WARN([libftdi not found, LinkUSB native will be disabled])
			ENABLE_FTDI=false
		fi
	fi
fi
AC_SUBST(LIBFTDI_CFLAGS)
AC_SUBST(LIBFTDI_LIBS)
AC_SUBST(ENABLE_FTDI)
AM_CONDITIONAL(ENABLE_FTDI, test "${ENABLE_FTDI}" = "true")

if test "${HAVE_CYGWIN}" = "true" ; then
	OW_CYGWIN=1
else
	OW_CYGWIN=0
fi
AC_SUBST(OW_CYGWIN)

if test "${HAVE_DARWIN}" = "true" ; then
	OW_DARWIN=1
else
	OW_DARWIN=0
fi
AC_SUBST(OW_DARWIN)


AC_CHECK_TYPES([struct sockaddr_storage], , ,
[[
	#ifdef HAVE_SYS_TYPES_H
		#include <sys/types.h>
	#endif
	#ifdef HAVE_SYS_SOCKET_H
		#include <sys/socket.h>
	#endif
	#ifdef HAVE_NETINET_IN_H
		#include <netinet/in.h>
	#endif
	#ifdef HAVE_ARPA_INET_H
		#include <arpa/inet.h>
	#endif
]]
)

if test "$ac_cv_type_struct_sockaddr_storage" = no; then
	AC_CHECK_MEMBER([struct sockaddr_in.sin_len],
	[AC_DEFINE(SOCKADDR_IN_HAS_LEN, 1, [Define to 1 if sockaddr_in has a 'sin_len' member.])])
fi
AC_CHECK_TYPES([struct addrinfo], , , [[#include <netdb.h>]])
AC_CHECK_TYPE(socklen_t, ,
[AC_DEFINE([socklen_t], [unsigned int],
[If we do not have a real socklen_t, unsigned int is good enough.])],
[
	#ifdef HAVE_SYS_TYPES_H
		#include <sys/types.h>
	#endif
	#ifdef HAVE_SYS_SOCKET_H
		#include <sys/socket.h>
	#endif
])


AC_MSG_CHECKING(for AF_NETLINK)
AC_COMPILE_IFELSE(
[AC_LANG_PROGRAM(
[
	#ifdef HAVE_SYS_TYPES_H
		#include <sys/types.h>
	#endif
	#ifdef HAVE_SYS_SOCKET_H
		#include <sys/socket.h>
	#endif
],
[int ss_family = AF_NETLINK;])],
[
	AC_MSG_RESULT(yes)
	have_af_netlink="yes"
	AC_DEFINE(HAVE_AF_NETLINK, 1, [Have AF_NETLINK in socket.h])
],
[AC_MSG_RESULT(no)
have_af_netlink="no"])

if test "${have_af_netlink}" != "yes" ; then
   if test "${ENABLE_W1}" != "false" ; then
      AC_MSG_WARN([Disable w1 since AF_NETLINK not found])
      ENABLE_W1="false"
   fi
fi


AC_MSG_CHECKING(for broken glibc with __ss_family)
AC_COMPILE_IFELSE(
[AC_LANG_PROGRAM(
[
	#ifdef HAVE_SYS_TYPES_H
		#include <sys/types.h>
	#endif
	#ifdef HAVE_SYS_SOCKET_H
		#include <sys/socket.h>
	#endif
],
[struct sockaddr_storage s; s.__ss_family = AF_INET;])],
[
	AC_MSG_RESULT(yes)
	AC_DEFINE(HAVE_BROKEN_SS_FAMILY, 1, [Broken glibc implementations use __ss_family instead of ss_family.])
],
[AC_MSG_RESULT(no)])


AC_MSG_CHECKING([for core ipv6 support])
AC_COMPILE_IFELSE(
[AC_LANG_PROGRAM(
	[[#define IN_AUTOCONF
	#include "include/stdinc.h"]],
	[[struct sockaddr_in6 s;
	  struct sockaddr_storage t;
	    s.sin6_family = 0;]]
	    )],
[
	if test "${HAVE_CYGWIN}" = "true" ; then
		AC_MSG_RESULT([no, Cygwins ipv6 is incomplete])
		have_v6=no
	else
		have_v6=yes
		AC_DEFINE(IPV6, 1, [Define if ipv6 support is present and available.])
		AC_MSG_RESULT(yes)
		AC_MSG_CHECKING([for struct in6addr_any])
		AC_COMPILE_IFELSE(
			[AC_LANG_PROGRAM(
				[[#define IN_AUTOCONF
				  #include "include/stdinc.h"]],
				  [[struct in6_addr a = in6addr_any;]]
			)],
			[AC_MSG_RESULT(yes)],
			[
				AC_MSG_RESULT(no)
				AC_DEFINE(NO_IN6ADDR_ANY, 1, [Define to 1 if your system has no in6addr_any.])
				inet_misc=1
			]
		)
	fi
],
[AC_MSG_RESULT(no)
have_v6="no"])


AC_CACHE_CHECK(if sem_timedwait exists,ac_cv_sem_timedwait, [
   save_LIBS="$LIBS"; LIBS="$LIBS $PTHREAD_LIBS" ; save_CFLAGS="$CFLAGS"; CFLAGS="$CFLAGS $PTHREAD_CFLAGS"
AC_TRY_LINK([
#if HAVE_STDLIB_H
#include <stdlib.h>
#endif
#if HAVE_SEMAPHORE_H
#include <semaphore.h>
#endif
],
    [sem_timedwait(NULL, NULL)],[
    ac_cv_sem_timedwait="yes"],[
    AC_MSG_WARN([Cannot find sem_timedwait])
    ac_cv_sem_timedwait="no"
])
LIBS="$save_LIBS"
CFLAGS="$save_CFLAGS"
])

if test "$ac_cv_sem_timedwait" = "yes"; then
    AC_DEFINE(HAVE_SEM_TIMEDWAIT, 1, [Define to 1 if you have the sem_timedwait function.])
fi


AC_CACHE_CHECK([whether string.h and strings.h may both be included],
gcc_cv_header_string,
[
	AC_COMPILE_IFELSE(
		[AC_LANG_SOURCE(
			[[#include <string.h>
			 #include <strings.h>]]
		)],
		[gcc_cv_header_string=yes],
		[gcc_cv_header_string=no]
	)
])

if test "$gcc_cv_header_string" = "yes"; then
	AC_DEFINE(STRING_WITH_STRINGS, 1, [Define to 1 if string.h may be included along with strings.h])
fi

if test "${ENABLE_I2C}" = "true" ; then
	OW_I2C=1
else
	OW_I2C=0
fi
AC_SUBST(OW_I2C)

if test "${ENABLE_W1}" = "true" ; then
	OW_W1=1
else
	OW_W1=0
fi
AC_SUBST(OW_W1)

if test "${ENABLE_USB}" = "true" ; then
	OW_USB=1
else
	OW_USB=0
fi
AC_SUBST(OW_USB)

if test "${ENABLE_AVAHI}" = "true" ; then
	OW_AVAHI=1
else
	OW_AVAHI=0
fi
AC_SUBST(OW_AVAHI)

if test "${ENABLE_ZERO}" = "true" ; then
	OW_ZERO=1
else
	OW_ZERO=0
fi
AC_SUBST(OW_ZERO)

if test "${ENABLE_PARPORT}" = "true" ; then
	OW_PARPORT=1
else
	OW_PARPORT=0
fi
AC_SUBST(OW_PARPORT)

if test "${ENABLE_FTDI}" = "true" ; then
	OW_FTDI=1
else
	OW_FTDI=0
fi
AC_SUBST(OW_FTDI)

if test "${ENABLE_DEBUG}" = "true" ; then
	OW_DEBUG=1
else
	OW_DEBUG=0
fi
AC_SUBST(OW_DEBUG)

if test "${ENABLE_MUTEX_DEBUG}" = "true" ; then
	OW_MUTEX_DEBUG=1
else
	OW_MUTEX_DEBUG=0
fi
AC_SUBST(OW_MUTEX_DEBUG)

if test "${ENABLE_OWMALLOC}" = "true" ; then
	OW_ALLOC_DEBUG=1
else
	OW_ALLOC_DEBUG=0
fi
AC_SUBST(OW_ALLOC_DEBUG)

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
#AC_FUNC_MKTIME
#AC_FUNC_REALLOC
AC_FUNC_SELECT_ARGTYPES
AC_FUNC_STRFTIME
AC_FUNC_STRTOD
AC_TYPE_SIGNAL
AC_CHECK_FUNCS([accept daemon getaddrinfo freeaddrinfo gethostbyname2_r gethostbyaddr_r gethostbyname_r getservbyname_r getopt getopt_long gmtime_r gettimeofday localtime_r inet_ntop inet_pton memchr memset select socket strcasecmp strchr strdup strncasecmp strtol strtoul twalk tsearch tfind tdelete tdestroy vasprintf strsep vsprintf vsnprintf writev getline])

save_LIBS="$LIBS"
DL_LIBS=""
if test "${ENABLE_ZERO}" = "true" ; then
	AC_SEARCH_LIBS(dlopen, dl,
		DL_LIBS="-ldl"
		AC_DEFINE(HAVE_DLOPEN, 1, [Define if you have dlopen]))
fi
AC_SUBST([DL_LIBS])
LIBS="$save_LIBS"

save_LIBS="$LIBS"
M_LIBS=""
AC_SEARCH_LIBS(lrint, m,
       M_LIBS="-lm"
       AC_DEFINE(HAVE_LRINT, 1, [Define if you have lrint]))
AC_SUBST([M_LIBS])
LIBS="$save_LIBS"

AC_SEARCH_LIBS(nanosleep, rt posix4, AC_DEFINE(HAVE_NANOSLEEP, 1, [Define if you have nanosleep]))
# clock_gettime needed if real-time wait is needed for sem_timedwait. gettimeofday is enough.
#AC_SEARCH_LIBS(clock_gettime, rt posix4, AC_DEFINE(HAVE_CLOCK_GETTIME, 1, [Define if you have clock_gettime]))

# systemd support (see man 7 daemon)
AC_ARG_WITH([systemdsystemunitdir],
       [AS_HELP_STRING([--with-systemdsystemunitdir=DIR], [Directory for systemd service files])],,
       [with_systemdsystemunitdir=auto])
AS_IF([test "x$with_systemdsystemunitdir" = "xyes" -o "x$with_systemdsystemunitdir" = "xauto"], [
       def_systemdsystemunitdir=$($PKG_CONFIG --variable=systemdsystemunitdir systemd)

AS_IF([test "x$def_systemdsystemunitdir" = "x"],
       [AS_IF([test "x$with_systemdsystemunitdir" = "xyes"],
               [AC_MSG_ERROR([systemd support requested but pkg-config unable to query systemd package])])
        with_systemdsystemunitdir=no],
       [with_systemdsystemunitdir="$def_systemdsystemunitdir"])])
AS_IF([test "x$with_systemdsystemunitdir" != "xno"],
       [AC_SUBST([systemdsystemunitdir], [$with_systemdsystemunitdir])])
AM_CONDITIONAL([HAVE_SYSTEMD], [test "x$with_systemdsystemunitdir" != "xno"])
save_LIBS="$LIBS"
MQ_LIBS=""
AC_SEARCH_LIBS([mq_getattr], [rt],
		[MQ_LIBS="-lrt"])
AC_SUBST([MQ_LIBS])
LIBS="$save_LIBS"

AC_CONFIG_FILES([
	Makefile
	module/Makefile

	module/owlib/Makefile
	module/owlib/src/Makefile
	module/owlib/src/include/Makefile
	module/owlib/src/c/Makefile

	module/owfs/Makefile
	module/owfs/src/Makefile
	module/owfs/src/include/Makefile
	module/owfs/src/c/Makefile

	module/owhttpd/Makefile
	module/owhttpd/src/Makefile
	module/owhttpd/src/include/Makefile
	module/owhttpd/src/c/Makefile

	module/owserver/Makefile
	module/owserver/src/Makefile
	module/owserver/src/include/Makefile
	module/owserver/src/c/Makefile

	module/owftpd/Makefile
	module/owftpd/src/Makefile
	module/owftpd/src/include/Makefile
	module/owftpd/src/c/Makefile

	module/ownet/Makefile
	module/ownet/php/Makefile
	module/ownet/php/examples/ownet_example.php
	module/ownet/python/Makefile
	module/ownet/perl5/Makefile
	module/ownet/c/Makefile
	module/ownet/c/src/Makefile
	module/ownet/c/src/include/Makefile
	module/ownet/c/src/c/Makefile
	module/ownet/c/src/example/Makefile

	module/owshell/Makefile
	module/owshell/src/Makefile
	module/owshell/src/include/Makefile
	module/owshell/src/c/Makefile

	module/owcapi/Makefile
	module/owcapi/src/Makefile
	module/owcapi/src/include/Makefile
	module/owcapi/src/c/Makefile
	module/owcapi/src/example/Makefile
	module/owcapi/src/example++/Makefile

	module/owtap/Makefile

	module/owmon/Makefile

	module/swig/Makefile
	module/swig/perl5/Makefile
	module/swig/perl5/OW/Makefile.linux
	module/swig/perl5/OW/Makefile.osx
	module/swig/php/Makefile
	module/swig/php/example/load_php_OW.php
	module/swig/python/Makefile
	module/swig/python/ow/Makefile
	module/swig/python/setup.py

	module/owtcl/Makefile

	src/Makefile
	src/include/Makefile

	src/man/Makefile
	src/man/man1/Makefile
	src/man/man3/Makefile
	src/man/man5/Makefile
	src/man/mann/Makefile

	src/rpm/Makefile
	src/rpm/owfs.spec
	src/scripts/Makefile
	src/scripts/windows/Makefile
	src/scripts/windows/owfs.nsi
	src/scripts/usb/Makefile
	src/scripts/usb/cygwin/Makefile
	src/scripts/usb/windows/Makefile
	src/scripts/systemd/Makefile
	src/include/owfs_config.h
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

if test "${ENABLE_USB}" = "true"; then
	AC_MSG_RESULT([                      USB is enabled])
else
	AC_MSG_RESULT([                      USB is DISABLED])
fi
if test "${ENABLE_AVAHI}" = "true"; then
	AC_MSG_RESULT([                    AVAHI is enabled])
else
	AC_MSG_RESULT([                    AVAHI is DISABLED])
fi
if test "${ENABLE_I2C}" = "true"; then
	AC_MSG_RESULT([                      I2C is enabled])
else
	AC_MSG_RESULT([                      I2C is DISABLED])
fi
if test "${ENABLE_W1}" = "true"; then
	AC_MSG_RESULT([                       W1 is enabled])
else
	AC_MSG_RESULT([                       W1 is DISABLED])
fi
if test "${ENABLE_PARPORT}" = "true"; then
	AC_MSG_RESULT([    Parallel port DS1410E is enabled])
else
	AC_MSG_RESULT([    Parallel port DS1410E is DISABLED])
fi
if test "${ENABLE_FTDI}" = "true"; then
	AC_MSG_RESULT([           FTDI (LinkUSB) is enabled])
else
	AC_MSG_RESULT([           FTDI (LinkUSB) is DISABLED])
fi
if test "${ENABLE_ZERO}" = "true"; then
	AC_MSG_RESULT([         Zeroconf/Bonjour is enabled])
else
	AC_MSG_RESULT([         Zeroconf/Bonjour is DISABLED])
fi
if test "${ENABLE_DEBUG}" = "true"; then
	AC_MSG_RESULT([             Debug-output is enabled])
else
	AC_MSG_RESULT([             Debug-output is DISABLED])
fi
if test "${ENABLE_MUTEX_DEBUG}" = "true"; then
	AC_MSG_RESULT([               Mutexdebug is enabled])
else
	AC_MSG_RESULT([               Mutexdebug is DISABLED])
fi
if test "${ENABLE_PROFILING}" = "true"; then
	AC_MSG_RESULT([                Profiling is enabled])
else
	AC_MSG_RESULT([                Profiling is DISABLED])
fi
if test "${ENABLE_OWMALLOC}" = "true"; then
	AC_MSG_RESULT([Tracing memory allocation is enabled])
else
	AC_MSG_RESULT([Tracing memory allocation is DISABLED])
fi

AC_MSG_RESULT()
AC_MSG_RESULT([Module configuration:])

if test "${ENABLE_OWLIB}" = "true"; then
	AC_MSG_RESULT([                    owlib is enabled])
else
	AC_MSG_RESULT([                    owlib is DISABLED])
fi
if test "${ENABLE_OWSHELL}" = "true"; then
	AC_MSG_RESULT([                  owshell is enabled])
else
	AC_MSG_RESULT([                  owshell is DISABLED])
fi
if test "${ENABLE_OWFS}" = "true"; then
	AC_MSG_RESULT([                     owfs is enabled])
else
	AC_MSG_RESULT([                     owfs is DISABLED])
fi
if test "${ENABLE_OWHTTPD}" = "true"; then
	AC_MSG_RESULT([                  owhttpd is enabled])
else
	AC_MSG_RESULT([                  owhttpd is DISABLED])
fi
if test "${ENABLE_OWFTPD}" = "true"; then
	AC_MSG_RESULT([                   owftpd is enabled])
else
	AC_MSG_RESULT([                   owftpd is DISABLED])
fi
if test "${ENABLE_OWSERVER}" = "true"; then
	AC_MSG_RESULT([                 owserver is enabled])
else
	AC_MSG_RESULT([                 owserver is DISABLED])
fi
if test "${ENABLE_OWEXTERNAL}" = "true"; then
	AC_MSG_RESULT([               owexternal is enabled])
else
	AC_MSG_RESULT([               owexternal is DISABLED])
fi
if test "${ENABLE_OWNET}" = "true"; then
	AC_MSG_RESULT([                    ownet is enabled])
else
	AC_MSG_RESULT([                    ownet is DISABLED])
fi
if test "${ENABLE_OWNETLIB}" = "true"; then
	AC_MSG_RESULT([                 ownetlib is enabled])
else
	AC_MSG_RESULT([                 ownetlib is DISABLED])
fi
if test "${ENABLE_OWTAP}" = "true"; then
	AC_MSG_RESULT([                    owtap is enabled])
else
	AC_MSG_RESULT([                    owtap is DISABLED])
fi
if test "${ENABLE_OWMON}" = "true"; then
	AC_MSG_RESULT([                    owmon is enabled])
else
	AC_MSG_RESULT([                    owmon is DISABLED])
fi
if test "${ENABLE_OWCAPI}" = "true"; then
	AC_MSG_RESULT([                   owcapi is enabled])
else
	AC_MSG_RESULT([                   owcapi is DISABLED])
fi
if test "${ENABLE_SWIG}" = "true"; then
	AC_MSG_RESULT([                     swig is enabled])
else
	AC_MSG_RESULT([                     swig is DISABLED])
fi
if test "${ENABLE_OWPERL}" = "true" -a "${ENABLE_SWIG}" = "true"; then
	AC_MSG_RESULT([                   owperl is enabled])
else
	AC_MSG_RESULT([                   owperl is DISABLED])
fi
if test "${ENABLE_OWPHP}" = "true" -a "${ENABLE_SWIG}" = "true"; then
	AC_MSG_RESULT([                    owphp is enabled])
else
	AC_MSG_RESULT([                    owphp is DISABLED])
fi
if test "${ENABLE_OWPYTHON}" = "true" -a "${ENABLE_SWIG}" = "true"; then
	AC_MSG_RESULT([                 owpython is enabled])
else
	AC_MSG_RESULT([                 owpython is DISABLED])
fi
if test "${ENABLE_OWTCL}" = "true"; then
	AC_MSG_RESULT([                    owtcl is enabled])
else
	AC_MSG_RESULT([                    owtcl is DISABLED])
fi
AC_MSG_RESULT()
