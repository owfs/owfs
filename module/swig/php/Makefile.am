# $Id$


EXTRA_DIST = php.m4 example/owdir.php example/load_php_OW.php.in
DISTCLEANFILES = example/load_php_OW.php

#use libtool to build the php extension
lib_LTLIBRARIES = libowphp.la
libowphp_la_LIBADD = ../../owlib/src/c/libow.la
libowphp_la_DEPENDENCIES = ../../owlib/src/c/libow.la
libowphp_la_LDFLAGS = -shared -shrext .so

# Older php installations doesn't handle owfs_config.h filename, and tries
# to include non-existing config.h if HAVE_CONFIG_H is defined.
#DEFS+= -UHAVE_CONFIG_H

EXTENSION_DIR = @PHPLIBDIR@
# Something like /usr/lib/php4, /usr/lib/php5/20041030, /usr/lib/php/modules/ or /usr/lib64/php/modules/

libdir = $(EXTENSION_DIR)

AM_CFLAGS = -fexceptions \
	-I.. \
	-I$(top_srcdir)/src/include \
	-I$(top_srcdir)/module/owlib/src/include \
	${EXTRACFLAGS} \
	${PTHREAD_CFLAGS} \
	@PHPINC@

LDADD = -L../../owlib/src/c -low ${PTHREAD_LIBS} ${LD_EXTRALIBS} ${OSLIBS}

nodist_libowphp_la_SOURCES = ow_wrap.c php_OW.h

ow_wrap.c: ../ow.i
if ENABLE_OWPHP
	$(SWIG) -php -o $@ ../ow.i
endif

#install-data-local:
#	@INSTALL@ -d $(DESTDIR)$(EXTENSION_DIR)
#	@INSTALL@ $(DESTDIR)${libdir}/libowphp.so $(DESTDIR)$(EXTENSION_DIR)/
#	ln -sf ${exec_prefix}/lib/libowphp.so $(DESTDIR)$(EXTENSION_DIR)/libowphp.so
#	ln -sf ../../../lib/libowphp.so $(DESTDIR)$(EXTENSION_DIR)/php_OW.so

clean-local:
	@RM@ -f OW.* ow_wrap.c php_OW.h

clean-generic:
	@RM@ -f *~ .*~
