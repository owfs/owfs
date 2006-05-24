# $Id$

bin_PROGRAMS = owfs
owfs_SOURCES = owfs.c owfs_callback.c fuse_line.c
owfs_DEPENDENCIES = ../../../owlib/src/c/libow.la

AM_CFLAGS = -I../include \
         -D_FILE_OFFSET_BITS=64 \
         ${FUSE_FLAGS} \
         ${FUSE_INCLUDES} \
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
         ${PROFILING_FLAG} \
         ${MT_FLAG}

LDADD = -low $(FUSE_LIBS) ${PROFILING_FLAG} ${PTHREAD_LIBS} ${LD_EXTRALIBS}

clean-generic:

	@RM@ -f *~ .*~
