## $Id$

EXTRA_DIST = OWNet/README OWNet/MANIFEST OWNet/Changes OWNet/Makefile.PL OWNet/lib/OWNet.pm OWNet/t/OWNet.t examples/test.pl examples/test2.pl

all: OWNet/Makefile
	cd OWNet; $(MAKE) all

# Can't really install the perl-modules under prefix-directory.. it should be site-global.
OWNet/Makefile: OWNet/Makefile.PL
if HAVE_DEBIAN
	cd OWNet; $(PERL) Makefile.PL INSTALLDIRS=vendor
else
	cd OWNet; $(PERL) Makefile.PL
endif

install-data-local: OWNet/Makefile
if HAVE_DEBIAN
	(cd OWNet && $(MAKE) install_vendor DESTDIR="$(DESTDIR)")
else
	(cd OWNet && $(MAKE) install DESTDIR="$(DESTDIR)")
endif
#	$(MAKE) -C OWNet install DESTDIR="$(DESTDIR)"
#	( cd OWNet; $(MAKE) ; $(MAKE) test; $(MAKE) install )

clean-generic:
	@RM@ -f *~ .*~
	if test -f OWNet/Makefile; then cd OWNet; $(MAKE) clean; fi
	@RM@ -f OWNet/Makefile.old OWNet/Makefile

