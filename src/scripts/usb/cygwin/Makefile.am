
EXTRA_DIST = ds9490.inf ds9490.cat README.txt

if HAVE_CYGWIN
ds9490cygdriversdir = ${prefix}/drivers
ds9490cygdrivers_DATA = ds9490.inf ds9490.cat README.txt

install-data-local:
	@INSTALL@ -d ${DESTDIR}${bindir}
	-if test -f /bin/cygwin1.dll; then \
		@INSTALL@ /bin/cygwin1.dll ${DESTDIR}${bindir}/ ; \
	fi
	-if test -f /bin/cygusb0.dll; then \
		@INSTALL@ /bin/cygusb0.dll ${DESTDIR}${bindir}/ ; \
	fi
#	@INSTALL@ -d ${DESTDIR}${prefix}/drivers
#	@INSTALL@ ${EXTRA_DIST} ${DESTDIR}${prefix}/drivers/
endif

clean-generic:

	@RM@ -f *~ .*~

