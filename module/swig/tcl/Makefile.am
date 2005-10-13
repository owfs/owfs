# $Id$

include version.h

EXTRA_DIST = owtclInitScript.h tcl.m4 ow.tcl

pkglib_LTLIBRARIES = ow.la
ow_la_SOURCES = ow.c
ow_la_LDFLAGS = -module -release $(OWTCL_VERSION)
ow_la_DEPENDENCIES = ../../owlib/src/c/libow.la
nodist_ow_la_SOURCES = owtclInitScript.h ow.tcl.h version.h
ow.lo: owtclInitScript.h ow.tcl.h version.h

data_DATA = ow.tcl pkgIndex.tcl
#mann_MANS = owtcl.n

PACKAGE = ow
MAJOR_VERSION = $(OWTCL_MAJOR_VERSION)
MINOR_VERSION = $(OWTCL_MINOR_VERSION)

pkglibdir = $(TCL_BIN_DIR)/owtcl-$(OWTCL_VERSION)
datadir = $(pkglibdir)

if ENABLE_MT
MT_FLAG = -DOW_MT
endif

SHLIB_SUFFIX = $(TCL_SHLIB_SUFFIX)

LIBTOOL_DEPS = @LIBTOOL_DEPS@


AM_CFLAGS = -I../../owlib/src/include \
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
	${MT_FLAG} \
	$(TCL_DEFS) \
	$(TCL_SHLIB_CFLAGS) \
	$(TCL_INCLUDE_SPEC) \
	-DTCL_PACKAGE_PATH=\"'$(TCL_PACKAGE_PATH)'\" \
	-DOWTCL_PACKAGE_PATH=\"$(pkglibdir)\"


AM_LDFLAGS = -L../../owlib/src/c $(TCL_LIB_SPEC) -low ${PTHREAD_LIBS}

#SHLIB_LD = $(TCL_SHLIB_LD) -low $(LIBS)

clean-local:
	@RM@ -f ow.tcl.h pkgIndex.tcl

clean-generic:

	@RM@ -f *~ .*~

ow.tcl.h: ow.tcl ${ow_la_DEPENDENCIES}
	sed -e '/^$\#/d' -e '/^$$/d' -e 's/\"/\\"/g' -e 's/^/"/' -e 's/$$/\\n"/' <ow.tcl > $@ || { $(RM) $@; exit 1; }

pkgIndex.tcl:
	(echo 'if {[catch {package require Tcl 8.1}]} return';\
	echo 'package ifneeded $(PACKAGE) $(OWTCL_VERSION)\
	     [list load [file join $$dir $(PACKAGE)$(TCL_SHLIB_SUFFIX)] $(PACKAGE)]'\
	) > pkgIndex.tcl

