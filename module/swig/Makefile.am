if ENABLE_OWPERL
  MAYBE_OWPERL = perl5
endif

if ENABLE_OWPYTHON
  MAYBE_OWPYTHON = python
endif

if ENABLE_OWTCL
  MAYBE_OWTCL = tcl
endif

SUBDIRS = $(MAYBE_OWPERL) $(MAYBE_OWPYTHON) $(MAYBE_OWTCL)

clean-generic:

	rm -f *~ .*~
