## $Id$

if ENABLE_OWFS
  MAYBE_OWFS = owfs
endif
if ENABLE_OWHTTPD
  MAYBE_OWHTTPD = owhttpd
endif
if ENABLE_OWFTPD
  MAYBE_OWFTPD = owftpd
endif
if ENABLE_OWPERL
  MAYBE_OWPERL = owperl
endif

# VT: SUBDIRS: In *that* order
SUBDIRS = owlib $(MAYBE_OWPERL) $(MAYBE_OWFS) $(MAYBE_OWHTTPD) $(MAYBE_OWFTPD)

clean-generic:

	rm -f *~ .*~
