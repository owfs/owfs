# $Id$

OWLIB_SOURCE = ow_xxxx.c \
               ow_none.c \
               ow_link.c \
               ow_1820.c \
               ow_1921.c \
               ow_2890.c \
               ow_2401.c \
               ow_2404.c \
               ow_2405.c \
               ow_2406.c \
               ow_2408.c \
               ow_2409.c \
               ow_2450.c \
               ow_2436.c \
               ow_2438.c \
               ow_2415.c \
               ow_2423.c \
               ow_1993.c \
               ow_2502.c \
               ow_2505.c \
               ow_2760.c \
               ow_cache.c \
               ow_lcd.c \
               ow_parsename.c \
               ow_read.c \
               ow_write.c \
               ow_dir.c \
               ow_com.c \
               ow_verify.c \
               ow_ibuttonlink.c \
               ow_bus.c \
               ow_ds9097.c \
               ow_ds1410.c \
               ow_ds9097U.c \
               ow_ds9490.c \
               ow_crc.c \
               ow_util.c \
               ow_iterate.c \
               ow_temp.c \
               ow_stats.c \
               ow_fstat.c \
               ow_locks.c \
               ow_opt.c \
               owlib.c

lib_LTLIBRARIES = libow.la

libow_la_SOURCES = ${OWLIB_SOURCE}

if ENABLE_CACHE
CACHE_FLAG = -DOW_CACHE
endif

if ENABLE_USB
USB_FLAG = -DOW_USB
endif

if ENABLE_MT
MT_FLAG = -DOW_MT
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
         ${CACHE_FLAG} \
         ${MT_FLAG} \
         ${USB_FLAG}

clean-generic:

	@RM@ -f *~ .*~

