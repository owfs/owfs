# $Id$

bin_PROGRAMS = owdir
owfs_SOURCES = owdir.c \
               ow_read.c  \
               ow_help.c \
               ow_connect.c \
               ow_server.c \
               ow_net.c \
               ow_connect.c \
               globals.c \
               ow_opt.c
owfs_DEPENDENCIES =

AM_CFLAGS = -I../include \
         -D_FILE_OFFSET_BITS=64 \
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
