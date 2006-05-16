# $Id$

bin_PROGRAMS = owftpd
owftpd_SOURCES = file_list.c \
                 ftp_command.c \
                 ftp_listener.c \
                 ftp_session.c \
                 owftpd.c \
                 telnet_session.c \
                 watchdog.c \
                 error.c \
                 daemon_assert.c
owftpd_DEPENDENCIES = ../../../owlib/src/c/libow.la

# multithreading -- must be available on the library as well
if ENABLE_MT
MT_FLAG = -DOW_MT
endif

if HAVE_NEW_SS_FAMILY
SS_FLAG = -DHAVE_NEW_SS_FAMILY
endif

if ENABLE_PROFILING
PROFILING_FLAG = -pg
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
         ${SS_FLAG} \
         ${PROFILING_FLAG} \
         -D_FILE_OFFSET_BITS=64

AM_LDFLAGS = -low ${PROFILING_FLAG} ${PTHREAD_LIBS} ${LD_EXTRALIBS}

clean-generic:

	@RM@ -f *~ .*~
