# $Id$

libowcapi_la_DEPENDENCIES = ../../../owlib/src/c/libow.la

lib_LTLIBRARIES = libowcapi.la

libowcapi_la_SOURCES = owcapi.c
libowcapi_la_LDFLAGS = -low ${PROFILING_FLAG} -shared -shrext .so

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
         ${PROFILING_FLAG} \
         -D__EXTENSIONS__ \
         -D_FILE_OFFSET_BITS=64

clean-generic:

	@RM@ -f *~ .*~
