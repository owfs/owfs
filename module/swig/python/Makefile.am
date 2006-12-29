# $Id$

SUBDIRS = ow
EXTRA_DIST = python.m4 $(wildcard examples/*.py) $(wildcard unittest/*.py) $(wildcard unittest/*.ini) unittest/Readme.txt
noinst_DATA = OW.py
LIBOW = ../../owlib/src/c/libow.la

#owpython_LDFLAGS = -shared -shrext .so

AM_CFLAGS = -I../include \
         -fexceptions \
         -D_FILE_OFFSET_BITS=64

LDADD = -low ${PTHREAD_LIBS} ${LD_EXTRALIBS}

ow_wrap.c: ../ow.i ${LIBOW}
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
