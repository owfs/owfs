## man sources
MANFILES = \
    owtcl.man
## .so includes
SOFILES = \
    description.nso seealso.nso
## man files that need no preprocessing
dist_mann_MANS = \
    ow.n
## file to include in distribution
EXTRA_DIST = $(SOFILES) $(MANFILES)


if SOELIM

mann_MANS = $(addsuffix .n,$(basename $(MANFILES)))
CLEANFILES = $(mann_MANS)

# preproc man pages via soelim
$(mann_MANS): $(MANFILES) $(SOFILES)
%.n :: %.man
	$(SOELIM) -r -I $(srcdir)/.. $< > $@

else !SOELIM

mann_MANS = $(MANFILES) $(SOFILES)

endif !SOELIM
