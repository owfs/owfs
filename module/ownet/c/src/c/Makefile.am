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
        ownet_present.c \
        ownet_setget.c  \
        ownet_write.c   \
        ow_rwlock.c     \
        ow_server.c     \
        ow_tcp_read.c

lib_LTLIBRARIES = libownet.la

libownet_la_SOURCES = ${LIBOWNET_SOURCE}


if HAVE_CYGWIN
libownet_la_LDFLAGS = ${PTHREAD_LIBS} ${LD_EXTRALIBS} -shared -no-undefined
else
libownet_la_LDFLAGS = -version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE) -release $(LT_RELEASE) ${PTHREAD_LIBS} ${LD_EXTRALIBS} -shared -shrext .so ${DL_LIBS}
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
	-D__EXTENSIONS__ \
	${EXTRACFLAGS} \
	${PTHREAD_CFLAGS} \
	${LIBAVAHI_CFLAGS} \
	${PIC_FLAGS}

