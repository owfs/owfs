systemdsystemunit_DATA = \
        owfs.service \
        owftpd.service \
        owhttpd.service \
        owserver.service \
        owserver.socket
CLEANFILES = $(systemdsystemunit_DATA)
EXTRA_DIST = \
        owfs.service.in \
        owftpd.service.in \
        owhttpd.service.in \
        owserver.service.in \
        owserver.socket.in

do_subst = sed -e 's,[@]bindir[@],$(bindir),g'

% :: %.in
	$(do_subst) < $(srcdir)/$< > $@

$(systemdsystemunit_DATA): Makefile
