# $Id$

bin_PROGRAMS = owhttpd
owhttpd_SOURCES = owhttpd.c owhttpd_handler.c
#owhttpd_LDADD = libow.la

AM_CFLAGS = -I../include \
         -I../../../owlib/src/include \
         -L../../../owlib/src/c \
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
         -Wredundant-decls

if ENABLE_CACHE
CACHE_FLAG = -ldb
endif
                        
AM_LDFLAGS = -low ${CACHE_FLAG}

clean-generic:

	rm -f *~ .*~
