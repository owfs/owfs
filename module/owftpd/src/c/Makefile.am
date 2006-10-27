# $Id$

if HAVE_CYGWIN
if ENABLE_ZERO
DNSSDLIB = ../../../../src/scripts/windows/libdnssd.dll.a
endif
endif

bin_PROGRAMS = owftpd
owftpd_SOURCES = file_list.c \
                 file_cd.c    \
                 ftp_command.c \
                 ftp_listener.c \
                 ftp_session.c   \
                 owftpd.c         \
                 telnet_session.c  \
                 watchdog.c         \
                 daemon_assert.c
owftpd_DEPENDENCIES = ../../../owlib/src/c/libow.la $(DNSSDLIB)

if HAVE_NEW_SS_FAMILY
SS_FLAG = -DHAVE_NEW_SS_FAMILY
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
         -Wstrict-prototypes \
         -Wredundant-decls \
         ${PROFILING_FLAG} \
         ${SS_FLAG} \
         -D_FILE_OFFSET_BITS=64

#         -Wconversion \
#

LDADD = -low ${PROFILING_FLAG} ${PTHREAD_LIBS} ${LD_EXTRALIBS}

clean-generic:

	@RM@ -f *~ .*~
