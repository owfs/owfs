SUBDIRS = src module
RPMMACROS = ${HOME}/.rpmmacros
RPMDIR = ${HOME}/.rpm

EXTRA_DIST = bootstrap acx_pthread.m4

clean-generic:

	@RM@ -f *~ .*~

distclean-generic:

	@RM@ -rf auto*cache

preparerpm: dist

	@ if @TEST@ -z "@RPMBUILD@" ; then \
	    @ECHO@ "RPMBUILD binary not found, can't build RPM package"; \
	    exit 1; \
	fi
	if @TEST@ ! -f ${RPMMACROS} ; then \
	    @ECHO@ "%_topdir ${RPMDIR}" > ${RPMMACROS}; \
	    mkdir -p ${RPMDIR}/SOURCES \
	             ${RPMDIR}/SPECS \
	             ${RPMDIR}/BUILD \
	             ${RPMDIR}/RPMS/i386 \
	             ${RPMDIR}/SRPMS ; \
	fi
	cd src/rpm && ${MAKE} @PACKAGE@.spec
	@LN_S@ -f `pwd`/src/rpm/@PACKAGE@.spec ${RPMDIR}/SPECS/@PACKAGE@.spec

rpm: preparerpm
	@LN_S@ -f `pwd`/@PACKAGE@-@VERSION@.tar.gz ${RPMDIR}/SOURCES/@PACKAGE@-@VERSION@.tar.gz
	cd ${RPMDIR}/SPECS && @RPMBUILD@ -ba @PACKAGE@.spec

rpmcvs: preparerpm
	@LN_S@ -f `pwd`/@PACKAGE@-@VERSION@.tar.gz ${RPMDIR}/SOURCES/@PACKAGE@-@VERSION@_cvs_`date +"%Y%m%d"`.tar.gz
	cd ${RPMDIR}/SPECS && @RPMBUILD@ -ba @PACKAGE@.spec --define 'cvs 1'

