## $Id$

#EXTRA_DIST = OW/README OW/MANIFEST OW/Changes OW/Makefile.PL OW/lib/OW.pm OW/t/OW.t $(wildcard example/*.pl)
EXTRA_DIST = OW/Makefile.linux OW/Makefile.osx OW/OW.pm $(wildcard example/*.pl)

all: OW/OW.pm

OW/Makefile:
if HAVE_DARWIN
	( cd OW; cp Makefile.osx Makefile.PL; $(PERL) Makefile.PL )
else
	( cd OW; cp Makefile.linux Makefile.PL; $(PERL) Makefile.PL )
endif

#OW/Makefile:
#	if test $(HAVE_DARWIN); then \
#		( cd OW; cp Makefile.osx Makefile.PL; $(PERL) Makefile.PL; ) \
#	else \
#		( cd OW; cp Makefile.linux Makefile.PL; $(PERL) Makefile.PL ) \
#	fi

OW/ow_wrap.c: ../ow.i OW/Makefile ${LIBOW}
	$(SWIG) -perl5 -o $@ ../ow.i

OW/OW.pm: OW/ow_wrap.c
	( cd OW; $(MAKE) -fMakefile )

PERLSITEDIR = $(wildcard ${DESTDIR}/usr/lib*/perl5/site_perl/*/*-linux-thread-*/auto/ )

install-data-local: OW/Makefile OW/ow_wrap.c OW/OW.pm
	( cd OW; $(MAKE) install DESTDIR="$(DESTDIR)" )
#	( cd OW; $(MAKE) -fMakefile.perl PREFIX=$(prefix) DESTDIR="$(DESTDIR)" install )
#	( cd OW; $(MAKE) ; $(MAKE) test; $(MAKE) install )
	@INSTALL@ -d ${DESTDIR}${prefix}/bin/
	@INSTALL@ OW/OW.pm ${DESTDIR}${prefix}/bin/
	if test -d "${PERLSITEDIR}"; then @INSTALL@ OW/OW.pm "${PERLSITEDIR}/../"; fi

clean-generic:
	@RM@ -f *~ .*~
	if test -f OW/Makefile; then cd OW; $(MAKE) clean; fi
	@RM@ -f OW/Makefile.old OW/ow_wrap.c OW/OW.pm OW/Makefile OW/*~
