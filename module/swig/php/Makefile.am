# $Id$

bin_PROGRAMS = OW.pm

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
         -D_FILE_OFFSET_BITS=64 \
         ${CACHE_FLAG} \
         ${MT_FLAG} \
         ${USB_FLAG}

#noinst_DATA = .made_mod

#.made_mod: owperl_wrap.c Makefile.perl
#	$(MAKE) -fMakefile.perl
#	touch $@

ow_wrap.c: ../ow.i
	$(SWIG) -php4 phpfull -o $@ ../ow.i

#CLEANFILES = *~ .made_mod  owperl.c OW.pm
CLEANFILES = *~ *.o ow_wrap.c OW.*

clean-local:

clean-generic:

	@RM@ -f *~ .*~
