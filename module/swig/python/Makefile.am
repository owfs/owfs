# $Id$

if ENABLE_CACHE
CACHE_FLAG = -DOW_CACHE
endif

if ENABLE_USB
USB_FLAG = -DOW_USB
endif

if ENABLE_MT
MT_FLAG = -DOW_MT
endif

AM_CFLAGS = -I../../owlib/src/include \
         -I../../../src/include \
         -I/usr/local/include/fuse \
         -I/usr/local/include \
         -L../../owlib/src/c \
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

AM_LDFLAGS = -low

ow_wrap.c: ../ow.i
	$(SWIG) -python -o $@ ../ow.i


clean-generic:

	@RM@ -f *~ .*~
