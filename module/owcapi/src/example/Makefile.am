# $Id$

bin_PROGRAMS = owdir
owdir_SOURCES = owdir.c
owdir_DEPENDENCIES = ../c/libowcapi.la

AM_CFLAGS = -I../include \
         -I../../../owlib/src/include \
         -L../c \
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
         ${PROFILING_FLAG} \
         ${MT_FLAG}

LDADD = -lowcapi ${PROFILING_FLAG} ${PTHREAD_LIBS} ${LD_EXTRALIBS}

clean-generic:

	@RM@ -f *~ .*~
