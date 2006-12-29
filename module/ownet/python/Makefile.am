## $Id$

EXTRA_DIST = setup.py Readme.txt Readme_pypi.txt $(wildcard examples/*.py) $(wildcard ownet/*.py)

install-data-local:
	$(PYTHON) setup.py install --root="/$(DESTDIR)"

clean-local:
	@RM@ -rf build

clean-generic:
	@RM@ -f *~ .*~
