# $Id$

bin_PROGRAMS = owserver
owserver_SOURCES = owserver.c
owserver_DEPENDENCIES = ../../../owlib/src/c/libow.la

AM_CFLAGS = -I../include \
         -I../../../owlib/src/include \
         -L../../../owlib/src/c \
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
         -D_FILE_OFFSET_BITS=64 \
         ${PROFILING_FLAG} \
	 ${LIBUSB_CFLAGS}

LDADD = -low ${LIBUSB_LIBS} ${PROFILING_FLAG} ${PTHREAD_LIBS} ${LD_EXTRALIBS} ${OSLIBS}

clean-generic:

	@RM@ -f *~ .*~
