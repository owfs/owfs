# $Id$

LIBOWNET_SOURCE =	dummy1.c	\
        compat.c   \
        dummy1.c   \
        dummy2.c   \
        error.c   \
        getaddrinfo.c   \
        getopt.c   \
        globals.c   \
        ow_api.c   \
        ow_browse.c   \
        ow_charblob.c   \
        ow_connect.c   \
        ow_dirblob.c   \
        ow_dl.c   \
        ow_dnssd.c   \
        owlib.c   \
        ow_locks.c   \
        ow_memblob.c   \
        ow_net_client.c   \
        ownet_close.c   \
        ownet_dir.c   \
        ownet_init.c   \
        ownet_read.c   \
        ownet_setget.c   \
        ownet_write.c   \
        ow_opt.c   \
        ow_server.c   \
        ow_tcp_read.c   \
        ow_util.c   \
        ow_zero.c   \
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

