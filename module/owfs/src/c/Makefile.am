# $Id$

bin_PROGRAMS = owfs
owfs_SOURCES = owfs.c owfs_callback.c

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
         -Wredundant-decls
                        
clean-generic:

	rm -f *~ .*~
