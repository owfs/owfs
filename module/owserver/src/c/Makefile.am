# $Id$

if ENABLE_OWSERVER
  MODULE_OWSERVER = owserver
endif

if ENABLE_OWEXTERNAL
  MODULE_OWEXTERNAL = owexternal
endif

bin_PROGRAMS = $(MODULE_OWSERVER) $(MODULE_OWEXTERNAL)

owserver_SOURCES = owserver.c  \
                   from_client.c \
                   to_client.c   \
                   read.c        \
                   write.c       \
                   dir.c         \
                   dirall.c      \
                   dirallslash.c \
                   data.c        \
                   error.c       \
                   handler.c     \
                   loop.c        \
                   ping.c
                   
owserver_DEPENDENCIES = ../../../owlib/src/c/libow.la

owexternal_SOURCES = $(owserver_SOURCES)

owexternal_DEPENDENCIES = $(owserver_DEPENDENCIES)

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
	${EXTRACFLAGS} \
	${PTHREAD_CFLAGS} \
	${LIBUSB_CFLAGS}

LDADD = -low ${LIBUSB_LIBS} ${PTHREAD_LIBS} ${LD_EXTRALIBS} ${OSLIBS}

clean-generic:

	@RM@ -f *~ .*~
