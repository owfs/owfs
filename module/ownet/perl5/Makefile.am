## $Id$

EXTRA_DIST = OWNet/README OWNet/MANIFEST OWNet/Changes OWNet/Makefile.PL OWNet/lib/OWNet.pm OWNet/t/OWNet.t $(wildcard examples/*.pl)

all: OWNet/Makefile

OWNet/Makefile: OWNet/Makefile.PL
	( cd OWNet; $(PERL) Makefile.PL PREFIX=@prefix@ )

install-data-local: OWNet/Makefile
	( cd OWNet; $(MAKE) install DESTDIR="$(DESTDIR)" )
#	( cd OWNet; $(MAKE) ; $(MAKE) test; $(MAKE) install )

clean-generic:
	@RM@ -f *~ .*~
	if test -f OWNet/Makefile; then cd OWNet; $(MAKE) clean; fi
	@RM@ -f OWNet/Makefile.old OWNet/Makefile

