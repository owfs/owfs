## man sources
MANFILES = \
    owfs.man
## .so includes
SOFILES = \
    description.5so seealso.5so
## man files that need no preprocessing
dist_man5_MANS = \
    owfs.conf.5
## file to include in distribution
EXTRA_DIST = $(SOFILES) $(MANFILES)


if SOELIM

man5_MANS = $(addsuffix .5,$(basename $(MANFILES)))
CLEANFILES = $(man5_MANS)

# preproc man pages via soelim
$(man5_MANS): $(MANFILES) $(SOFILES)
%.5 :: %.man
	$(SOELIM) -r -I $(srcdir)/.. $< > $@

else !SOELIM

man5_MANS = $(MANFILES) $(SOFILES)

endif !SOELIM
