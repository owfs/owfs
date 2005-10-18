dnl Process this file with autoconf to produce a configure script.

AC_REVISION($Id$)

AC_INIT
AC_CONFIG_SRCDIR([module/owlib/src/include/ow.h])

VERSION_MAJOR="2"
VERSION_MINOR="2"
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
AC_PATH_PROG([SWIG],[swig],,$PATH)dnl
AC_PATH_PROG([PERL],[perl],,$PATH)dnl
AC_PATH_PROG([PHP_CONFIG],[php-config],,$PATH)dnl
AC_PATH_PROG([PYTHON],[python],,$PATH)dnl

# ccmalloc macros from Armljn Hemel stratego-dev 14 jan 2003
AC_CHECK_PROG(CCMALLOC, ccmalloc, ccmalloc)
AC_SUBST(CCMALLLOC)
AC_ARG_ENABLE(ccmalloc, 
[--enable-ccmalloc        ccmalloc memory checking],
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


# Check for multithreading
ENABLE_MT=true
AC_MSG_CHECKING([if multithreading is enabled])
AC_ARG_ENABLE(mt,
[  --enable-mt           Enable multithreading support (default true)],
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
    ACX_PTHREAD
    if test "$acx_pthread_ok" = "yes"; then
	CFLAGS="$CFLAGS $PTHREAD_CFLAGS"
    else
	ENABLE_MT=false
	AC_SUBST(ENABLE_MT)
    fi
fi
AM_CONDITIONAL(ENABLE_MT, test "${ENABLE_MT}" = "true")




# Check if the modules are enabled

#Check owfs
AC_MSG_CHECKING([if owfs is enabled])
ENABLE_OWFS="auto"
AC_ARG_ENABLE(owfs,
[  --enable-owfs         Enable owfs module (default auto)],
[
    AC_MSG_RESULT([$enableval])
    if test "$enableval" != "yes" ; then
        ENABLE_OWFS="false"
    else
        ENABLE_OWFS="true"
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

#Check owserver
AC_MSG_CHECKING(if owserver is enabled)
ENABLE_OWSERVER="true"
AC_ARG_ENABLE(owserver,
[  --enable-owserver     Enable owserver module (default true)],
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
ENABLE_OWCAPI="false"
AC_ARG_ENABLE(owcapi,
[  --enable-owcapi       Enable owcapi module (default false)],
[
    AC_MSG_RESULT([$enableval])
    if test "$enableval" = "yes" ; then
        ENABLE_OWCAPI=true
    fi
],
[
    AC_MSG_RESULT([false (default)])
])
AC_SUBST(ENABLE_OWCAPI)

#Check swig
AC_MSG_CHECKING(if swig is enabled)
if test -z "$SWIG"  ; then
     ENABLE_SWIG="false"
else
     ENABLE_SWIG="true"
fi
AC_ARG_ENABLE(swig,
[  --enable-swig         Enable swig (default true)],
[
    if test ! "$enableval" = "yes"; then
        ENABLE_SWIG="false"
        SWIG=""
	AC_MSG_RESULT([no])
    else
	AC_MSG_RESULT([yes (default)])
    fi
],
[
    AC_MSG_RESULT([yes (default)])
])
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
[  --enable-owperl       Enable owperl module (default true)],
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
        if test -z "$PERL"  ; then
            AC_MSG_WARN([Cannot find perl program. Look for it at http://www.perl.org])
            AC_MSG_WARN([OWPERL is disabled because perl is not found])
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
AC_SUBST(ENABLE_OWPERL)
AM_CONDITIONAL(ENABLE_OWPERL, test "${ENABLE_OWPERL}" = "true")

#Check owphp
AC_MSG_CHECKING(if owphp is enabled)
if test -z "$SWIG" ; then
    ENABLE_OWPHP="false"
else
    ENABLE_OWPHP="false"
fi
AC_ARG_ENABLE(owphp,
[  --enable-owphp        Enable owphp module (default false)],
[
    AC_MSG_RESULT([$enableval])
    if test "$enableval" = "yes" ; then
        ENABLE_OWPHP="true"
    fi
    if test -z "$SWIG"  ; then
        AC_MSG_WARN([Cannot find swig program. Look for it at http://www.swig.org or use CPAN])
        AC_MSG_WARN([OWPHP is disabled because swig is not found])
        ENABLE_OWPHP="false"
    fi
    if test -z "$PHP_CONFIG"  ; then
        AC_MSG_WARN([Cannot find php-config program. Look for it at http://www.php.net])
        AC_MSG_WARN([OWPHP is disabled because php-config is not found])
        ENABLE_OWPHP="false"
    fi
],
[
    if test -z "$SWIG" ; then
        AC_MSG_RESULT([no (swig disabled)])
    else
        AC_MSG_RESULT([no (default)])
    fi
])
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
[  --enable-owpython     Enable owpython module (default true)],
[
    AC_MSG_RESULT([$enableval])
    if test ! "$enableval" = "yes" ; then
        ENABLE_OWPYTHON="false"
    else
        if test -z "$SWIG"  ; then
            AC_MSG_WARN([Cannot find swig program. Look for it at http://www.swig.org or use CPAN])
            AC_MSG_WARN([OWPYTHON is disabled because swig is not found])
            ENABLE_OWPYTHON="false"
        fi
        if test -z "$PYTHON"  ; then
            AC_MSG_WARN([Cannot find python program. Look for it at http://www.python.org])
            AC_MSG_WARN([OWPYTHON is disabled because python is not found])
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
AC_SUBST(ENABLE_OWPYTHON)
AM_CONDITIONAL(ENABLE_OWPYTHON, test "${ENABLE_OWPYTHON}" = "true")

#Check owtcl
AC_MSG_CHECKING(if owtcl is enabled)
if test -z "$SWIG" ; then
    ENABLE_OWTCL="false"
else
    ENABLE_OWTCL="false"
fi
AC_ARG_ENABLE(owtcl,
[  --enable-owtcl        Enable owtcl module (default false)],
[
    AC_MSG_RESULT([$enableval])
    if test "$enableval" = "yes" ; then
        ENABLE_OWTCL="true"
    fi
],
[
    if test -z "$SWIG" ; then
        AC_MSG_RESULT([no (swig disabled)])
    else
        AC_MSG_RESULT([no (default)])
    fi
])


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

AC_SUBST(ENABLE_OWTCL)
AM_CONDITIONAL(ENABLE_OWTCL, test "${ENABLE_OWTCL}" = "true")
if test "${ENABLE_OWTCL}" = "true" ; then
    m4_include(module/swig/tcl/tcl.m4)
    AC_MSG_RESULT(Looking for tclConfig.sh)
    SC_PATH_TCLCONFIG
    SC_LOAD_TCLCONFIG
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

#Check ownfsd
AC_MSG_CHECKING(if ownfsd enabled)
ENABLE_OWNFSD="false"
AC_ARG_ENABLE(ownfsd,
[  --enable-ownfsd       Enable ownfsd module (default false)],
[
    AC_MSG_RESULT([$enableval])
    if test "$enableval" = "yes" ; then
        ENABLE_OWNFSD="true"
    fi
],
[
    AC_MSG_RESULT([no (default)])
])
AC_SUBST(ENABLE_OWNFSD)
AM_CONDITIONAL(ENABLE_OWNFSD, test "${ENABLE_OWNFSD}" = "true")
if test "${ENABLE_OWNFSD}" = "true" ; then
        AC_SEARCH_LIBS(xdr_int, nsl)
fi


#disable FUSE version test (This option will be removed later, but will make it possible
#  to compile it for fuse-1.4 and fuse-2.1 for example)
AC_MSG_CHECKING(if FUSE version is checked)
ENABLE_FUSETEST="true"
AC_ARG_ENABLE(fusetest,
[  --enable-fusetest     Enable FUSE version test (default true)],
[
    AC_MSG_RESULT([$enableval])
    if test "$enableval" = "no" ; then
        ENABLE_FUSETEST="false"
    fi
],
[
    AC_MSG_RESULT([yes (default)])
])
AC_SUBST(ENABLE_FUSETEST)
AM_CONDITIONAL(ENABLE_FUSETEST, test "${ENABLE_FUSETEST}" = "true")


# We need fuse only if OWFS is enabled
if test "${ENABLE_OWFS}" != "false" ; then
  if test "${ENABLE_FUSETEST}" == "false" ; then
    FUSE_FLAGS=""
    AC_CHECK_LIB(fuse,fuse_mount, [
	FUSE_LIBS=-lfuse
    ], [
    if test "${ENABLE_OWFS}" = "true"; then
        AC_MSG_ERROR([You have to install libfuse first - download it from http://fuse.sourceforge.net/])
    else
        AC_MSG_WARN([[OWFS is disabled because libfuse is not installed.]])
            ENABLE_OWFS="false"
    fi
    ])
    if test "${ENABLE_OWFS}" != "false"; then
        ENABLE_OWFS="true"
    fi
  else
    FUSE_FLAGS="-DFUSE_USE_VERSION=22"
    save_CPPFLAGS="$CPPFLAGS"
    CPPFLAGS="$CPPFLAGS -D_FILE_OFFSET_BITS=64 $FUSE_FLAGS"

    AC_CHECK_HEADER(fuse.h,,
         [AC_MSG_ERROR([
		 Can't find fuse.h - add the search path to CPPFLAGS and
		 rerun configure, eg:
		 export CPPFLAGS=-I/usr/local/include ])])
    AC_CHECK_LIB(fuse,fuse_main, [FUSE_LIBS="-lfuse"],
         [AC_MSG_ERROR([
		 Can't find libfuse.a - add the search path to LDFLAGS
		 and rerun configure, eg:
		 export LDFLAGS=-L/usr/local/lib ])],)

    # check for a supported FUSE_MAJOR_VERSION.
    AC_MSG_CHECKING([For supported FUSE API version])
	AC_COMPILE_IFELSE([
	    AC_LANG_PROGRAM([[#include "fuse.h"]],
	    [[
	    #ifndef FUSE_MAJOR_VERSION
		#error "FUSE_MAJOR_VERSION not defined"
	    #endif
	    #if (FUSE_MAJOR_VERSION == 2) && (FUSE_MINOR_VERSION >= 2)
		/* Fuse > 2.2 is ok */
	    #else
		#error "Fuse < 2.2"
	    #endif
	    ]])],
	    [AC_MSG_RESULT([yes])],
	    [
	        if test "${ENABLE_OWFS}" = "true"; then
		   AC_MSG_ERROR([You have to install fuse-2.2 or later first - download it from http://fuse.sourceforge.net/])
		else
		   AC_MSG_WARN([OWFS is disabled because your installed libfuse is too old.])
		   ENABLE_OWFS="false"
		fi
		AC_MSG_RESULT([no])
	    ]
	)
    CPPFLAGS="$save_CPPFLAGS"
  fi
  if test "${ENABLE_OWFS}" != "false"; then
      ENABLE_OWFS="true"
  fi
else
    ENABLE_OWFS="false"
fi
AC_SUBST(FUSE_LIBS)
AC_SUBST(FUSE_FLAGS)

AM_CONDITIONAL(ENABLE_OWFS, test "${ENABLE_OWFS}" = "true")

# Check if the cache is enabled.
AC_MSG_CHECKING(if caching is enabled)
ENABLE_CACHE="true"
AC_ARG_ENABLE(cache,
[  --enable-cache        Enable caching (default true)],
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


#Check parallel port
AC_MSG_CHECKING(if parport DS1410E is enabled)
ENABLE_PARPORT="false"
AC_ARG_ENABLE(parport,
[  --enable-parport      Enable parport (DS1410E) module (default false)],
[
AC_MSG_RESULT([$enableval])
    if test "$enableval" = "yes" ; then
        ENABLE_PARPORT=true
    fi
],
[
    AC_MSG_RESULT([no (default)])
])
AC_SUBST(ENABLE_PARPORT)

AM_CONDITIONAL(ENABLE_PARPORT, test "${ENABLE_PARPORT}" = "true")

#Check for profiling
AC_MSG_CHECKING(if profiling is enabled)
ENABLE_PROFILING="false"
AC_ARG_ENABLE(profiling,
[  --enable-profiling    Enable profiling (default false)],
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

AC_MSG_CHECKING([if cflag _XOPEN_SOURCE is required])
case "${host_cpu}-${host_os}" in
	*-aix* | *-freebsd* | *-darwin* | *solaris* | *-osf* | *-hpux*)
		CFLAGS="$CFLAGS -D_XOPEN_SOURCE=500"
		AC_MSG_RESULT([-D_XOPEN_SOURCE=500])
	;;
	*)
		AC_MSG_RESULT([no])
	;;
esac

# Check if extra libs are needed. Should perhaps test if they are available too
# instead of hardcoding the libraries.

AC_MSG_CHECKING([if extra libs are required])
case "${host_cpu}-${host_os}" in
	*-darwin*)
		LD_EXTRALIBS="-framework CoreFoundation -framework IOKit"
		AC_MSG_RESULT([-framework CoreFoundation -framework IOKit])
	;;
	*-solaris*)
                LIBS="$LIBS -lsocket -lnsl"
		LD_EXTRALIBS="-lsocket -lnsl"
		AC_MSG_RESULT([-lsocket -lnsl])
	;;
	*)
		LD_EXTRALIBS=""
		AC_MSG_RESULT([no])
	;;
esac
AC_SUBST(LD_EXTRALIBS)

# Checks for header files.

AC_HEADER_DIRENT
AC_HEADER_STDC
AC_CHECK_HEADERS([sys/mkdev.h sys/time.h sys/types.h feature_tests.h fcntl.h netinet/in.h stdlib.h string.h sys/file.h syslog.h termios.h unistd.h limits.h stdint.h features.h getopt.h linux/limits.h bits/netdb.h bits/siginfo.h])

# owftpd section
AC_SUBST(HAVE_NEW_SS_FAMILY)
AC_EGREP_HEADER(__ss_family,
    [sys/socket.h],
    [HAVE_NEW_SS_FAMILY=false],
    [HAVE_NEW_SS_FAMILY=true] )
AC_MSG_WARN([SS is ${HAVE_NEW_SS_FAMILY}])
AM_CONDITIONAL(HAVE_NEW_SS_FAMILY, test "${HAVE_NEW_SS_FAMILY}" = "true")
# end owftpd section


# conditionals for httpd/ftpd/nfsd
AM_CONDITIONAL(ENABLE_OWHTTPD, test "${ENABLE_OWHTTPD}" = "true")
AM_CONDITIONAL(ENABLE_OWFTPD, test "${ENABLE_OWFTPD}" = "true")
AM_CONDITIONAL(ENABLE_OWCAPI, test "${ENABLE_OWCAPI}" = "true")


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
AC_CHECK_FUNCS([accept daemon getaddrinfo freeaddrinfo getopt getopt_long gettimeofday inet_ntop inet_pton memchr memset nanosleep select socket strcasecmp strchr strdup strncasecmp strtol strtoul tsearch tfind tdelete tdestroy vasprintf usb_interrupt_read strsep vsprintf vsnprintf])

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
    module/owserver/Makefile
    module/owserver/src/Makefile
    module/owserver/src/include/Makefile
    module/owserver/src/c/Makefile
    module/owserver/src/man/Makefile
    module/ownfsd/Makefile
    module/ownfsd/src/Makefile
    module/ownfsd/src/include/Makefile
    module/ownfsd/src/c/Makefile
    module/ownfsd/src/man/Makefile
    module/owftpd/Makefile
    module/owftpd/src/Makefile
    module/owftpd/src/include/Makefile
    module/owftpd/src/c/Makefile
    module/owftpd/src/man/Makefile
    module/owcapi/Makefile
    module/owcapi/src/Makefile
    module/owcapi/src/include/Makefile
    module/owcapi/src/c/Makefile
    module/owcapi/src/man/Makefile
    module/swig/Makefile
    module/swig/perl5/Makefile
    module/swig/perl5/Makefile.PL
    module/swig/php/Makefile
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
if test "${ENABLE_PROFILING}" = "true"; then
    AC_MSG_RESULT([    Profiling is enabled])
else
    AC_MSG_RESULT([    Profiling is DISABLED])
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
if test "${ENABLE_OWNFSD}" = "true"; then
    AC_MSG_RESULT([    ownfsd is enabled])
else
    AC_MSG_RESULT([    ownfsd is DISABLED])
fi
if test "${ENABLE_OWFTPD}" = "true"; then
    AC_MSG_RESULT([    owftpd is enabled])
else
    AC_MSG_RESULT([    owftpd is DISABLED])
fi
if test "${ENABLE_OWSERVER}" = "true"; then
    AC_MSG_RESULT([    owserver is enabled])
else
    AC_MSG_RESULT([    owserver is DISABLED])
fi
if test "${ENABLE_OWCAPI}" = "true"; then
    AC_MSG_RESULT([    owcapi is enabled])
else
    AC_MSG_RESULT([    owcapi is DISABLED])
fi
if test "${ENABLE_SWIG}" = "true"; then
    AC_MSG_RESULT([    swig is enabled])
else
    AC_MSG_RESULT([    swig is DISABLED])
fi
if test "${ENABLE_OWPERL}" = "true"; then
    AC_MSG_RESULT([    owperl is enabled])
else
    AC_MSG_RESULT([    owperl is DISABLED])
fi
if test "${ENABLE_OWPHP}" = "true"; then
    AC_MSG_RESULT([    owphp is enabled])
else
    AC_MSG_RESULT([    owphp is DISABLED])
fi
if test "${ENABLE_OWPYTHON}" = "true"; then
    AC_MSG_RESULT([    owpython is enabled])
else
    AC_MSG_RESULT([    owpython is DISABLED])
fi
if test "${ENABLE_OWTCL}" = "true"; then
    AC_MSG_RESULT([    owtcl is enabled])
else
    AC_MSG_RESULT([    owtcl is DISABLED])
fi
AC_MSG_RESULT()
