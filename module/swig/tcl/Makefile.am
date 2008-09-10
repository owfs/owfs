# $Id$

include version.h

EXTRA_DIST = owtclInitScript.h tcl.m4 ow.tcl example/owdir.tcl example/test.tcl

pkglib_LTLIBRARIES = ow.la
ow_la_SOURCES = ow.c
ow_la_LDFLAGS = -module -release $(OWTCL_VERSION) -shared -shrext "$(TCL_SHLIB_SUFFIX)"
#ow_la_LIBADD = ../../owlib/src/c/libow.la ../../owcapi/src/c/libowcapi.la
ow_la_LIBADD = $(TCL_LIB_SPEC) ../../owlib/src/c/libow.la ../../owcapi/src/c/libowcapi.la ${TCL_LIBS}
ow_la_DEPENDENCIES = ../../owlib/src/c/libow.la ../../owcapi/src/c/libowcapi.la
ow.lo: owtclInitScript.h ow.tcl.h version.h

data_DATA = ow.tcl pkgIndex.tcl
if HAVE_DARWIN
noinst_DATA = darwin-relink
endif

PACKAGE = ow
MAJOR_VERSION = $(OWTCL_MAJOR_VERSION)
MINOR_VERSION = $(OWTCL_MINOR_VERSION)

pkglibdir = @OWTCL_INSTALL_PATH@/owtcl-$(OWTCL_VERSION)
#something like this. $(TCL_EXEC_PREFIX)/lib@LIBPOSTFIX@/owtcl-$(OWTCL_VERSION)
datadir = $(pkglibdir)

#SHLIB_SUFFIX = $(TCL_SHLIB_SUFFIX)

LIBTOOL_DEPS = @LIBTOOL_DEPS@

AM_CFLAGS = -I../../owlib/src/include \
	-I ../../owcapi/src/include \
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
	${EXTRACFLAGS} \
	${PTHREAD_CFLAGS} \
	$(TCL_DEFS) \
	$(TCL_SHLIB_CFLAGS) \
	$(TCL_INCLUDE_SPEC) \
	-DTCL_PACKAGE_PATH=\"'$(TCL_PACKAGE_PATH)'\" \
	-DOWTCL_PACKAGE_PATH=\"$(pkglibdir)\" \
	${PIC_FLAGS}

#LDADD = -L../../owlib/src/c $(TCL_LIB_SPEC) -low -lowcapi ${TCL_LIBS} ${PTHREAD_LIBS} ${LD_EXTRALIBS} ${OSLIBS}
#LDADD = $(TCL_LIB_SPEC) ../../owlib/src/c/libow.la ../../owcapi/src/c/libowcapi.la ${TCL_LIBS} ${PTHREAD_CFLAGS} ${PTHREAD_LIBS}
#SHLIB_LD = $(TCL_SHLIB_LD) $(LIBS) -low ${PTHREAD_LIBS} ${LD_EXTRALIBS} ${OSLIBS}

clean-local:
	@RM@ -f ow.tcl.h pkgIndex.tcl darwin-relink

clean-generic:

	@RM@ -f *~ .*~

ow.tcl.h: ow.tcl ${ow_la_DEPENDENCIES}
	sed -e '/^$\#/d' -e '/^$$/d' -e 's/\"/\\"/g' -e 's/^/"/' -e 's/$$/\\n"/' <ow.tcl > $@ || { $(RM) $@; exit 1; }

# Have to relink the library since it's generated with -bundle, it should be -dynamiclib
darwin-relink: ow.la
#	mv .libs/ow-$(OWTCL_VERSION)$(TCL_SHLIB_SUFFIX) .libs/ow-$(OWTCL_VERSION)$(TCL_SHLIB_SUFFIX).old
	gcc -dynamiclib -flat_namespace -undefined suppress \
	  -o .libs/ow-$(OWTCL_VERSION)$(TCL_SHLIB_SUFFIX) \
	  .libs/ow.o \
	  ../../owlib/src/c/.libs/libow.so \
	  ${LIBUSB_LIBS} \
	  ../../owcapi/src/c/.libs/libowcapi.so \
	 -install_name /usr/lib/owtcl-$(OWTCL_VERSION)/ow-$(OWTCL_VERSION)$(TCL_SHLIB_SUFFIX) \
	 -compatibility_version 1 -current_version 1.0
	touch $@

pkgIndex.tcl:
	(echo 'if {[catch {package require Tcl 8.1}]} return';\
	echo 'package ifneeded $(PACKAGE) $(OWTCL_VERSION)\
	     [list load [file join $$dir $(PACKAGE)$(TCL_SHLIB_SUFFIX)] $(PACKAGE)]'\
	) > pkgIndex.tcl

