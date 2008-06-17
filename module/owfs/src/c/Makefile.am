# $Id$

bin_PROGRAMS = owfs
owfs_SOURCES = owfs.c owfs_callback.c fuse_line.c
owfs_DEPENDENCIES = ../../../owlib/src/c/libow.la

AM_CFLAGS = -I../include \
	${FUSE_FLAGS} \
	${FUSE_INCLUDES} \
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
	${LIBUSB_CFLAGS} \
	${PTHREAD_CFLAGS}

LDADD = -low ${FUSE_LIBS} ${LIBUSB_LIBS} ${PTHREAD_LIBS} ${LD_EXTRALIBS} ${OSLIBS}

clean-generic:

	@RM@ -f *~ .*~
