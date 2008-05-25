## $Id$

EXTRA_DIST = setup.py MANIFEST.in Readme.txt Readme_pypi.txt $(wildcard examples/*.py) $(wildcard ownet/*.py)

install-data-local:
#	$(PYTHON) setup.py install --root="/$(DESTDIR)"
#	OpenSUSE is buggy and install libraries at /usr/local.
#	Calling install_lib instead, seems to work for all platforms.
	$(PYTHON) setup.py install_lib --install-dir="/$(DESTDIR)$(PYSITEDIR)"

clean-local:
	@RM@ -rf build

clean-generic:
	@RM@ -f *~ .*~
