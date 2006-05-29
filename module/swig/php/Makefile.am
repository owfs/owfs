# $Id$

EXTRA_DIST = php4.m4 example/runme.php example/load_php_OW.php.in example/load_php_OW.php

#include owfs header and php headers
INCLUDES = -I.. -I$(top_srcdir)/src/include -I$(top_srcdir)/module/owlib/src/include @PHP4INC@

#use libtool to build the php extension
lib_LTLIBRARIES = libowphp.la
libowphp_la_LIBADD = ../../owlib/src/c/libow.la
libowphp_la_DEPENDENCIES = ../../owlib/src/c/libow.la
libowphp_la_LDFLAGS = -shared -shrext .so

# Older php4 installations doesn't handle owfs_config.h filename, and tries
# to include non-existing config.h if HAVE_CONFIG_H is defined.
#DEFS+= -UHAVE_CONFIG_H

AM_CFLAGS = -I../include \
	-fexceptions \
	-D_FILE_OFFSET_BITS=64 \
	${MT_FLAG}

LDADD = -L../../owlib/src/c -low ${PTHREAD_LIBS} ${LD_EXTRALIBS}

libowphp_la_SOURCES = ow_wrap.c

ow_wrap.c: ../ow.i
	$(SWIG) -php4 -o $@ ../ow.i

EXTENSION_DIR := @PHP4LIBDIR@
# Something like /usr/lib/php4, /usr/lib/php/modules/ or /usr/lib64/php/modules/

install-data-local:
	$(INSTALL) -d $(DESTDIR)$(EXTENSION_DIR)
	ln -sf ${exec_prefix}/lib/libowphp.so $(DESTDIR)$(EXTENSION_DIR)/php_OW.so

CLEANFILES = *~ *.o ow_wrap.c OW.* php_OW.h

clean-local:
	@RM@ -f $(CLEANFILES)

clean-generic:

	@RM@ -f *~ .*~
