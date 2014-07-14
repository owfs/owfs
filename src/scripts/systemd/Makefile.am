
if HAVE_SYSTEMD
systemdsystemunit_DATA = \
       owfs.service \
       owftpd.service \
       owhttpd.service \
       owserver.service

$(systemdsystemunit_DATA): Makefile
       rm -f $@ $@.tmp
       srcdir=''; \
               test -f ./$@.in || srcdir=$(srcdir)/; \
               sed -e 's|@bindir[@]|$(bindir)|g' $${srcdir}$@.in >$@.tmp
       mv $@.tmp $@
endif
