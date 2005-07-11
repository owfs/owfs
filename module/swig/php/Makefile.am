# $Id$

#include owfs header and php headers
INCLUDES = -I.. -I$(top_srcdir)/src/include -I$(top_srcdir)/module/owlib/src/include `php-config --includes`

#use libtool to build the php extension
lib_LTLIBRARIES = libowphp.la
# libowphp_la_LIBADD = $(top_builddir)/module/owlib/src/c/libow.la
# libowphp_la_DEPENDENCIES = $(top_builddir)/module/owlib/src/c/libow.la
libowphp_la_LIBADD = ../../owlib/src/c/libow.la
libowphp_la_DEPENDENCIES = ../../owlib/src/c/libow.la
libowphp_la_LDFLAGS = -shared -shrext .so

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

AM_LDFLAGS = ${THREADS_LIBS}

libowphp_la_SOURCES = ow_wrap.c

ow_wrap.c: ../ow.i
	$(SWIG) -php4 -o $@ ../ow.i

install:
	$(INSTALL) -d /usr/lib/php/modules/
	ln -sf ${exec_prefix}/lib/libowphp.so /usr/lib/php/modules/php_OW.so

CLEANFILES = *~ *.o ow_wrap.c OW.* php_OW.h

clean-local:
	@RM@ -f $(CLEANFILES)

clean-generic:

	@RM@ -f *~ .*~
