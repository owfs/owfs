
EXTRA_DIST = ds9490.inf ds9490.cat README.txt

if HAVE_WINDOWS
ds9490windriversdir = ${prefix}/drivers
ds9490windrivers_DATA = ds9490.inf ds9490.cat README.txt

install-ds9490windrivers-local:
	@INSTALL@ -d ${DESTDIR}${prefix}/drivers
	@INSTALL@ ${EXTRA_DIST} ${DESTDIR}${prefix}/drivers/
endif

clean-generic:

	@RM@ -f *~ .*~

