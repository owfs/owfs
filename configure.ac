dnl Process this file with autoconf to produce a configure script.

AC_REVISION($Id$)

AC_INIT
AC_CONFIG_SRCDIR([src/include/owfs_config.h.in])

VERSION_MAJOR="0"
VERSION_MINOR="98"
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
AC_PREREQ(2.53)dnl
AC_CONFIG_AUX_DIR(src/scripts/install)dnl

AM_INIT_AUTOMAKE(${PACKAGE}, ${VERSION})dnl

# put them in config.h
AH_TOP([
#ifndef OWFS_CONFIG_H
#define OWFS_CONFIG_H
])
AH_BOTTOM([#endif])

AM_CONFIG_HEADER(src/include/owfs_config.h)

# Checks for programs.
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

# Keep in mind that we may be installed in /opt/owfs

LDFLAGS="-L/opt/owfs/lib $LDFLAGS"
CPPFLAGS="-I/opt/owfs/include $CPPFLAGS"

# Checks for libraries.

# A major annoyance, libdb. If owfs is linked with it, then the configure
# script will want it, too. It'll fail unless the access to the library is
# provided.

AC_CHECK_LIB(db,__db_dbopen,[
    AC_MSG_CHECKING([if libdb < DB41])
    AC_MSG_RESULT()
    AC_CHECK_LIB(db,__log_register,[ENABLE_CACHE41=false],[ENABLE_CACHE41=true])
    
    # VT: Provide the next check with the flag that allows it to use libdb
    
    LDFLAGS="-ldb ${LDFLAGS}"
    ],[
    AC_MSG_WARN([libdb1 not found, cache will be disabled])
])

AC_CHECK_LIB(ow,DS2480_detect, ,[AC_MSG_ERROR([

You have to have libow installed first - get it from http://sf.net/projects/owfs
If you *do* have it installed, then you may need to specify LDFLAGS=-L\${owlib-prefix}/lib
Default \${owlib-prefix} value is /opt/owfs.

Alternatively, install the owlib RPM.

])])

# Checks for header files.

AC_HEADER_DIRENT
AC_HEADER_STDC
AC_HEADER_SYS_WAIT
AC_CHECK_HEADERS([arpa/inet.h fcntl.h netinet/in.h stdlib.h string.h sys/file.h sys/socket.h syslog.h termios.h unistd.h])
AC_CHECK_HEADERS([ow_stub.h], ,[AC_MSG_ERROR([

You have to have libow installed first - get it from http://sf.net/projects/owfs
If you *do* have it installed, then you may need to specify CPPFLAGS=-I\${owlib-prefix}/include
Default \${owlib-prefix} value is /opt/owfs.

Alternatively, install the owlib RPM.

])])

# Checks for typedefs, structures, and compiler characteristics.

AC_C_CONST
AC_HEADER_TIME
AC_TYPE_OFF_T
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_STRUCT_TM

# Checks for library functions.

AC_FUNC_FORK
AC_FUNC_MALLOC
AC_FUNC_MEMCMP
AC_FUNC_MKTIME
AC_FUNC_VPRINTF
AC_TYPE_SIGNAL
AC_CHECK_FUNCS([memchr memset select socket strchr strdup strncasecmp strtol])

#AC_MSG_CHECKING(if we have socklen_t)
#AC_EGREP_CPP(socklen_t,[
##define _POSIX_PII_SOCKET	
##include <sys/types.h>
##include <sys/socket.h>],
#  socklen=yes, socklen=no)
#if test $socklen = yes
#then
#	AC_DEFINE(HAVE_SOCKLEN_T)
#	AC_MSG_RESULT(yes)
#else
#	AC_MSG_RESULT(no)
#fi
AC_OUTPUT([Makefile
           src/Makefile
           src/c/Makefile
           src/include/Makefile
           src/man/Makefile
           src/rpm/Makefile
           src/rpm/owfs.spec
           src/scripts/Makefile
])
