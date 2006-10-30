# $Id$

#if HAVE_CYGWIN
#if ENABLE_ZERO
#DNSSDLIB = ../../../../src/scripts/windows/libdnssd.dll.a
#endif
#endif

bin_PROGRAMS = owserver
owserver_SOURCES = owserver.c
owserver_DEPENDENCIES = ../../../owlib/src/c/libow.la $(DNSSDLIB)

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
         ${PROFILING_FLAG} \
         -D_FILE_OFFSET_BITS=64

LDADD = -low ${PROFILING_FLAG} ${PTHREAD_LIBS} ${LD_EXTRALIBS}

clean-generic:

	@RM@ -f *~ .*~
