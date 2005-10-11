# $Id$

owcapi_SOURCES = owcapi.c
libowcapi_la_DEPENDENCIES = ../../../owlib/src/c/libow.la

lib_LTLIBRARIES = libowcapi.la

libowcapi_la_SOURCES = ${owcapi_SOURCES}

if ENABLE_CACHE
CACHE_FLAG = -DOW_CACHE
endif

if ENABLE_USB
USB_FLAG = -DOW_USB
endif

if ENABLE_PROFILING
PROFILING_FLAG = -pg
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
         -Wconversion \
         -Wstrict-prototypes \
         -Wredundant-decls \
         ${CACHE_FLAG} \
         ${USB_FLAG} \
         ${PROFILING_FLAG} \
         -D__EXTENSIONS__ \
         -D_FILE_OFFSET_BITS=64

AM_LDFLAGS = ${PROFILING_FLAG}

clean-generic:

	rm -f *~ .*~
