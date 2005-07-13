# $Id$

bin_PROGRAMS = OW.pm
OW_pm_DEPENDENCIES = ../../owlib/src/c/libow.la

if ENABLE_CACHE
CACHE_FLAG = -DOW_CACHE
endif

if ENABLE_USB
USB_FLAG = -DOW_USB
endif

if ENABLE_MT
MT_FLAG = -DOW_MT
endif

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
         ${CACHE_FLAG} \
         ${MT_FLAG} \
         ${USB_FLAG}

AM_LDFLAGS = -low ${PTHREAD_LIBS}

#noinst_DATA = .made_mod

#.made_mod: owperl_wrap.c Makefile.perl
#	$(MAKE) -fMakefile.perl
#	touch $@

ow_wrap.c: ../ow.i Makefile.perl ${OW_pm_DEPENDENCIES}
	$(SWIG) -perl5 -o $@ ../ow.i

Makefile.perl: Makefile.PL
	$(PERL) $< MAKEFILE=$@

OW.pm: ow_wrap.c
	$(MAKE) -fMakefile.perl

install-data-local:
	$(MAKE) -fMakefile.perl PREFIX=$(prefix) install

#CLEANFILES = *~ .made_mod  owperl.c OW.pm
CLEANFILES = *~ *.o ow_wrap.c OW.pm *.PL Makefile.in

clean-local:
	if test -f Makefile.perl; then $(MAKE) -fMakefile.perl realclean; fi
	rm -f Makefile.perl Makefile.perl.old

clean-generic:

	@RM@ -f *~ .*~
