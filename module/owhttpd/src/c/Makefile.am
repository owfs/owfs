# $Id$

bin_PROGRAMS = owhttpd
owhttpd_SOURCES = owhttpd.c owhttpd_handler.c
#owhttpd_LDADD = libow.la

CFLAGS = -I../include \
         -I../../../owlib/src/include \
         -L../../../owlib/src/c \
         -low \
         -ldb \
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
         -Wunreachable-code
                        
clean-generic:

	rm -f *~ .*~
