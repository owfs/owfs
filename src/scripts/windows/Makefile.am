
EXTRA_DIST = dnssd.dll dllextract.sed libdns_sd.dll

if HAVE_CYGWIN
install-data-local:
	@INSTALL@ -d ${DESTDIR}${libdir}
	@INSTALL@ libdns_sd.dll ${DESTDIR}${libdir}

#all:	libdnssd.dll.a

#libdnssd.dll.a:	dnssd.lib dnssd.def
#	dlltool --export-all-symbols --dllname dnssd.dll --input-def dnssd.def --output-lib libdnssd.dll.a

#dnssd.def: dllextract.sed dnssd.dll
#	echo "LIBRARY dnssd.dll" > $@
#	echo EXPORTS >> $@
#	objdump -p dnssd.dll | sed -nf dllextract.sed >> $@
endif

clean-generic:

	@RM@ -f *~ .*~ libdnssd.dll.a dnssd.def
