## $Id$

EXTRA_DIST = setup.py Readme.txt Readme_pypi.txt examples/check_ow.py examples/temperatures.py ownet/connection.py ownet/__init__.py

install-data-local:
	$(PYTHON) setup.py install --root="/$(DESTDIR)"

clean-local:
	@RM@ -rf build

clean-generic:
	@RM@ -f *~ .*~
