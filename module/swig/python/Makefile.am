# $Id$

SUBDIRS = ow

bin_PROGRAMS = owpython

owpython_SOURCES = ow_wrap.c
owpython_DEPENDENCIES = ../../owlib/src/c/libow.la

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

AM_LDFLAGS = ${PTHREAD_LIBS}

#noinst_DATA = .made_mod

#.made_mod: owperl_wrap.c Makefile.perl
#	$(MAKE) -fMakefile.perl
#	touch $@

ow_wrap.c: ../ow.i ${owpython_DEPENDENCIES}
	$(SWIG) -python -o $@ ../ow.i

owpython: ow_wrap.c setup.py
	$(PYTHON) setup.py build

install-data-local:
	$(PYTHON) setup.py install

CLEANFILES = *~ *.o

clean-local:
	@RM@ -rf build OW.py ow_wrap.c

clean-generic:
	@RM@ -f *~ .*~
