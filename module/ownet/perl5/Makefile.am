## $Id$

EXTRA_DIST = $(wildcard examples/*.pl)
noinst_DATA = OWNet.pm

PERLSITEDIR = $(wildcard ${DESTDIR}/usr/lib*/perl5/site_perl/*/*-linux-thread-*/auto/ )

install-data-local:
	@INSTALL@ OWNet.pm ${DESTDIR}${prefix}/bin/
	if test -d "${PERLSITEDIR}"; then @INSTALL@ OWNet.pm "${PERLSITEDIR}/../"; fi


clean-generic:
	@RM@ -f *~ .*~
