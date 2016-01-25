/*
	OWFS -- One-Wire filesystem
	ow_ftdi: low-level communication routines for FTDI-based adapters

	Copyright 2014-2016 Johan Ström <johan@stromnet.se>
	Released under GPLv2 for inclusion in the OWFS project.

	This file implements a serial layer for use with FTDI-based USB serial
	adapters. It should be usable as an in-place replacement for any ports
	with type ct_serial, given that the actual device can be reached through
	a FTDI chip (which is accessible).

	The main benifit of using this rather than accessing through a regular
	ttyUSB/cuaU device is that it will tweak the FTDI chip's latency timer.
	Since 1W transactions often use single character reads, the default setting
	inflicts a 16ms delay on every single read operation. With this layer
	we bring that down to (max) 1ms.

	This is (mostly) called from ow_com* routines, when the underlying
	port_in type is ct_ftdi.

	Please note that this is only a serial layer, for interacting with FTDI
	serial adapter chips. Generally, there is no way to know WHAT is conneted
	a given chip. Thus, ut is *not* possible to write a "scan for any FTDI
	1wire device" feature, unless the device has a unique VID/PID.

	The owftdi_open method assumes that the device arg is prefixed with "ftdi:".

	The following args can be used to identify a specific port:

		ftdi:d:<devicenode>
			path of bus and device-node (e.g. "003/001") within usb device tree
			(usually at /proc/bus/usb/)
		ftdi:i:<vendor>:<product>
			first device with given vendor and product id, ids can be decimal, octal
			(preceded by "0") or hex (preceded by "0x")
		ftdi:i:<vendor>:<product>:<index>
			as above with index being the number of the device (starting with 0)
			if there are more than one
		ftdi:s:<vendor>:<product>:<serial>
			first device with given vendor id, product id and serial string

	The strings are passed to libftdi as a "descriptor".
	An additional format is supported, for certain bus types.

		ftdi:<serial>

	This only works if there is a pre-defined VID/PID for the bus type used.
	Currently, this is only valid for the VID/PID found on the LinkUSB (i.e. bus_link).
	Note that those VID/PID's are the default for any FT232R device, and in no way exclusive
	to LinkUSB.
*/

#include <config.h>
#include "owfs_config.h"
#include "ow_connection.h"

#if OW_FTDI
//#define BENCH

#define FTDI_LINKUSB_VID 0x0403
#define FTDI_LINKUSB_PID 0x6001

#include <ftdi.h>
#include <assert.h>
#include <inttypes.h>

#include "ow_ftdi.h"

#define FTDIC(in) ((in)->master.link.ftdic)
static GOOD_OR_BAD owftdi_open_device(struct connection_in *in, const char *description) ;
static GOOD_OR_BAD owftdi_open_device_specific(struct connection_in *in, int vid, int pid, const char *serial) ;
static GOOD_OR_BAD owftdi_opened(struct connection_in *in) ;

#define owftdi_configure_baud(in) owftdi_configure_baud0(in, COM_BaudRate((in)->pown->baud))
static GOOD_OR_BAD owftdi_configure_baud0(struct connection_in *in, int baud) ;
static GOOD_OR_BAD owftdi_configure_bits(struct connection_in *in, enum ftdi_break_type break_) ;
static GOOD_OR_BAD owftdi_configure_dtrrts(struct connection_in *in, int value) ;

GOOD_OR_BAD owftdi_open( struct connection_in * in ) {
	GOOD_OR_BAD gbResult = gbBAD;
	const char* arg = in->pown->init_data;
	const char* target = arg+5;
	assert(arg);
	assert(strstr(arg, "ftdi:") == arg);

	if(strlen(target) < 2) {
		LEVEL_DEFAULT("Invalid ftdi target string");
		return gbBAD ;
	}

	char c0 = target[0];
	char c1 = target[1];
	const char* serial = NULL;
	if(c0 != 'd' && c0 != 'i' && c0 != 's' && c1 != ':') {
		// Assume plain serial; only works with known device-types
		serial = target;
	}

	if(FTDIC(in)) {
		LEVEL_DEFAULT("FTDI interface %s was NOT closed?",
				DEVICENAME(in));
		return gbBAD ;
	}

	if(!(FTDIC(in) = ftdi_new())) {
		LEVEL_DEFAULT("Failed to get FTDI context");
		return gbBAD ;
	}

	if(serial) {
		switch(in->pown->busmode) {
			case bus_link:
				gbResult = owftdi_open_device_specific(in,
						FTDI_LINKUSB_VID, FTDI_LINKUSB_PID,
						serial);

				if(GOOD(gbResult))
					in->adapter_name = "LinkUSB";

				break;
			default:
				LEVEL_DEFAULT(
						"Cannot use ftdi:<serial> with this device (%d), please use full format",
						in->pown->busmode);
				gbResult = gbBAD;
				break;
		}
	}else{
		gbResult = owftdi_open_device(in, target);
	}

	if(GOOD(gbResult)) {
		gbResult = owftdi_change(in);
	}

	if(BAD(gbResult)) {
		ftdi_free(FTDIC(in));
		FTDIC(in) = NULL;
	}
	else {
		in->pown->state = cs_deflowered;
	}



	return gbResult;
}

void owftdi_free( struct connection_in *in ) {
	if(!FTDIC(in)) {
		assert(in->pown->file_descriptor == FILE_DESCRIPTOR_BAD);
		return ;
	}

	// Best effort, port might not be open, only context
	ftdi_usb_close(FTDIC(in));

	ftdi_free(FTDIC(in));
	FTDIC(in) = NULL;

	in->pown->state = cs_virgin;
	// TODO: Cleaner?
	in->pown->file_descriptor = FILE_DESCRIPTOR_BAD;
}

void owftdi_close( struct connection_in * in ) {
	owftdi_free(in);
}

void owftdi_flush( const struct connection_in *in ) {
	if(!FTDIC(in)) {
		LEVEL_DEFAULT("Cannot flush FTDI interface %s, not open", DEVICENAME(in));
		return;
	}

	ftdi_usb_purge_tx_buffer(FTDIC(in));
	ftdi_usb_purge_rx_buffer(FTDIC(in));
}


void owftdi_break ( struct connection_in *in ) {
	assert (FTDIC(in));

	/* tcsendbreak manpage:
	 * 	linux: 0.25-0.5s
	 * 	freebsd: "four tenths of a second"
	 *
	 * break for 0.4s here as well
	 */
	LEVEL_DEBUG("Sending FTDI break..");
	owftdi_configure_bits(in, BREAK_ON);
	usleep(400000);
	owftdi_configure_bits(in, BREAK_OFF);
}


static GOOD_OR_BAD owftdi_open_device(struct connection_in *in, const char *description) {
	int ret = ftdi_usb_open_string(FTDIC(in), description);
	if(ret != 0) {
		ERROR_CONNECT("Failed to open FTDI device with description '%s': %d = %s",
				description, ret, ftdi_get_error_string(FTDIC(in)));
		return ret;
	}

	return owftdi_opened(in);
}

static GOOD_OR_BAD owftdi_open_device_specific(struct connection_in *in, int vid, int pid, const char *serial) {
	int ret = ftdi_usb_open_desc(FTDIC(in), vid, pid, NULL, serial);
	if(ret != 0) {
		ERROR_CONNECT("Failed to open FTDI device with vid/pid 0x%x/0%x and serial '%s': %d = %s",
				vid, pid, serial, ret, ftdi_get_error_string(FTDIC(in)));
		return ret;
	}

	return owftdi_opened(in);
}

static GOOD_OR_BAD owftdi_opened(struct connection_in *in) {
	int ret;

	/* Set DTR and RTS high. This is normally done by the OS when using the
	 * generic serial interface. We must do it manually, or we will not be able to
	 * communicate with the LinkUSB after boot/re-plug.
	 *
	 * Unfortunately, this can not be used to power-cycle the LinkUSB, it seems..
	 */
	RETURN_BAD_IF_BAD(owftdi_configure_dtrrts(in, 1));

	// Set latency timer to 1ms; improves response speed alot
	if((ret = ftdi_set_latency_timer(FTDIC(in), 1)) != 0) {
		ERROR_CONNECT("Failed to set FTDI latency timer: %d = %s",
				ret,
				ftdi_get_error_string(FTDIC(in)));
		return gbBAD;
	}

	// Fake a file_descriptor, so we can pass FILE_DESCRIPTOR_NOT_VALID check in
	// COM_read, COM_test
	// TODO: Cleaner?
	in->pown->file_descriptor = 999;

	return gbGOOD;
}

static GOOD_OR_BAD owftdi_configure_baud0(struct connection_in *in, int baud) {
	int ret;
	if((ret = ftdi_set_baudrate(FTDIC(in), baud)) != 0) {
		ERROR_CONNECT("Failed to set FTDI baud rate to %d: %d = %s",
				baud,
				ret,
				ftdi_get_error_string(FTDIC(in)));
		return gbBAD;
	}

	return gbGOOD;
}

static GOOD_OR_BAD owftdi_configure_bits(struct connection_in *in, enum ftdi_break_type break_) {
	struct port_in * pin = in->pown ;
	enum ftdi_bits_type bits;
	enum ftdi_stopbits_type stop;
	enum ftdi_parity_type parity;
	int ret;

	switch (pin->bits) {
		case 7:
			bits = BITS_7;
			break ;
		case 8:
		default:
			bits = BITS_8;
			break ;
	}

	switch (pin->parity) {
		case parity_none:
			parity = NONE;
			break ;
		case parity_even:
			parity = EVEN;
			break ;
		case parity_odd:
			parity = ODD;
			break ;
		case parity_mark:
			parity = MARK;
			break ;
	}

	// stop bits
	switch (pin->stop) {
		case stop_1:
			stop = STOP_BIT_1;
			break ;
		case stop_15:
			stop = STOP_BIT_15;
			break;
		case stop_2:
			stop = STOP_BIT_2;
			break ;
	}

	if((ret = ftdi_set_line_property2(FTDIC(in), bits, stop, parity, break_)) != 0) {
		ERROR_CONNECT("Failed to set FTDI bit-configuration: %d = %s",
				ret,
				ftdi_get_error_string(FTDIC(in)));
		return gbBAD;
	}

	return gbGOOD;
}

static GOOD_OR_BAD owftdi_configure_flow(struct connection_in *in) {
	struct port_in * pin = in->pown ;
	int flow;
	int ret;

	switch( pin->flow ) {
		case flow_hard:
			flow = SIO_RTS_CTS_HS;
			break ;
		case flow_none:
			flow = SIO_DISABLE_FLOW_CTRL;
			break ;
		case flow_soft:
		default:
			LEVEL_DEBUG("Unsupported COM port flow control");
			return -ENOTSUP ;
	}

	if((ret = ftdi_setflowctrl(FTDIC(in), flow)) != 0) {
		ERROR_CONNECT("Failed to set FTDI flow-control: %d = %s",
				ret,
				ftdi_get_error_string(FTDIC(in)));
		return gbBAD;
	}

	return gbGOOD;
}

static GOOD_OR_BAD owftdi_configure_dtrrts(struct connection_in *in, int value) {
	int ret = ftdi_setdtr_rts(FTDIC(in), value, value);
	if(ret != 0) {
		ERROR_CONNECT("Failed to set FTDI DTR/RTS high, %d: %s",
				ret, ftdi_get_error_string(FTDIC(in)));
		return gbBAD;
	}
	return gbGOOD;
}

GOOD_OR_BAD owftdi_change(struct connection_in *in) {
	RETURN_BAD_IF_BAD(owftdi_configure_baud(in));
	RETURN_BAD_IF_BAD(owftdi_configure_bits(in, BREAK_OFF));
	RETURN_BAD_IF_BAD(owftdi_configure_flow(in));

	return gbGOOD;
}

SIZE_OR_ERROR owftdi_read(BYTE * data, size_t requested_size, struct connection_in *in) {
	struct port_in * pin = in->pown ;
	struct timeval tv_start;
	size_t chars_read =0,
			 to_be_read = requested_size;
	int retries = 0;
	time_t timeout;

#ifdef BENCH
	struct timeval tv_end, tv_timing;
#endif

	/* Configure USB *block transfer* timeout, this is not wait-for-any-data timeout
	 * which we must implement manually below. */
	timeout = pin->timeout.tv_sec * 1000 + pin->timeout.tv_usec/1000;
	FTDIC(in)->usb_read_timeout = timeout;


	/* ftdi_read_data will loop internally and read until requested_size has been filled,
	 * or at least filled from whats available in the FTDI buffer.
	 * The FTDI buffer is however one step away from the LINK, a RX buffer is inbetween.
	 * A copy operation from the RX buffer to the FTDI buffer is done only when it's full,
	 * or when the "latency timer" kicks in. We thus have to keep reading from the FTDI
	 * buffer for a while.
	 *
	 * With our latency timer of 1ms, we will have a empty read after 1ms. However, in most
	 * cases we actuall DO get data in time!
	 */
	LEVEL_DEBUG("attempt %"PRIu64" bytes Time: "TVformat, requested_size, TVvar(&(pin->timeout)));
	gettimeofday(&tv_start, NULL);
	while(to_be_read> 0) {
		int ret = ftdi_read_data(FTDIC(in), &data[chars_read], to_be_read);
		if(ret < 0) {
			LEVEL_DATA("FTDI read failed: %d = %s",
					ret, ftdi_get_error_string(FTDIC(in)));
			STAT_ADD1(NET_read_errors);
			owftdi_close(in);
			return -EINVAL;
		}
		if(ret == 0) {
			// No data there yet... Sleep a bit and wait for more
			struct timeval tv_cur;
			time_t timeleft;
			gettimeofday(&tv_cur, NULL);
			timeleft = timeout*1000L -
				((tv_cur.tv_sec - tv_start.tv_sec)*1000000L +
				 (tv_cur.tv_usec - tv_start.tv_usec));

#if 0
			LEVEL_DEBUG("No data available, delaying a bit (%"PRIu64"ms left, retrie %d)",
					timeleft/1000L,
					retries
					);
#endif

			if(timeleft < 0) {
				LEVEL_CONNECT("TIMEOUT after %d bytes", requested_size - to_be_read);
				// XXX: Stats here is not in tcp_read?
				STAT_ADD1_BUS(e_bus_timeouts, in);
				return -EAGAIN;
			}

			/* In <= 1ms, the latency timer will trigger a copy of the RX buffer
			 * to the USB bus. When that time has come, we can try to read again.
			 * Do it a bit quicker to get smaller latency.
			 * Testing shows that we usually get our data within one or two loops
			 * in normal situations, so this isn't really heavy busyloading.
			 */
			usleep(MIN(timeleft, (retries < 10) ? 200 : 1000));
			retries++;
			continue;
		}
		TrafficIn("read", &data[chars_read], ret, in) ;
		to_be_read -= ret;
		chars_read += ret;
	}
#ifdef BENCH
	gettimeofday(&tv_end, NULL);
	timersub(&tv_end, &tv_start, &tv_timing);
	LEVEL_DEBUG("ftdi_read, read %d bytes. %d read-loops=%.6fus, actual= %d",
		requested_size,
		retries,
		((uint64_t)(tv_timing.tv_sec*1000000L) + (double)(tv_timing.tv_usec)),
		chars_read
		);
#endif

	LEVEL_DEBUG("ftdi_read: %d - %d = %d (%d retries)",
			(int)requested_size, (int) to_be_read, (int) (requested_size-to_be_read),
			retries) ;
	return chars_read;
}

GOOD_OR_BAD owftdi_write_once( const BYTE * data, size_t length, struct connection_in *in) {
	// Mimics COM_write_once

	FTDIC(in)->usb_write_timeout = Globals.timeout_serial * 1000; // XXX why not pin->timeout?

	TrafficOut("write", data, length, in);
	int ret = ftdi_write_data(FTDIC(in), data, (int)length);
	if(ret < 0) {
		ERROR_CONNECT("FTDI write to %s failed: %d = %s",
				SAFESTRING(DEVICENAME(in)),
				ret,
				ftdi_get_error_string(FTDIC(in)));

		STAT_ADD1_BUS(e_bus_write_errors, in);
		owftdi_close(in);
		return gbBAD;
	}

	LEVEL_DEBUG("ftdi_write: %"PRIu64" = actual %d", length, ret) ;

	return gbGOOD;
}

/* slurp up any pending chars -- used at the start to clear the com buffer */
void owftdi_slurp(struct connection_in *in, uint64_t usec) {
	int ret;
	BYTE data[1];

	/* This function is implemented similar to read; it must do repeated
	 * ftdi_read_data calls, or we might miss to slurp critical data,
	 * and the state machine will become confused.
	 *
	 * First purge any data in rx buffer, then do some subsequent reads.
	 */

	ret = ftdi_usb_purge_rx_buffer(FTDIC(in));
	if(ret != 0) {
		ERROR_CONNECT("Failed to purge rx buffers on FTDI device, %d: %s",
				ret, ftdi_get_error_string(FTDIC(in)));
	}

	// Allow for at least 2 rounds of latency timeouts.. or we seem to miss \n in 19200 sometimes..
	usec = MAX(usec, 2*1000);

	// USB block transfer timeout
	int prev_timeout = FTDIC(in)->usb_read_timeout;
	struct timeval tv_start;
	FTDIC(in)->usb_read_timeout = usec/1000;

	gettimeofday(&tv_start, NULL);
	while(1) {
		ret = ftdi_read_data(FTDIC(in), data, 1);
#if 0
		LEVEL_DEBUG("ftdi_slurp read ret %d, %s ", ret, ftdi_get_error_string(FTDIC(in)));
#endif
		if(ret < 1) {
			// No data there yet... Sleep a bit and wait for more
			struct timeval tv_cur;
			time_t timeleft;
			gettimeofday(&tv_cur, NULL);
			timeleft = usec -
				((tv_cur.tv_sec - tv_start.tv_sec)*1000000L +
				 (tv_cur.tv_usec - tv_start.tv_usec));

			if(timeleft < 0) {
#if 0
				LEVEL_DEBUG("ftdi_slurp timeout, timeleft=%d ", timeleft);
#endif
				return;
			}

			usleep(MIN(timeleft, 200));
			continue;
		}

		TrafficIn("slurp", data, 1, in);
	}

	FTDIC(in)->usb_read_timeout = prev_timeout;
}

#endif /* OW_FTDI */
