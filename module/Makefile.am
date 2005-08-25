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

if ENABLE_OWNFSD
  MAYBE_OWNFSD = ownfsd
endif

if ENABLE_OWSERVER
  MAYBE_OWSERVER = owserver
endif

if ENABLE_SWIG
  MAYBE_SWIG = swig
endif

# VT: SUBDIRS: In *that* order (owlib first is the critical point)
SUBDIRS = owlib $(MAYBE_OWFS) $(MAYBE_OWHTTPD) $(MAYBE_OWFTPD) $(MAYBE_OWNFSD) $(MAYBE_OWSERVER) $(MAYBE_SWIG)

clean-generic:

	rm -f *~ .*~
