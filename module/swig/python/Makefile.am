# $Id$

SUBDIRS = ow
EXTRA_DIST = python.m4

bin_PROGRAMS = OW.py
OW_py_SOURCES = 
OW_py_DEPENDENCIES = ../../owlib/src/c/libow.la

#owpython_SOURCES = ow_wrap.c
#owpython_DEPENDENCIES = ../../owlib/src/c/libow.la
#owpython_LDFLAGS = -shared -shrext .so

AM_CFLAGS = -I../include \
         -fexceptions \
         -D_FILE_OFFSET_BITS=64 \
         ${MT_FLAG}

LDADD = -low ${PTHREAD_LIBS} ${LD_EXTRALIBS}
# ../../owlib/src/c

#noinst_DATA = .made_mod

ow_wrap.c: ../ow.i ${OW_py_DEPENDENCIES}
	$(SWIG) -python -o $@ ../ow.i

OW.py: ow_wrap.c setup.py
	$(PYTHON) setup.py build

install-data-local:
	$(PYTHON) setup.py install --root="/$(DESTDIR)"

CLEANFILES = *~ .*~ *.o

clean-local:
	@RM@ -f $(CLEANFILES)
	@RM@ -rf build OW.py ow_wrap.c

clean-generic:
	@RM@ -f *~ .*~
