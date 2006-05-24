
EXTRA_DIST = ow.i

if ENABLE_OWPERL
  SWIG_SUBDIRS += perl5
endif

if ENABLE_OWPYTHON
  SWIG_SUBDIRS += python
endif

if ENABLE_OWTCL
  SWIG_SUBDIRS += tcl
endif

if ENABLE_OWPHP
  SWIG_SUBDIRS += php
endif

SUBDIRS = $(SWIG_SUBDIRS)

clean-generic:

	@RM@ -f *~ .*~
