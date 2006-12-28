## $Id$

install-data-local:
	$(PYTHON) setup.py install --root="/$(DESTDIR)"

CLEANFILES = *~ .*~ *.o

clean-local:
	@RM@ -f $(CLEANFILES)
	@RM@ -rf build

clean-generic:
	@RM@ -f *~ .*~
