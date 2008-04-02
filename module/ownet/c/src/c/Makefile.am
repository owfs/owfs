# $Id$

LIBOWNET_SOURCE =	    \
        compat.c        \
        error.c         \
        getaddrinfo.c   \
        getopt.c        \
        globals.c       \
        ow_browse.c     \
        ow_charblob.c   \
        ow_connect.c    \
        ow_dl.c         \
        ow_dnssd.c      \
        ow_locks.c      \
        ow_net_client.c \
        ownet_close.c   \
        ownet_dir.c     \
        ownet_init.c    \
        ownet_read.c    \
        ownet_setget.c  \
        ownet_write.c   \
        ow_rwlock.c     \
        ow_server.c     \
        ow_tcp_read.c   \
        ow_util.c

#        ow_opt.c   


lib_LTLIBRARIES = libownet.la

libownet_la_SOURCES = ${LIBOWNET_SOURCE}


if HAVE_CYGWIN
#libownet_la_LDFLAGS = ${PROFILING_FLAG} ${PTHREAD_LIBS} -shared -no-undefined ${LIBUSB_LIBS}
libownet_la_LDFLAGS = ${PROFILING_FLAG} ${PTHREAD_LIBS} -shared -no-undefined
else
libownet_la_LDFLAGS = -version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE) -release $(LT_RELEASE) ${PROFILING_FLAG} ${PTHREAD_LIBS} -shared -shrext .so
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
	 ${PTHREAD_CFLAGS}

clean-generic:

	@RM@ -f *~ .*~

