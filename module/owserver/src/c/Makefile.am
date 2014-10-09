if ENABLE_OWSERVER
  MODULE_OWSERVER = owserver
endif

if ENABLE_OWEXTERNAL
  MODULE_OWEXTERNAL = owexternal
endif

bin_PROGRAMS = $(MODULE_OWSERVER) $(MODULE_OWEXTERNAL)

owserver_SOURCES = owserver.c    \
                   from_client.c \
                   to_client.c   \
                   read.c        \
                   write.c       \
                   dir.c         \
                   dirall.c      \
                   dirallslash.c \
                   data.c        \
                   error.c       \
                   handler.c     \
                   loop.c        \
                   md5.c         \
                   ping.c

owserver_DEPENDENCIES = ../../../owlib/src/c/libow.la

owexternal_SOURCES = $(owserver_SOURCES)

owexternal_DEPENDENCIES = $(owserver_DEPENDENCIES)

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
	-Wstrict-prototypes \
	-Wredundant-decls \
	${EXTRACFLAGS} \
	${PTHREAD_CFLAGS} \
	${LIBUSB_CFLAGS}

LDADD = -low ${LIBUSB_LIBS} ${PTHREAD_LIBS} ${LD_EXTRALIBS} ${OSLIBS}

#if HAVE_CYGWIN
#NOWINE=1
#else
#DOS_OWFSROOT:=$(OWFSROOT)
#LINT_DIR:=$(DOS_OWFSROOT)/src/tools/lint
#LINT_CC:=wine $(LINT_DIR)/lint-nt.exe
#endif
#
#SRC:=$(owserver_SOURCES)
#include $(abs_top_srcdir)/src/tools/lint/lint.mk
#
#clean-generic:
#	@RM@ -f *~ .*~ *.lint *.lint.txt .#* *.bak
