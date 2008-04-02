# $Id$

bin_PROGRAMS = owhttpd
owhttpd_SOURCES = owhttpd.c          \
                  owhttpd_handler.c  \
                  owhttpd_present.c  \
                  owhttpd_write.c    \
                  owhttpd_read.c     \
                  owhttpd_dir.c      \
                  owhttpd_favicon.c

owhttpd_DEPENDENCIES = ../../../owlib/src/c/libow.la

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

LDADD = -low ${LIBUSB_LIBS} ${PROFILING_FLAG} ${PTHREAD_LIBS} ${LD_EXTRALIBS} ${OSLIBS}

clean-generic:

	@RM@ -f *~ .*~
