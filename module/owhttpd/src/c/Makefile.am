# $Id$

bin_PROGRAMS = owserver
owhttpd_SOURCES = owserver.c
#owhttpd_LDADD = libow.la

# multithreading -- must be available on the library as well
if ENABLE_MT
MT_FLAG = -DOW_MT
endif

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
         -Wredundant-decls \
         ${MT_FLAG} \
         -D_FILE_OFFSET_BITS=64

AM_LDFLAGS = -low

clean-generic:

    rm -f *~ .*~
