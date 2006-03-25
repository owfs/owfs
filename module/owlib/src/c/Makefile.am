# $Id$

OWLIB_SOURCE = compat.c          \
               getopt.c          \
               getaddrinfo.c     \
               ow_xxxx.c         \
               ow_none.c         \
               ow_1820.c         \
               ow_1821.c         \
               ow_1921.c         \
               ow_1923.c         \
               ow_1977.c         \
               ow_2890.c         \
               ow_2401.c         \
               ow_2404.c         \
               ow_2405.c         \
               ow_2406.c         \
               ow_2408.c         \
               ow_2409.c         \
               ow_2413.c         \
               ow_2450.c         \
               ow_2433.c         \
               ow_2436.c         \
               ow_2438.c         \
               ow_2415.c         \
               ow_2423.c         \
               ow_1991.c         \
               ow_1963.c         \
               ow_1993.c         \
               ow_2502.c         \
               ow_2505.c         \
               ow_2760.c         \
               ow_2804.c         \
               ow_badadapter.c   \
               ow_bus.c          \
               ow_cache.c        \
               ow_com.c          \
               ow_connect.c      \
               ow_crc.c          \
               ow_dir.c          \
               ow_ds9097.c       \
               ow_ds1410.c       \
               ow_ds9097U.c      \
               ow_ds9490.c       \
               ow_filelength.c   \
               ow_iterate.c      \
               ow_lcd.c          \
               ow_parsename.c    \
               ow_name.c         \
               ow_read.c         \
               ow_reconnect.c    \
               ow_stats.c        \
               ow_size.c         \
               ow_server.c       \
               ow_settings.c     \
               ow_simultaneous.c \
               ow_system.c       \
               ow_temp.c         \
               ow_tree.c         \
               ow_transaction.c  \
               ow_write.c        \
               ow_verify.c       \
               ow_link.c         \
               ow_util.c         \
               ow_net.c          \
               ow_fstat.c        \
               ow_locks.c        \
               ow_api.c          \
               ow_opt.c          \
               owlib.c           \
               error.c           \
               globals.c

lib_LTLIBRARIES = libow.la

libow_la_SOURCES = ${OWLIB_SOURCE}
libow_la_LDFLAGS = -shared -shrext .so

# Maybe need this for MacOS X
#if DARWIN_API
#LDADDS = -framework IOKit -framework CoreFoundation
#endif
#libow_la_LDFLAGS = -shared -shrext .so \
#	-version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE) \
#	-release $(LT_RELEASE) \
#	-export-dynamic \
#	$(LDADDS)

if ENABLE_CACHE
CACHE_FLAG = -DOW_CACHE
endif

if ENABLE_USB
USB_FLAG = -DOW_USB
endif

if ENABLE_MT
MT_FLAG = -DOW_MT
endif

if ENABLE_PROFILING
PROFILING_FLAG = -pg
endif

if ENABLE_PARPORT
PARPORT_FLAG = -DOW_PARPORT
endif

AM_CFLAGS = -I../include \
         -fexceptions \
         -Wall \
         -W \
         -Wundef \
         -Wshadow \
         -Wpointer-arith \
         -Wcast-qual \
         -Wcast-align \
         -Wconversion \
         -Wstrict-prototypes \
         -Wredundant-decls \
         -D_FILE_OFFSET_BITS=64 \
         -D__EXTENSIONS__ \
         ${CACHE_FLAG} \
         ${MT_FLAG} \
         ${PARPORT_FLAG} \
         ${PROFILING_FLAG} \
         ${USB_FLAG}

AM_LDFLAGS = ${PROFILING_FLAG} ${PTHREAD_LIBS}

clean-generic:

	@RM@ -f *~ .*~

