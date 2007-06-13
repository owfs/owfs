## $Id$

EXTRA_DIST = perl5.m4 OW/META.yml OW/README OW/MANIFEST OW/Changes OW/Makefile.linux.in OW/Makefile.osx.in OW/t/OW.t $(wildcard example/*.pl)

#all: OW/Makefile
all: OW/blib/lib/OW.pm

OW/Makefile.PL:
if HAVE_DARWIN
	cp OW/Makefile.osx OW/Makefile.PL
else
	cp OW/Makefile.linux OW/Makefile.PL
endif

OW/Makefile: OW/Makefile.PL
if HAVE_DARWIN
	( cd OW; $(PERL) Makefile.PL )
else
if HAVE_DEBIAN
	( cd OW; $(PERL) Makefile.PL PREFIX=@prefix@ )
else
	( cd OW; $(PERL) Makefile.PL )
endif
endif

OW/ow_wrap.c: ../ow.i OW/Makefile ${LIBOW}
	$(SWIG) -perl5 -o $@ ../ow.i

OW/blib/lib/OW.pm: OW/ow_wrap.c
	$(MAKE) -C OW -fMakefile
#	( cd OW; $(MAKE) -fMakefile )
	@INSTALL@ -d OW/blib/lib
	@INSTALL@ OW/OW.pm OW/blib/lib/
	@ECHO@ >> OW/blib/lib/OW.pm 'our $$VERSION='\''@VERSION@'\'' ;'

install-data-local: OW/Makefile OW/ow_wrap.c OW/blib/lib/OW.pm
	$(MAKE) -C OW install DESTDIR="$(DESTDIR)"
#	( cd OW; $(MAKE) install DESTDIR="$(DESTDIR)" )

clean-generic:
	@RM@ -f *~ .*~
	if test -f OW/Makefile; then cd OW; $(MAKE) clean; fi
	@RM@ -f OW/Makefile.old OW/Makefile OW/Makefile.PL OW/OW.pm OW/OW.bs OW/ow_wrap.c

