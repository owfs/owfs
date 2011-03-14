# $Id$

libowcapi_la_DEPENDENCIES = ../../../owlib/src/c/libow.la

lib_LTLIBRARIES = libowcapi.la

libowcapi_la_SOURCES = owcapi.c

if HAVE_CYGWIN
libowcapi_la_LDFLAGS = -low ${LD_EXTRALIBS} -shared -no-undefined 
else if HAVE_DEBIAN
# Debian patch
libowcapi_la_LDFLAGS = -low -version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE) -release $(LT_RELEASE) ${PTHREAD_LIBS} ${LD_EXTRALIBS} -shared -shrext .so
else
libowcapi_la_LDFLAGS = -low -version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE) -release $(LT_RELEASE) ${PTHREAD_LIBS} ${LIBUSB_LIBS} ${LD_EXTRALIBS} -shared -shrext .so
endif

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
	-D__EXTENSIONS__ \
	${EXTRACFLAGS} \
	${PTHREAD_CFLAGS} \
	${LIBUSB_CFLAGS} \
	${PIC_FLAGS}

clean-generic:

	@RM@ -f *~ .*~
