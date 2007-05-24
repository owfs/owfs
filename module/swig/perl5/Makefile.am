## $Id$

EXTRA_DIST = OW/README OW/MANIFEST OW/Changes OW/Makefile.PL OW/t/OW.t $(wildcard example/*.pl)

all: OW/Makefile

OW/Makefile: OW/Makefile.PL
if HAVE_DARWIN
	( cd OW; cp Makefile.osx Makefile.PL; $(PERL) Makefile.PL )
else
if HAVE_DEBIAN
	( cd OW; cp Makefile.linux Makefile.PL; $(PERL) Makefile.PL PREFIX=@prefix@ )
else
	( cd OW; cp Makefile.linux Makefile.PL; $(PERL) Makefile.PL )
endif
endif

OW/ow_wrap.c: ../ow.i OW/Makefile ${LIBOW}
	$(SWIG) -perl5 -o $@ ../ow.i

OW/blib/lib/OW.pm: OW/ow_wrap.c
	( cd OW; $(MAKE) -fMakefile )
	@INSTALL@ -d OW/blib/lib
	@INSTALL@ OW/OW.pm OW/blib/lib/
	@ECHO@ >> OW/blib/lib/OW.pm 'our $$VERSION='\''@VERSION@'\'' ;'

install-data-local: OW/Makefile OW/ow_wrap.c OW/blib/lib/OW.pm
	( cd OW; $(MAKE) install DESTDIR="$(DESTDIR)" )

clean-generic:
	@RM@ -f *~ .*~
	if test -f OW/Makefile; then cd OW; $(MAKE) clean; fi
	@RM@ -f OW/Makefile.old OW/Makefile OW/OW.pm OW/OW.bs OW/ow_wrap.c

