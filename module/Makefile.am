## $Id$

if ENABLE_OWHTTPD
  MODULE_SUBDIRS += owhttpd
endif

if ENABLE_OWSERVER
  MODULE_SUBDIRS += owserver
endif

if ENABLE_OWFS
  MODULE_SUBDIRS += owfs
endif

if ENABLE_OWFTPD
  MODULE_SUBDIRS += owftpd
endif

if ENABLE_OWCAPI
  MODULE_SUBDIRS += owcapi
endif
  
if ENABLE_SWIG
  MODULE_SUBDIRS += swig
endif

SUBDIRS = owlib $(MODULE_SUBDIRS)

clean-generic:

	@RM@ -f *~ .*~
