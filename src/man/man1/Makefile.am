## man sources
MANFILES = \
    owcapi.man owfs.man owftpd.man owhttpd.man owmon.man ownet.man \
    owserver.man owshell.man owtap.man
## .so includes
SOFILES = \
    cmdline_mini.1so configuration.1so description.1so \
    device.1so format.1so help.1so job_control.1so \
    persistent_thresholds.1so pressure.1so seealso.1so temperature.1so \
    timeout.1so
## man files that need no preprocessing
dist_man1_MANS = \
    libowcapi.1 \
    owdir.1 owget.1 owread.1 owwrite.1 owpresent.1 \
    ownetapi.1 ownetlib.1 libownet.1
## file to include in distribution
EXTRA_DIST = $(SOFILES) $(MANFILES)


if SOELIM

man1_MANS = $(addsuffix .1,$(basename $(MANFILES)))
CLEANFILES = $(man1_MANS)

# preproc man pages via soelim
$(man1_MANS): $(MANFILES) $(SOFILES)
%.1 :: %.man
	$(SOELIM) -r -I $(srcdir)/.. $< > $@

else !SOELIM

man1_MANS = $(MANFILES) $(SOFILES)

endif !SOELIM
