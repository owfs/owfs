
if HAVE_SYSTEMD
systemdsystemunit_DATA = \
	owfs.service \
	owftpd.service \
	owhttpd.service \
	owserver.service \
	owserver.socket

$(systemdsystemunit_DATA): Makefile
	rm -f $@ $@.tmp
	sourcedir=''; \
		test -f ./$@.in || sourcedir=$(sourcedir)/; \
		sed -e 's|@bindir[@]|$(bindir)|g' $${sourcedir}$@.in >$@.tmp
	mv $@.tmp $@
endif
