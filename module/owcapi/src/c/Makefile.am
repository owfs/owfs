# $Id$

libowcapi_la_DEPENDENCIES = ../../../owlib/src/c/libow.la

lib_LTLIBRARIES = libowcapi.la

libowcapi_la_SOURCES = owcapi.c
#perhaps define this... haven't tried yet
libowcapi_la_LDFLAGS = -shared -shrext .so

if ENABLE_MT
MT_FLAG = -DOW_MT
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
         ${MT_FLAG} \
         ${PROFILING_FLAG} \
         -D__EXTENSIONS__ \
         -D_FILE_OFFSET_BITS=64

AM_LDFLAGS = -low ${PROFILING_FLAG}

clean-generic:

	@RM@ -f *~ .*~
