# $Id$

bin_PROGRAMS = owdir owread owwrite owpresent
owdir_SOURCES = ow_opt.c    \
               ow_help.c    \
               ow_connect.c \
               ow_server.c  \
               ow_net.c     \
               ow_browse.c  \
               getaddrinfo.c  \
               globals.c    \
               owdir.c

owread_SOURCES = ow_opt.c   \
               ow_help.c    \
               ow_connect.c \
               ow_server.c  \
               ow_net.c     \
               ow_browse.c  \
               getaddrinfo.c  \
               globals.c    \
               owread.c

owwrite_SOURCES = ow_opt.c   \
               ow_help.c    \
               ow_connect.c \
               ow_server.c  \
               ow_net.c     \
               ow_browse.c  \
               getaddrinfo.c  \
               globals.c    \
               owwrite.c

owpresent_SOURCES = ow_opt.c   \
               ow_help.c    \
               ow_connect.c \
               ow_server.c  \
               ow_net.c     \
               ow_browse.c  \
               getaddrinfo.c  \
               globals.c    \
               owpresent.c

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

#         -Wconversion \
#

LDADD = ${PROFILING_FLAG} ${LD_EXTRALIBS}

clean-generic:

	@RM@ -f *~ .*~
