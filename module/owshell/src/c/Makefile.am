COMMON_OWSHELL_SOURCE = ow_opt.c   \
               ow_help.c    \
               ow_server.c  \
               ow_net.c     \
               ow_browse.c  \
               ow_dl.c      \
               ow_dnssd.c   \
               ow_tcp_read.c\
               getaddrinfo.c\
               getopt.c     \
               globals.c

bin_PROGRAMS = owget owdir owread owwrite owpresent owexist owusbprobe
owget_SOURCES = ${COMMON_OWSHELL_SOURCE} \
               owget.c

owdir_SOURCES = ${COMMON_OWSHELL_SOURCE} \
               owdir.c

owread_SOURCES = ${COMMON_OWSHELL_SOURCE} \
               owread.c

owwrite_SOURCES = ${COMMON_OWSHELL_SOURCE} \
               owwrite.c

owpresent_SOURCES = ${COMMON_OWSHELL_SOURCE} \
               owpresent.c

owexist_SOURCES = ${COMMON_OWSHELL_SOURCE} \
               owexist.c

owusbprobe_SOURCES = ${COMMON_OWSHELL_SOURCE} \
               owusbprobe.c
owusbprobe_LDFLAGS = ${LIBUSB_LIBS}

AM_CFLAGS = -I../include \
	-I../../../owlib/src/include \
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
	${LIBUSB_CFLAGS} \
	${PTHREAD_CFLAGS} \
	${PIC_FLAGS} \
	${EXTRACFLAGS}

LDADD = ${DL_LIBS} ${LD_EXTRALIBS} ${OSLIBS}

