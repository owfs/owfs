# $Id$


EXTRA_DIST = perl5.m4 example/test.pl
noinst_DATA = OW.pm
LIBOW = ../../owlib/src/c/libow.la

AM_CFLAGS = -I../include \
         -fexceptions \
         -Wall \
         -W \
         -Wundef \
         -Wshadow \
         -Wpointer-arith \
         -Wcast-qual \
         -Wcast-align \
         -Wconversion \
         -Wstrict-prototypes \
         -Wredundant-decls \
         -DSKIP_SEARCH_H \
         -D_FILE_OFFSET_BITS=64 \
         ${MT_FLAG}

LDADD = -low ${PTHREAD_LIBS}

Makefile.perl: Makefile.PL
	$(PERL) $< MAKEFILE=$@

ow_wrap.c: ../ow.i Makefile.perl ${LIBOW}
	$(SWIG) -perl5 -o $@ ../ow.i

OW.pm: ow_wrap.c
	$(MAKE) -fMakefile.perl

PERLSITEDIR = $(wildcard ${DESTDIR}/usr/lib*/perl5/site_perl/*/*-linux-thread-*/auto/ )

install-data-local:
	$(MAKE) -fMakefile.perl PREFIX=$(prefix) install
	@INSTALL@ -d ${DESTDIR}${prefix}/bin/
	@INSTALL@ OW.pm ${DESTDIR}${prefix}/bin/
	if test -d "${PERLSITEDIR}"; then @INSTALL@ OW.pm "${PERLSITEDIR}/../"; fi

clean-local:
	if test -f Makefile.perl; then $(MAKE) -fMakefile.perl realclean; fi
	@RM@ -f Makefile.perl Makefile.perl.old
	@RM@ -f *~ .*~ *.o ow_wrap.c OW.pm

clean-generic:

	@RM@ -f *~ .*~
