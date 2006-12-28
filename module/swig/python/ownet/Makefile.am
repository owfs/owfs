
EXTRA_DIST = MANIFEST.in Readme.txt Readme_pypi.txt setup.py examples/check_ow.py examples/temperatures.py

# fix this
#all:

build: setup.py
	$(PYTHON) setup.py build

#install-data-local:
#	$(PYTHON) setup.py install --root="/$(DESTDIR)"

clean-local:
	@RM@ -rf build

clean-generic:

	@RM@ -f *~ .*~
