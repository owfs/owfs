# $Id$

OWLIB_SOURCE = ow_dl.c           \
               ow_dnssd.c        \
               ow_browse.c       \
               compat.c          \
               getaddrinfo.c     \
               getopt.c          \
               ow_xxxx.c         \
               ow_none.c         \
               ow_1820.c         \
               ow_1821.c         \
               ow_1921.c         \
               ow_1923.c         \
               ow_1963.c         \
               ow_1977.c         \
               ow_1991.c         \
               ow_1993.c         \
               ow_2401.c         \
               ow_2404.c         \
               ow_2405.c         \
               ow_2406.c         \
               ow_2408.c         \
               ow_2409.c         \
               ow_2413.c         \
               ow_2415.c         \
               ow_2423.c         \
               ow_2430.c         \
               ow_2433.c         \
               ow_2436.c         \
               ow_2438.c         \
               ow_2450.c         \
               ow_2502.c         \
               ow_2505.c         \
               ow_2760.c         \
               ow_2804.c         \
               ow_2890.c         \
               ow_api.c          \
               ow_badadapter.c   \
               ow_bus.c          \
               ow_cache.c        \
               ow_charblob.c     \
               ow_com.c          \
               ow_connect.c      \
               ow_crc.c          \
               ow_delay.c        \
               ow_dir.c          \
               ow_dirblob.c      \
               ow_ds2482.c       \
               ow_ds9097.c       \
               ow_ds1410.c       \
               ow_ds9097U.c      \
               ow_ds9490.c       \
               ow_ha7.c          \
               ow_fake.c         \
               ow_fakeread.c     \
               ow_filelength.c   \
               ow_fstat.c        \
               ow_help.c         \
               ow_iterate.c      \
               ow_lcd.c          \
               ow_link.c         \
               ow_locator.c      \
               ow_locks.c        \
               ow_memory.c       \
               ow_multicast.c    \
               ow_parsename.c    \
               ow_pid.c          \
               ow_presence.c     \
               ow_name.c         \
               ow_net.c          \
               ow_opt.c          \
               ow_read.c         \
               ow_reconnect.c    \
               ow_stats.c        \
               ow_server.c       \
               ow_select.c       \
               ow_settings.c     \
               ow_simultaneous.c \
               ow_system.c       \
               ow_tcp_read.c     \
               ow_temp.c         \
               ow_tree.c         \
               ow_transaction.c  \
               ow_util.c         \
               ow_verify.c       \
               ow_write.c        \
               ow_zero.c         \
               owlib.c           \
               error.c           \
               globals.c

lib_LTLIBRARIES = libow.la

libow_la_SOURCES = ${OWLIB_SOURCE}

if HAVE_CYGWIN
libow_la_LDFLAGS = ${PROFILING_FLAG} ${PTHREAD_LIBS} -shared -no-undefined ${LIBUSB_LIBS}
else
libow_la_LDFLAGS = ${PROFILING_FLAG} ${PTHREAD_LIBS} -shared -shrext .so ${LIBUSB_LIBS}
endif
# -version-info 0:0:1 -release ow

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
	 ${LIBUSB_CFLAGS}

clean-generic:

	@RM@ -f *~ .*~

