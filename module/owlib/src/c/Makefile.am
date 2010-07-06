# $Id$

OWLIB_SOURCE = ow_dl.c            \
               ow_dnssd.c         \
               ow_alias.c         \
	           ow_arg.c           \
	           ow_avahi_announce.c\
               ow_avahi_browse.c  \
               ow_avahi_link.c    \
               ow_baud.c          \
               ow_browse.c        \
               ow_browse_resolve.c\
               ow_browse_monitor.c\
               ow_buslock.c       \
               compat.c           \
               getaddrinfo.c      \
               getline.c          \
               getopt.c           \
               ow_none.c          \
               ow_1820.c          \
               ow_1821.c          \
               ow_1921.c          \
               ow_1923.c          \
               ow_1954.c          \
               ow_1963.c          \
               ow_1977.c          \
               ow_1991.c          \
               ow_1993.c          \
               ow_2401.c          \
               ow_2404.c          \
               ow_2405.c          \
               ow_2406.c          \
               ow_2408.c          \
               ow_2409.c          \
               ow_2413.c          \
               ow_2415.c          \
               ow_2423.c          \
               ow_2430.c          \
               ow_2433.c          \
               ow_2436.c          \
               ow_2438.c          \
               ow_2450.c          \
               ow_2502.c          \
               ow_2505.c          \
               ow_2760.c          \
               ow_2804.c          \
               ow_2890.c          \
               ow_alloc.c         \
               ow_api.c           \
               ow_badadapter.c    \
               ow_bae.c           \
               ow_bus.c           \
               ow_cache.c         \
               ow_charblob.c      \
               ow_com.c           \
               ow_com_write.c     \
               ow_com_read.c      \
               ow_connect.c       \
               ow_crc.c           \
               ow_daemon.c        \
               ow_date.c          \
               ow_delay.c         \
               ow_devicelock.c    \
               ow_dir.c           \
               ow_dirblob.c       \
               ow_ds2482.c        \
               ow_ds9097.c        \
               ow_ds1410.c        \
               ow_ds9097U.c       \
               ow_ds9490.c        \
               ow_eds.c           \
               ow_eeef.c          \
               ow_eprom_write.c   \
               ow_etherweather.c  \
               ow_fs_address.c    \
               ow_fs_alias.c      \
               ow_fs_code.c       \
               ow_fs_crc8.c       \
               ow_fs_id.c         \
               ow_fs_type.c       \
               ow_ha5.c           \
               ow_ha7.c           \
               ow_ha7e.c          \
               ow_fake.c          \
               ow_fakeread.c      \
               ow_filelength.c    \
               ow_fstat.c         \
               ow_generic_read.c  \
               ow_get.c           \
               ow_help.c          \
               ow_interface.c     \
               ow_iterate.c       \
               ow_lcd.c           \
               ow_lib_close.c     \
               ow_lib_setup.c     \
               ow_lib_stop.c      \
               ow_link.c          \
               ow_locator.c       \
               ow_locks.c         \
               ow_max_delay.c     \
               ow_memblob.c       \
               ow_memory.c        \
               ow_multicast.c     \
               ow_name.c          \
               ow_net_client.c    \
               ow_net_server.c    \
	       ow_offset.c        \
               ow_opt.c           \
               ow_parse_address.c \
               ow_parseinput.c    \
               ow_parsename.c     \
               ow_parseobject.c   \
               ow_parseoutput.c   \
               ow_parseshallow.c  \
               ow_parse_sn.c      \
               ow_pid.c           \
	       ow_powerbyte.c     \
               ow_presence.c      \
               ow_pressure.c      \
               ow_printparse.c    \
	       ow_programpulse.c  \
               ow_read.c          \
               ow_read_telnet.c   \
               ow_reconnect.c     \
	       ow_remote_alias.c  \
	       ow_reset.c         \
               ow_rwlock.c        \
               ow_stats.c         \
               ow_search.c        \
               ow_server.c        \
               ow_server_message.c\
	       ow_server_enet.c   \
               ow_select.c        \
               ow_settings.c      \
               ow_sibling.c       \
               ow_sibling_binary.c\
               ow_sibling_float.c \
               ow_sibling_uint.c  \
               ow_sibling_yesno.c \
               ow_sig_handlers.c  \
               ow_simultaneous.c  \
               ow_slurp.c         \
	       ow_specialcase.c   \
               ow_stateinfo.c     \
               ow_system.c        \
               ow_tcp_read.c      \
               ow_temp.c          \
               ow_testerread.c    \
               ow_thermocouple.c  \
               ow_tree.c          \
               ow_transaction.c   \
               ow_udp_read.c      \
               ow_usb_msg.c     \
               ow_usb_cycle.c     \
               ow_usb_monitor.c   \
               ow_util.c          \
               ow_verify.c        \
	       ow_visibility.c    \
               ow_w1.c            \
               ow_w1_addremove.c  \
               ow_w1_bind.c       \
               ow_w1_browse.c     \
               ow_w1_dispatch.c   \
               ow_w1_list.c       \
               ow_w1_monitor.c    \
               ow_w1_parse.c      \
               ow_w1_print.c      \
               ow_w1_scan.c       \
               ow_w1_select.c     \
               ow_w1_send.c       \
               ow_write.c         \
               ow_zero.c          \
               owlib.c            \
               error.c            \
               globals.c

lib_LTLIBRARIES = libow.la

libow_la_SOURCES = ${OWLIB_SOURCE}


if HAVE_CYGWIN
libow_la_LDFLAGS = ${PTHREAD_LIBS} -shared -no-undefined ${LIBUSB_LIBS} ${LD_EXTRALIBS}
else
libow_la_LDFLAGS = -version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE) -release $(LT_RELEASE) ${PTHREAD_LIBS} -shared -shrext .so ${LIBUSB_LIBS} ${LD_EXTRALIBS}
endif

# Maybe need this for MacOS X
#if HAVE_DARWIN
#LDADDS = -framework IOKit -framework CoreFoundation
#endif
#libow_la_LDFLAGS = -shared -shrext .so \
#	-version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE) \
#	-release $(LT_RELEASE) \
#	-export-dynamic \
#	$(LDADDS)

AM_CFLAGS = -I../include \
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
	-D__EXTENSIONS__ \
	${EXTRACFLAGS} \
	${PTHREAD_CFLAGS} \
	${LIBUSB_CFLAGS} \
	${PIC_FLAGS}

clean-generic:

	@RM@ -f *~ .*~

