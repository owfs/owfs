## $Id$

if ENABLE_OWPHP
  SWIG_SUBDIRS += php
endif

if ENABLE_OWPYTHON
  SWIG_SUBDIRS += python
endif

#if ENABLE_OWPERL
#  SWIG_SUBDIRS += perl5
#endif

SUBDIRS = $(SWIG_SUBDIRS)

clean-generic:

	@RM@ -f *~ .*~
