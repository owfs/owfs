SUBDIRS = ow
EXTRA_DIST = python.m4 \
	examples/check_ow.py examples/errormessages.py examples/raw_access.py examples/temperature.py examples/tree.py examples/xmlrpc_client.py examples/xmlrpc_server.py \
	unittest/Readme.txt unittest/ds1420.py unittest/ds2408.py unittest/ds2409.py unittest/owload.py unittest/owsensors.py unittest/owtest.py unittest/owtest_sample.ini unittest/owtest_think.ini unittest/util.py
noinst_DATA = OW.py
LIBOW = ../../owlib/src/c/libow.la

ow_wrap.c: ../ow.i ${LIBOW}
	$(SWIG) -python -o $@ ../ow.i

OW.py: ow_wrap.c setup.py $(LIBOW)
	CFLAGS="" $(PYTHON) setup.py build

install-data-local:
#	OpenSUSE is buggy and install libraries at /usr/local.
#	Need to add call "install_lib --install-dir" or call "install --install-lib"
#	$(PYTHON) setup.py install_lib --install-dir="/$(DESTDIR)$(PYSITEDIR)"
	$(PYTHON) setup.py install --install-lib="$(DESTDIR)$(PYSITEDIR)"
# Other options are (but not needed): --install-data="/$(DESTDIR)$(PYSITEDIR)" --install-script="/$(DESTDIR)$(PYSITEDIR)" --install-header=

clean-local:
	@RM@ -f configuration.log
	@RM@ -rf build OW.py ow_wrap.c

