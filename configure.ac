dnl Process this file with autoconf to produce a configure script.

AC_REVISION($Id$)

AC_INIT
AC_CONFIG_SRCDIR([module/owlib/src/include/ow.h])

VERSION_MAJOR="2"
VERSION_MINOR="6"
PATCHLEVEL="2"

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

AC_PROG_CXX
AC_PROG_CC
AC_PROG_CPP
AC_PROG_INSTALL
AC_LIBTOOL_WIN32_DLL
AC_PROG_LN_S
AC_PROG_MAKE_SET
AC_PROG_RANLIB
AC_PROG_AWK
AC_LIBTOOL_DLOPEN
AM_PROG_LIBTOOL
AC_SUBST(LIBTOOL_DEPS)


HAVE_DARWIN="false"
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
  *)
    AC_MSG_RESULT([Other host])
    ;;
esac
AC_SUBST(HAVE_DARWIN)
AM_CONDITIONAL(HAVE_DARWIN, test "${HAVE_DARWIN}" = "true")
AC_SUBST(HAVE_CYGWIN)
AM_CONDITIONAL(HAVE_CYGWIN, test "${HAVE_CYGWIN}" = "true")

if test "$cross_compiling" != yes; then
OW_CHECK_NESTED_FUNCTIONS
fi

OWFSROOT="`pwd`"
AC_SUBST(OWFSROOT)

AC_PATH_PROG(RPM,rpm,,$PATH)dnl
AC_PATH_PROG(RPMBUILD,rpmbuild,,$PATH)dnl
AC_PATH_PROG([SWIG],[swig],,$PATH)dnl

# ccmalloc macros from Armljn Hemel stratego-dev 14 jan 2003
AC_CHECK_PROG(CCMALLOC, ccmalloc, ccmalloc)
AC_SUBST(CCMALLLOC)
AC_ARG_ENABLE(ccmalloc, 
[  --enable-ccmalloc       ccmalloc memory checking],
[ AC_MSG_RESULT(["$enableval"])
 if test "$enableval" = yes; then
   CCLD="$CCMALLOC --no-wrapper $CC"
 else
   CCLD="$CC"
 fi],
   CCLD=$CC
)
AC_SUBST(CCLD)

AC_DEFUN([ACX_PTHREAD], [
AC_REQUIRE([AC_CANONICAL_HOST])
AC_LANG_SAVE
AC_LANG_C
acx_pthread_ok=no

# We used to check for pthread.h first, but this fails if pthread.h
# requires special compiler flags (e.g. on True64 or Sequent).
# It gets checked for in the link test anyway.

# First of all, check if the user has set any of the PTHREAD_LIBS,
# etcetera environment variables, and if threads linking works using
# them:
if test x"$PTHREAD_LIBS$PTHREAD_CFLAGS" != x; then
        save_CFLAGS="$CFLAGS"
        CFLAGS="$CFLAGS $PTHREAD_CFLAGS"
        save_LIBS="$LIBS"
        LIBS="$PTHREAD_LIBS $LIBS"
        AC_MSG_CHECKING([for pthread_join in LIBS=$PTHREAD_LIBS with CFLAGS=$PTHREAD_CFLAGS])
        AC_TRY_LINK_FUNC(pthread_join, acx_pthread_ok=yes)
        AC_MSG_RESULT($acx_pthread_ok)
        if test x"$acx_pthread_ok" = xno; then
                PTHREAD_LIBS=""
                PTHREAD_CFLAGS=""
        fi
        LIBS="$save_LIBS"
        CFLAGS="$save_CFLAGS"
fi

# We must check for the threads library under a number of different
# names; the ordering is very important because some systems
# (e.g. DEC) have both -lpthread and -lpthreads, where one of the
# libraries is broken (non-POSIX).

# Create a list of thread flags to try.  Items starting with a "-" are
# C compiler flags, and other items are library names, except for "none"
# which indicates that we try without any flags at all, and "pthread-config"
# which is a program returning the flags for the Pth emulation library.

acx_pthread_flags="pthreads none -Kthread -kthread lthread -pthread -pthreads -mthreads pthread --thread-safe -mt pthread-config"

# The ordering *is* (sometimes) important.  Some notes on the
# individual items follow:

# pthreads: AIX (must check this before -lpthread)
# none: in case threads are in libc; should be tried before -Kthread and
#       other compiler flags to prevent continual compiler warnings
# -Kthread: Sequent (threads in libc, but -Kthread needed for pthread.h)
# -kthread: FreeBSD kernel threads (preferred to -pthread since SMP-able)
# lthread: LinuxThreads port on FreeBSD (also preferred to -pthread)
# -pthread: Linux/gcc (kernel threads), BSD/gcc (userland threads)
# -pthreads: Solaris/gcc
# -mthreads: Mingw32/gcc, Lynx/gcc
# -mt: Sun Workshop C (may only link SunOS threads [-lthread], but it
#      doesn't hurt to check since this sometimes defines pthreads too;
#      also defines -D_REENTRANT)
# pthread: Linux, etcetera
# --thread-safe: KAI C++
# pthread-config: use pthread-config program (for GNU Pth library)

case "${host_cpu}-${host_os}" in
        *solaris*)

        # On Solaris (at least, for some versions), libc contains stubbed
        # (non-functional) versions of the pthreads routines, so link-based
        # tests will erroneously succeed.  (We need to link with -pthread or
        # -lpthread.)  (The stubs are missing pthread_cleanup_push, or rather
        # a function called by this macro, so we could check for that, but
        # who knows whether they'll stub that too in a future libc.)  So,
        # we'll just look for -pthreads and -lpthread first:

        acx_pthread_flags="-pthread -pthreads pthread -mt $acx_pthread_flags"
        ;;

# We always want dependencies to libpthread.so on linux since some old
# libc doesn't load libow when using perl.
	*linux*)
        acx_pthread_flags="-pthread $acx_pthread_flags"
        PTHREAD_LIBS="-lpthread"
        ;;
	*cygwin*)
#        acx_pthread_flags="-lpthread $acx_pthread_flags"
        PTHREAD_LIBS="-lpthread"
        ;;
esac

if test x"$acx_pthread_ok" = xno; then
for flag in $acx_pthread_flags; do

        case $flag in
                none)
                AC_MSG_CHECKING([whether pthreads work without any flags])
                ;;

                -*)
                AC_MSG_CHECKING([whether pthreads work with $flag])
                PTHREAD_CFLAGS="$flag"
                ;;

		pthread-config)
		AC_CHECK_PROG(acx_pthread_config, pthread-config, yes, no)
		if test x"$acx_pthread_config" = xno; then continue; fi
		PTHREAD_CFLAGS="`pthread-config --cflags`"
		PTHREAD_LIBS="`pthread-config --ldflags` `pthread-config --libs`"
		;;

                *)
                AC_MSG_CHECKING([for the pthreads library -l$flag])
                PTHREAD_LIBS="-l$flag"
                ;;
        esac

        save_LIBS="$LIBS"
        save_CFLAGS="$CFLAGS"
        LIBS="$PTHREAD_LIBS $LIBS"
        CFLAGS="$CFLAGS $PTHREAD_CFLAGS"

        # Check for various functions.  We must include pthread.h,
        # since some functions may be macros.  (On the Sequent, we
        # need a special flag -Kthread to make this header compile.)
        # We check for pthread_join because it is in -lpthread on IRIX
        # while pthread_create is in libc.  We check for pthread_attr_init
        # due to DEC craziness with -lpthreads.  We check for
        # pthread_cleanup_push because it is one of the few pthread
        # functions on Solaris that doesn't have a non-functional libc stub.
        # We try pthread_create on general principles.
        AC_TRY_LINK([#include <pthread.h>],
                    [pthread_t th; pthread_join(th, 0);
                     pthread_attr_init(0); pthread_cleanup_push(0, 0);
                     pthread_create(0,0,0,0); pthread_cleanup_pop(0); ],
                    [acx_pthread_ok=yes])

        LIBS="$save_LIBS"
        CFLAGS="$save_CFLAGS"

        AC_MSG_RESULT($acx_pthread_ok)
        if test "x$acx_pthread_ok" = xyes; then
                break;
        fi

        PTHREAD_LIBS=""
        PTHREAD_CFLAGS=""
done
fi

# Various other checks:
if test "x$acx_pthread_ok" = xyes; then
        save_LIBS="$LIBS"
        LIBS="$PTHREAD_LIBS $LIBS"
        save_CFLAGS="$CFLAGS"
        CFLAGS="$CFLAGS $PTHREAD_CFLAGS"

        # Detect AIX lossage: threads are created detached by default
        # and the JOINABLE attribute has a nonstandard name (UNDETACHED).
        AC_MSG_CHECKING([for joinable pthread attribute])
        AC_TRY_LINK([#include <pthread.h>],
                    [int attr=PTHREAD_CREATE_JOINABLE;],
                    ok=PTHREAD_CREATE_JOINABLE, ok=unknown)
        if test x"$ok" = xunknown; then
                AC_TRY_LINK([#include <pthread.h>],
                            [int attr=PTHREAD_CREATE_UNDETACHED;],
                            ok=PTHREAD_CREATE_UNDETACHED, ok=unknown)
        fi
        if test x"$ok" != xPTHREAD_CREATE_JOINABLE; then
                AC_DEFINE(PTHREAD_CREATE_JOINABLE, $ok,
                          [Define to the necessary symbol if this constant
                           uses a non-standard name on your system.])
        fi
        AC_MSG_RESULT(${ok})
        if test x"$ok" = xunknown; then
                AC_MSG_WARN([we do not know how to create joinable pthreads])
        fi

        AC_MSG_CHECKING([if more special flags are required for pthreads])
        flag=no
        case "${host_cpu}-${host_os}" in
                *-aix* | *-freebsd* | *-darwin*) flag="-D_THREAD_SAFE";;
                *solaris* | *-osf* | *-hpux*) flag="-D_REENTRANT";;
        esac
        AC_MSG_RESULT(${flag})
        if test "x$flag" != xno; then
                PTHREAD_CFLAGS="$flag $PTHREAD_CFLAGS"
        fi

        LIBS="$save_LIBS"
        CFLAGS="$save_CFLAGS"

        # More AIX lossage: must compile with cc_r
        AC_CHECK_PROG(PTHREAD_CC, cc_r, cc_r, ${CC})
else
        PTHREAD_CC="$CC"
fi

AC_SUBST(PTHREAD_LIBS)
AC_SUBST(PTHREAD_CFLAGS)
AC_SUBST(PTHREAD_CC)

# Finally, execute ACTION-IF-FOUND/ACTION-IF-NOT-FOUND:
if test x"$acx_pthread_ok" = xyes; then
        ifelse([$1],,AC_DEFINE(HAVE_PTHREAD,1,[Define if you have POSIX threads libraries and header files.]),[$1])
        :
else
        acx_pthread_ok=no
        $2
fi
AC_LANG_RESTORE
])dnl ACX_PTHREAD

# Checks for header files.
AC_HEADER_DIRENT
AC_HEADER_STDC
AC_CHECK_HEADERS([sys/mkdev.h sys/time.h sys/types.h feature_tests.h fcntl.h netinet/in.h stdlib.h string.h sys/file.h syslog.h termios.h unistd.h limits.h stdint.h features.h getopt.h resolv.h])
AC_CHECK_HEADERS([linux/limits.h linux/types.h bits/netdb.h bits/siginfo.h dlfcn.h])

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

# Test if TAI8570 barometer should be supported
ENABLE_TAI8570="true"
AC_MSG_CHECKING([if TAI8570 barometer support is enabled])
AC_ARG_ENABLE(tai8570,
[  --enable-tai8570        Enable tai8570 barometer support (default true)],
[
    AC_MSG_RESULT([$enableval])
    if ! test "$enableval" = "yes" ; then
        ENABLE_TAI8570="false"
    fi
],
[
    AC_MSG_RESULT([yes (default)])
])
AC_SUBST(ENABLE_TAI8570)
AM_CONDITIONAL(ENABLE_TAI8570, test "${ENABLE_TAI8570}" = "true")

# Test if thermocouples should be supported
ENABLE_THERMOCOUPLE="true"
AC_MSG_CHECKING([if thermocouple support is enabled])
AC_ARG_ENABLE(thermocouple,
[  --enable-thermocouple   Enable thermocouple support (default true)],
[
    AC_MSG_RESULT([$enableval])
    if ! test "$enableval" = "yes" ; then
        ENABLE_THERMOCOUPLE="false"
    fi
],
[
    AC_MSG_RESULT([yes (default)])
])
AC_SUBST(ENABLE_THERMOCOUPLE)
AM_CONDITIONAL(ENABLE_THERMOCOUPLE, test "${ENABLE_THERMOCOUPLE}" = "true")

# Check for multithreading
ENABLE_MT="true"
AC_MSG_CHECKING([if multithreading is enabled])
AC_ARG_ENABLE(mt,
[  --enable-mt             Enable multithreading support (default true)],
[
    AC_MSG_RESULT([$enableval])
    if ! test "$enableval" = "yes" ; then
        ENABLE_MT="false"
    fi
],
[
    AC_MSG_RESULT([yes (default)])
])

if test "${ENABLE_MT}" = "true" ; then
    ACX_PTHREAD
    if test "$acx_pthread_ok" = "yes"; then
	CFLAGS="$CFLAGS $PTHREAD_CFLAGS"
    else
	ENABLE_MT="false"
    fi
fi
AC_SUBST(ENABLE_MT)
AM_CONDITIONAL(ENABLE_MT, test "${ENABLE_MT}" = "true")


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


# Check for HA7Net
ENABLE_HA7="true"
AC_MSG_CHECKING([if HA7Net is enabled])
AC_ARG_ENABLE(ha7,
[  --enable-ha7            Enable ha7 support (default true)],
[
    AC_MSG_RESULT([$enableval])
    if ! test "$enableval" = "yes" ; then
        ENABLE_HA7="false"
    fi
],
[
    AC_MSG_RESULT([yes (default)])
])
AC_SUBST(ENABLE_HA7)
AM_CONDITIONAL(ENABLE_HA7, test "${ENABLE_HA7}" = "true")


# Check if the modules are enabled

#Check owhttpd
AC_MSG_CHECKING(if owhttpd is enabled)
ENABLE_OWHTTPD="true"
AC_ARG_ENABLE(owhttpd,
[  --enable-owhttpd        Enable owhttpd module (default true)],
[
    AC_MSG_RESULT([$enableval])
    if test ! "$enableval" = "yes" ; then
        ENABLE_OWHTTPD="false"
    fi
],
[
    AC_MSG_RESULT([yes (default)])
])
AC_SUBST(ENABLE_OWHTTPD)
AM_CONDITIONAL(ENABLE_OWHTTPD, test "${ENABLE_OWHTTPD}" = "true")

#Check owftpd
AC_MSG_CHECKING(if owftpd is enabled)
ENABLE_OWFTPD="true"
AC_ARG_ENABLE(owftpd,
[  --enable-owftpd         Enable owftpd module (default false)],
[
    AC_MSG_RESULT([$enableval])
    if test ! "$enableval" = "yes" ; then
        ENABLE_OWFTPD="false"
    fi
],
[
    AC_MSG_RESULT([yes (default)])
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
    fi
],
[
    AC_MSG_RESULT([yes (default)])
])
AC_SUBST(ENABLE_OWSERVER)
AM_CONDITIONAL(ENABLE_OWSERVER, test "${ENABLE_OWSERVER}" = "true")

#Check owcapi
AC_MSG_CHECKING(if owcapi is enabled)
ENABLE_OWCAPI="true"
AC_ARG_ENABLE(owcapi,
[  --enable-owcapi         Enable owcapi module (default true)],
[
    AC_MSG_RESULT([$enableval])
    if test ! "$enableval" = "yes" ; then
        ENABLE_OWCAPI="false"
    fi
],
[
    AC_MSG_RESULT([yes (default)])
])
AC_SUBST(ENABLE_OWCAPI)
AM_CONDITIONAL(ENABLE_OWCAPI, test "${ENABLE_OWCAPI}" = "true")


# Make sure tclConfig.sh is found under /usr/lib64/
# Should perhaps support cross-compiling to other cpu-type too?
LIBPOSTFIX=
case "${host_os}" in
  *linux* )
    case "${host_cpu}" in
      powerpc64 | s390x | x86_64 )
        LIBPOSTFIX="64"
        ;;
    esac
    ;;
esac
AC_SUBST(LIBPOSTFIX)


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
if test -z "$SWIG" ; then
    ENABLE_OWPERL="false"
else
    ENABLE_OWPERL="true"
fi
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
    else
        AC_MSG_RESULT([yes (default)])
    fi
])

if test "${ENABLE_OWPERL}" = "true" ; then
    m4_include(module/swig/perl5/perl5.m4)
    AC_MSG_RESULT(Looking for location of Perl executable)
    SC_PATH_PERL5
    AC_SUBST(PERL)
    AC_SUBST(PERL5EXT)
    AC_SUBST(PERL5DYNAMICLINKING)
    AC_SUBST(PERL5LIB)
    AC_SUBST(PERL5CCFLAGS)
    if test -z "${PERL}" ; then
        AC_MSG_WARN([Cannot find perl binary.])
        AC_MSG_WARN([OWPERL is disabled because perl binary is not found])
        ENABLE_OWPERL="false"
    else
	if test -z "${PERL5LIB}" ; then
	    AC_MSG_WARN([Cannot find perl library. Install perl-devel package.])
            AC_MSG_WARN([OWPERL is disabled because perl library is not found])
            ENABLE_OWPERL="false"
	fi
    fi
fi
AC_SUBST(ENABLE_OWPERL)
AM_CONDITIONAL(ENABLE_OWPERL, test "${ENABLE_OWPERL}" = "true")

#Check owphp
AC_MSG_CHECKING(if owphp is enabled)
if test -z "$SWIG" ; then
    ENABLE_OWPHP="false"
else
    ENABLE_OWPHP="true"
fi
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
    else
        AC_MSG_RESULT([yes (default)])
    fi
])

if test "${ENABLE_OWPHP}" = "true" ; then
    m4_include(module/swig/php/php.m4)
    AC_MSG_RESULT(Looking for location of Php executable)
    SC_PATH_PHP
    AC_SUBST(PHP)
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
AC_SUBST(ENABLE_OWPHP)
AM_CONDITIONAL(ENABLE_OWPHP, test "${ENABLE_OWPHP}" = "true")


#Check owpython
AC_MSG_CHECKING(if owpython is enabled)
if test -z "$SWIG" ; then
    ENABLE_OWPYTHON="false"
else
    ENABLE_OWPYTHON="true"
fi
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
    else
        AC_MSG_RESULT([yes (default)])
    fi
])

if test "${ENABLE_OWPYTHON}" = "true" ; then
    m4_include(module/swig/python/python.m4)
    AC_MSG_RESULT(Looking for location of Python executable)
    SC_PATH_PYTHON
    AC_SUBST(PYTHON)
    AC_SUBST(PYINCLUDE)
    AC_SUBST(PYLIB)
    AC_SUBST(PYLINK)
    AC_SUBST(PYTHONDYNAMICLINKING)
    if test -z "${PYTHON}" ; then
        AC_MSG_WARN([Cannot find python binary. Install python package.])
        AC_MSG_WARN([OWPYTHON is disabled because python binary is not found])
        ENABLE_OWPYTHON="false"
    else
        if test -z "${PYLIB}" ; then
            AC_MSG_WARN([Cannot find python library. Install python-devel package.])
            AC_MSG_WARN([OWPYTHON is disabled because python library is not found])
            ENABLE_OWPYTHON="false"
        else
            if test -z "${PYINCLUDE}" ; then
                AC_MSG_WARN([Cannot find python include-file. Install python-devel package.])
                AC_MSG_WARN([OWPYTHON is disabled because python include-file is not found])
                ENABLE_OWPYTHON="false"
             fi
        fi
    fi
fi
AC_SUBST(ENABLE_OWPYTHON)
AM_CONDITIONAL(ENABLE_OWPYTHON, test "${ENABLE_OWPYTHON}" = "true")

#Check owtcl
AC_MSG_CHECKING(if owtcl is enabled)
if test -z "$SWIG" ; then
    ENABLE_OWTCL="false"
else
    ENABLE_OWTCL="true"
fi
AC_ARG_ENABLE(owtcl,
[  --enable-owtcl          Enable owtcl module (default true)],
[
    AC_MSG_RESULT([$enableval])
    if test "$enableval" = "no" ; then
        ENABLE_OWTCL="false"
    fi
    if test "$enableval" = "yes" ; then
        ENABLE_OWTCL="true"
        if test -z "$SWIG"  ; then
            AC_MSG_WARN([Cannot find swig program. Look for it at http://www.swig.org or use CPAN])
            AC_MSG_WARN([OWTCL is disabled because swig is not found])
            ENABLE_OWTCL="false"
        fi
    fi
],
[
    if test -z "$SWIG" ; then
        AC_MSG_RESULT([no (swig disabled)])
    else
        AC_MSG_RESULT([yes (default)])
    fi
])

if test "${ENABLE_OWTCL}" = "true" ; then
    m4_include(module/swig/tcl/tcl.m4)
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
    fi
fi
AC_SUBST(ENABLE_OWTCL)
AM_CONDITIONAL(ENABLE_OWTCL, test "${ENABLE_OWTCL}" = "true")

AC_MSG_CHECKING([if cflag _XOPEN_SOURCE is required])
case "${host_cpu}-${host_os}" in
	*-aix* | *solaris* | *-osf* | *-hpux*)
		CFLAGS="$CFLAGS -D_XOPEN_SOURCE=500"
		AC_MSG_RESULT([-D_XOPEN_SOURCE=500])
	;;
	*-freebsd* | *)
		AC_MSG_RESULT([no])
	;;
	*-darwin*)
		AC_MSG_RESULT([no])
	;;
esac

# Check if extra libs are needed. Should perhaps test if they are available too
# instead of hardcoding the libraries.
AC_MSG_CHECKING([if extra libs are required])
case "${host_cpu}-${host_os}" in
	*-darwin*)
		# This will actually be duplicates if libusb is used.
		OSLIBS="-Wl,-framework -Wl,IOKit -Wl,-framework -Wl,CoreFoundation"
		LD_EXTRALIBS=""
		AC_MSG_RESULT([${OSLIBS}])
	;;
	*-solaris*)
		OSLIBS=""
		LD_EXTRALIBS=" -lsocket -lnsl "
		LIBS="$LIBS -lsocket -lnsl"
		AC_MSG_RESULT([${LD_EXTRALIBS}])
	;;
	*)
		OSLIBS=""
		LD_EXTRALIBS="-Wl,--rpath -Wl,$libdir"
		AC_MSG_RESULT([${LD_EXTRALIBS}])
	;;
esac
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
    LD_EXTRALIBS="$save_LD_EXTRALIBS -Wl,--rpath -Wl,${fuse_lib_path}"
    CPPFLAGS="$save_CPPFLAGS -D_FILE_OFFSET_BITS=64 $FUSE_FLAGS $FUSE_INCLUDES"
    LDFLAGS="$save_LDFLAGS $FUSE_LIBS"

    AC_CHECK_HEADER(fuse.h,,[
	AC_MSG_WARN([
	Can't find fuse.h - Add the search path with --with-fuseinclude])

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
	AC_CHECK_LIB(fuse,fuse_main, [FUSE_LIBS="$FUSE_LIBS -lfuse"],[
	    AC_MSG_WARN([
	Can't find libfuse.a - add the search path with --with-fuselib])
		AC_MSG_WARN([Running ldconfig or adding "/usr/local/lib" to /etc/ld.so.conf might also solve the problem, otherwise re-install fuse.])
	    	if test "${ENABLE_OWFS}" = "auto"; then
			AC_MSG_WARN([OWFS is disabled because libfuse.a is not found.])
			ENABLE_OWFS="false"
		else
			AC_MSG_ERROR([Can't enable OWFS])
		fi
	    ],)
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

# Check if the cache is enabled.
AC_MSG_CHECKING(if caching is enabled)
ENABLE_CACHE="true"
AC_ARG_ENABLE(cache,
[  --enable-cache          Enable caching (default true)],
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
AM_CONDITIONAL(ENABLE_CACHE, test "${ENABLE_CACHE}" = "true")

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

if test "$cross_compiling" != yes; then
AC_CHECK_PROGS(LIBUSB_CONFIG, libusb-config)
else
LIBUSB_CONFIG=""
fi
AC_SUBST(LIBUSB_CONFIG)

AC_ARG_WITH(libusb-config,
            [  --with-libusb-config=PATH Specify full path to libusb-config])
dnl Check if user passed a specific php-config program.
if test "X$with_libusb_config" != "X" ; then
    LIBUSB_CONFIG=$with_libusb_config
fi

if test "$cross_compiling" != yes; then
 if test "X$LIBUSB_CONFIG" == "X" ; then
   dirs="/usr/bin /usr/local/bin /opt/local/bin"
   for i in $dirs; do
       echo $i/libusb-config
       if test -x $i/libusb-config; then
	  AC_MSG_RESULT($i/libusb-config is found)
	  LIBUSB_CONFIG="$i/libusb-config"
	  break;
       fi
   done
 fi
fi

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

  # Include libusb if the usb is enabled
  if test "X$LIBUSB_CONFIG" != "X" ; then

    LIBUSB_CFLAGS=`$LIBUSB_CONFIG --cflags`
    LIBUSB_LIBS=`$LIBUSB_CONFIG --libs`

    save_CPPFLAGS="$CPPFLAGS"
    save_LDFLAGS="$LDFLAGS"
    CPPFLAGS="$save_CPPFLAGS $LIBUSB_CFLAGS"
    LDFLAGS="$save_LDFLAGS $LIBUSB_LIBS"

    AC_MSG_CHECKING([if libusb compiles with includes+lib ])
    AC_COMPILE_IFELSE([
	AC_LANG_PROGRAM([[#include <usb.h>]],
	[[
	    usb_open(NULL);
	]])],
	[AC_MSG_RESULT([ok])],
	[AC_MSG_RESULT([compilation error])
	LIBUSB_CFLAGS=""
	LIBUSB_LDFLAGS=""
	LIBUSB_CONFIG=""
    ])

    CPPFLAGS="$save_CPPFLAGS"
    LDFLAGS="$save_LDFLAGS"

  #LIBUSB_CONFIG
  fi

  # last test to see if libusb is found after AC_CHECK_LIB
  if test "X${LIBUSB_CONFIG}" == "X"; then

       AC_MSG_CHECKING([if libusb is could be compiled])
       AC_CHECK_LIB(usb,usb_open,[
	    AC_MSG_RESULT([ok])
	    LIBUSB_LIBS="-lusb"],[
	    AC_MSG_WARN([Can't find libusb])
	        if test "${ENABLE_USB}" = "true" ; then
		        AC_MSG_ERROR([libusb must be installed to use a USB adapter])
		else
	                AC_MSG_WARN([libusb not found, usb will be disabled])
			ENABLE_USB=false
	        fi
        ],)
  fi

  if test "${ENABLE_USB}" != "false" ; then
      ENABLE_USB=true
  fi

# ENABLE_USB
fi


AC_SUBST(LIBUSB_CFLAGS)
AC_SUBST(LIBUSB_LIBS)
AC_SUBST(ENABLE_USB)
AM_CONDITIONAL(ENABLE_USB, test "${ENABLE_USB}" = "true")

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

#Check for profiling
AC_MSG_CHECKING(if profiling is enabled)
ENABLE_PROFILING="false"
AC_ARG_ENABLE(profiling,
[  --enable-profiling      Enable profiling (default false)],
[
AC_MSG_RESULT([$enableval])
    if test "$enableval" = "yes" ; then
        ENABLE_PROFILING=true
    fi
],
[
    AC_MSG_RESULT([no (default)])
])
AC_SUBST(ENABLE_PROFILING)
AM_CONDITIONAL(ENABLE_PROFILING, test "${ENABLE_PROFILING}" = "true")


# owftpd section
AC_SUBST(HAVE_NEW_SS_FAMILY)
AC_EGREP_HEADER(__ss_family,
    [sys/socket.h],
    [HAVE_NEW_SS_FAMILY=false],
    [HAVE_NEW_SS_FAMILY=true] )
AC_MSG_WARN([SS is ${HAVE_NEW_SS_FAMILY}])
AM_CONDITIONAL(HAVE_NEW_SS_FAMILY, test "${HAVE_NEW_SS_FAMILY}" = "true")
# end owftpd section



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

if test "${ENABLE_MT}" = "true" ; then
   OW_MT=1
else
   OW_MT=0
fi
AC_SUBST(OW_MT)

if test "${ENABLE_I2C}" = "true" ; then
   OW_I2C=1
else
   OW_I2C=0
fi
AC_SUBST(OW_I2C)

if test "${ENABLE_HA7}" = "true" ; then
   OW_HA7=1
else
   OW_HA7=0
fi
AC_SUBST(OW_HA7)

if test "${ENABLE_CACHE}" = "true" ; then
   OW_CACHE=1
else
   OW_CACHE=0
fi
AC_SUBST(OW_CACHE)

if test "${ENABLE_USB}" = "true" ; then
   OW_USB=1
else
   OW_USB=0
fi
AC_SUBST(OW_USB)

if test "${ENABLE_ZERO}" = "true" ; then
   OW_ZERO=1
else
   OW_ZERO=0
fi
AC_SUBST(OW_ZERO)

if test "${ENABLE_PROFILING}" = "true" ; then
   PROFILING_FLAG=-pg
fi
AC_SUBST(PROFILING_FLAG)

if test "${ENABLE_PARPORT}" = "true" ; then
   OW_PARPORT=1
else
   OW_PARPORT=0
fi
AC_SUBST(OW_PARPORT)

if test "${ENABLE_DEBUG}" = "true" ; then
   OW_DEBUG=1
else
   OW_DEBUG=0
fi
AC_SUBST(OW_DEBUG)

if test "${ENABLE_TAI8570}" = "true" ; then
   OW_TAI8570=1
else
   OW_TAI8570=0
fi
AC_SUBST(OW_TAI8570)

if test "${ENABLE_THERMOCOUPLE}" = "true" ; then
   OW_THERMOCOUPLE=1
else
   OW_THERMOCOUPLE=0
fi
AC_SUBST(OW_THERMOCOUPLE)

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
AC_FUNC_MKTIME
#AC_FUNC_REALLOC
AC_FUNC_SELECT_ARGTYPES
AC_FUNC_STRFTIME
AC_FUNC_STRTOD
AC_TYPE_SIGNAL
AC_CHECK_FUNCS([accept daemon getaddrinfo freeaddrinfo gethostbyname2_r gethostbyaddr_r gethostbyname_r getservbyname_r getopt getopt_long gettimeofday inet_ntop inet_pton memchr memset select socket strcasecmp strchr strdup strncasecmp strtol strtoul tsearch tfind tdelete tdestroy vasprintf usb_interrupt_read strsep vsprintf vsnprintf])

if test "${ENABLE_ZERO}" = "true" ; then
   AC_SEARCH_LIBS(dlopen, dl, AC_DEFINE(HAVE_DLOPEN, 1, [Define if you have dlopen]))
fi

AC_SEARCH_LIBS(nanosleep, rt posix4, AC_DEFINE(HAVE_NANOSLEEP, 1, [Define if you have nanosleep]))

#AC_SUBST(MODULE_SUBDIRS)
#AC_SUBST(SWIG_SUBDIRS)

# Process config.h.in
AH_TOP([
#ifndef OWCONFIG_H
#define OWCONFIG_H
])
AH_BOTTOM([#endif])
AM_CONFIG_HEADER(src/include/config.h)

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
    module/owshell/Makefile
    module/owshell/src/Makefile
    module/owshell/src/include/Makefile
    module/owshell/src/c/Makefile
    module/owcapi/Makefile
    module/owcapi/src/Makefile
    module/owcapi/src/include/Makefile
    module/owcapi/src/c/Makefile
    module/owcapi/src/example/Makefile
    module/swig/Makefile
    module/swig/perl5/Makefile
    module/swig/perl5/Makefile.PL
    module/swig/php/Makefile
    module/swig/php/example/load_php_OW.php
    module/swig/python/Makefile
    module/swig/python/ow/Makefile
    module/swig/python/setup.py
    module/swig/tcl/Makefile
    src/Makefile
    src/include/Makefile
    src/man/Makefile
    src/rpm/Makefile
    src/rpm/owfs.spec
    src/scripts/Makefile
    src/scripts/windows/Makefile
    src/scripts/windows/owfs.nsi
    src/scripts/usb/Makefile
    src/scripts/usb/cygwin/Makefile
    src/scripts/usb/windows/Makefile
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

if test "${ENABLE_CACHE}" = "true"; then
    AC_MSG_RESULT([                  Caching is enabled])
else
    AC_MSG_RESULT([                  Caching is DISABLED])
fi
if test "${ENABLE_USB}" = "true"; then
    AC_MSG_RESULT([                      USB is enabled])
else
    AC_MSG_RESULT([                      USB is DISABLED])
fi
if test "${ENABLE_I2C}" = "true"; then
    AC_MSG_RESULT([                      I2C is enabled])
else
    AC_MSG_RESULT([                      I2C is DISABLED])
fi
if test "${ENABLE_HA7}" = "true"; then
    AC_MSG_RESULT([                   HA7Net is enabled])
else
    AC_MSG_RESULT([                   HA7Net is DISABLED])
fi
if test "${ENABLE_MT}" = "true"; then
    AC_MSG_RESULT([           Multithreading is enabled])
else
    AC_MSG_RESULT([           Multithreading is DISABLED])
fi
if test "${ENABLE_PARPORT}" = "true"; then
    AC_MSG_RESULT([    Parallel port DS1410E is enabled])
else
    AC_MSG_RESULT([    Parallel port DS1410E is DISABLED])
fi
if test "${ENABLE_TAI8570}" = "true"; then
    AC_MSG_RESULT([        TAI8570 barometer is enabled])
else
    AC_MSG_RESULT([        TAI8570 barometer is DISABLED])
fi
if test "${ENABLE_THERMOCOUPLE}" = "true"; then
    AC_MSG_RESULT([             Thermocouple is enabled])
else
    AC_MSG_RESULT([             Thermocouple is DISABLED])
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
if test "${ENABLE_PROFILING}" = "true"; then
    AC_MSG_RESULT([                Profiling is enabled])
else
    AC_MSG_RESULT([                Profiling is DISABLED])
fi

AC_MSG_RESULT()
AC_MSG_RESULT([Module configuration:])

if test "${ENABLE_OWFS}" = "true"; then
    AC_MSG_RESULT([        owfs is enabled])
else
    AC_MSG_RESULT([        owfs is DISABLED])
fi
if test "${ENABLE_OWHTTPD}" = "true"; then
    AC_MSG_RESULT([     owhttpd is enabled])
else
    AC_MSG_RESULT([     owhttpd is DISABLED])
fi
if test "${ENABLE_OWFTPD}" = "true"; then
    AC_MSG_RESULT([      owftpd is enabled])
else
    AC_MSG_RESULT([      owftpd is DISABLED])
fi
if test "${ENABLE_OWSERVER}" = "true"; then
    AC_MSG_RESULT([    owserver is enabled])
else
    AC_MSG_RESULT([    owserver is DISABLED])
fi
if test "${ENABLE_OWCAPI}" = "true"; then
    AC_MSG_RESULT([      owcapi is enabled])
else
    AC_MSG_RESULT([      owcapi is DISABLED])
fi
if test "${ENABLE_SWIG}" = "true"; then
    AC_MSG_RESULT([        swig is enabled])
else
    AC_MSG_RESULT([        swig is DISABLED])
fi
if test "${ENABLE_OWPERL}" = "true"; then
    AC_MSG_RESULT([      owperl is enabled])
else
    AC_MSG_RESULT([      owperl is DISABLED])
fi
if test "${ENABLE_OWPHP}" = "true"; then
    AC_MSG_RESULT([       owphp is enabled])
else
    AC_MSG_RESULT([       owphp is DISABLED])
fi
if test "${ENABLE_OWPYTHON}" = "true"; then
    AC_MSG_RESULT([    owpython is enabled])
else
    AC_MSG_RESULT([    owpython is DISABLED])
fi
if test "${ENABLE_OWTCL}" = "true"; then
    AC_MSG_RESULT([       owtcl is enabled])
else
    AC_MSG_RESULT([       owtcl is DISABLED])
fi
AC_MSG_RESULT()
