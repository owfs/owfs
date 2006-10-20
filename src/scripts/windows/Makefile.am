
EXTRA_DIST = ds9490.inf ds9490.cat README.txt

if HAVE_CYGWIN
ds9490driversdir = ${prefix}/drivers
ds9490drivers_DATA = ds9490.inf ds9490.cat README.txt

install-ds9490drivers-local:
	@INSTALL@ -d ${DESTDIR}${prefix}/drivers
	@INSTALL@ ${EXTRA_DIST} ${DESTDIR}${prefix}/drivers/
endif

clean-generic:

	@RM@ -f *~ .*~

