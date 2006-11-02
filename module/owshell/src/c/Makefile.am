# $Id$

COMMON_OWSHELL_SOURCE = ow_opt.c   \
               ow_help.c    \
               ow_connect.c \
               ow_server.c  \
               ow_net.c     \
               ow_browse.c  \
               ow_dl.c      \
               ow_dnssd.c   \
               getaddrinfo.c  \
               globals.c

bin_PROGRAMS = owdir owread owwrite owpresent
owdir_SOURCES = ${COMMON_OWSHELL_SOURCE} \
               owdir.c

owread_SOURCES = ${COMMON_OWSHELL_SOURCE} \
               owread.c

owwrite_SOURCES = ${COMMON_OWSHELL_SOURCE} \
               owwrite.c

owpresent_SOURCES = ${COMMON_OWSHELL_SOURCE} \
               owpresent.c

owdir_DEPENDENCIES =
owread_DEPENDENCIES =
owwrite_DEPENDENCIES =
owpresent_DEPENDENCIES =

AM_CFLAGS = -I../include \
         -D_FILE_OFFSET_BITS=64 \
         -I../../../owlib/src/include \
         -fexceptions \
         -Wall \
         -W \
         -Wundef \
         -Wshadow \
         -Wpointer-arith \
         -Wcast-qual \
         -Wcast-align \
         -Wstrict-prototypes \
         -Wredundant-decls \
         ${PROFILING_FLAG}

LDADD = ${PROFILING_FLAG} ${LD_EXTRALIBS}

clean-generic:

	@RM@ -f *~ .*~
