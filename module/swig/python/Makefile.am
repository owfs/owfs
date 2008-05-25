# $Id$

SUBDIRS = ow
EXTRA_DIST = python.m4 $(wildcard examples/*.py) $(wildcard unittest/*.py) $(wildcard unittest/*.ini) unittest/Readme.txt
noinst_DATA = OW.py
LIBOW = ../../owlib/src/c/libow.la

#owpython_LDFLAGS = -shared -shrext .so

#AM_CFLAGS = -I../include \
#	-fexceptions \
#	${PTHREAD_CFLAGS} \
#	${EXTRACFLAGS}

#LDADD = -low ${PTHREAD_LIBS} ${LD_EXTRALIBS} ${OSLIBS}

ow_wrap.c: ../ow.i ${LIBOW}
	$(SWIG) -python -o $@ ../ow.i

OW.py: ow_wrap.c setup.py
	$(PYTHON) setup.py build

install-data-local:
#	OpenSUSE is buggy and install libraries at /usr/local.
#	Need to add call "install_lib --install-dir" or call "install --install-lib"
#	$(PYTHON) setup.py install_lib --install-dir="/$(DESTDIR)$(PYSITEDIR)"
	$(PYTHON) setup.py install --install-lib="$(DESTDIR)$(PYSITEDIR)"
# Other options are (but not needed): --install-data="/$(DESTDIR)$(PYSITEDIR)" --install-script="/$(DESTDIR)$(PYSITEDIR)" --install-header=

clean-local:
	@RM@ -f *~ .*~ *.o
	@RM@ -rf build OW.py ow_wrap.c

clean-generic:
	@RM@ -f *~ .*~
