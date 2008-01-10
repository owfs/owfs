# $Id$

LIBOWNET_SOURCE =	dummy1.c	\
			dummy2.c


lib_LTLIBRARIES = libownet.la

libownet_la_SOURCES = ${LIBOWNET_SOURCE}


if HAVE_CYGWIN
libownet_la_LDFLAGS = ${PROFILING_FLAG} ${PTHREAD_LIBS} -shared -no-undefined ${LIBUSB_LIBS}
else
libownet_la_LDFLAGS = -version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE) -release $(LT_RELEASE) ${PROFILING_FLAG} ${PTHREAD_LIBS} -shared -shrext .so ${LIBUSB_LIBS}
endif

# Maybe need this for MacOS X
#if HAVE_DARWIN
#LDADDS = -framework IOKit -framework CoreFoundation
#endif
#libow_la_LDFLAGS = -shared -shrext .so \
#	-version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE) \
#	-release $(LT_RELEASE) \
#	-export-dynamic \
#	$(LDADDS)

AM_CFLAGS = -I../include \
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
         -D__EXTENSIONS__ \
         ${PROFILING_FLAG} \
	 ${PTHREAD_CFLAGS} \
	 ${LIBUSB_CFLAGS}

clean-generic:

	@RM@ -f *~ .*~

