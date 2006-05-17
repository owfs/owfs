# $Id$

bin_PROGRAMS = owfs
owfs_SOURCES = owfs.c owfs_callback.c fuse_line.c
owfs_DEPENDENCIES = ../../../owlib/src/c/libow.la

if ENABLE_MT
MT_FLAG = -DOW_MT
endif

if ENABLE_PROFILING
PROFILING_FLAG = -pg
endif

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
         ${MT_FLAG} \
         ${PROFILING_FLAG}


AM_LDFLAGS = -low $(FUSE_LIBS) ${PROFILING_FLAG} ${PTHREAD_LIBS} ${LD_EXTRALIBS}

clean-generic:

	@RM@ -f *~ .*~
