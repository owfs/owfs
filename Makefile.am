SUBDIRS = src

clean-generic:

	rm -f *~ .*~

distclean-generic:

	rm -rf auto*cache

rpm: dist

	@ if @TEST@ -z "@RPMBUILD@" ; then \
	    @ECHO@ "RPMBUILD binary not found, can't build RPM package"; \
	    exit 1; \
	fi
	cd src/rpm && ${MAKE} @PACKAGE@.spec
	@LN_S@ -f `pwd`/src/rpm/@PACKAGE@.spec /usr/src/redhat/SPECS/@PACKAGE@.spec
	@LN_S@ -f `pwd`/@PACKAGE@-@VERSION@.tar.gz /usr/src/redhat/SOURCES/@PACKAGE@-@VERSION@.tar.gz
	cd /usr/src/redhat/SPECS && @RPMBUILD@ -ba @PACKAGE@.spec
