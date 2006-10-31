
EXTRA_DIST = libdns_sd.dll

if HAVE_CYGWIN

install-data-local:
	@INSTALL@ -d -m0755 ${DESTDIR}${libdir}
	@INSTALL@ libdns_sd.dll ${DESTDIR}${libdir}

endif

clean-generic:

	@RM@ -f *~ .*~ libdnssd.dll.a dnssd.def
