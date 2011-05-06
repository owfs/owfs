/*
$Id$
    OWFS -- One-Wire filesystem
    OWHTTPD -- One-Wire Web Server
    Written 2003 Paul H Alfille
    email: palfille@earthlink.net
    Released under the GPL
    See the header file: ow.h for full attribution
    1wire/iButton system from Dallas Semiconductor
*/

#include <config.h>
#include "owfs_config.h"
#include "ow.h"
#include "ow_counters.h"
#include "ow_connection.h"

/* All the rest of the program sees is the DS9907_detect and the entry in iroutines */

static RESET_TYPE DS9097_reset(const struct parsedname *pn);
static RESET_TYPE DS9097_reset_in( struct connection_in * in );
static GOOD_OR_BAD DS9097_pre_reset(struct connection_in *in ) ;
static void DS9097_post_reset(struct connection_in *in ) ;
static GOOD_OR_BAD DS9097_sendback_bits(const BYTE * outbits, BYTE * inbits, const size_t length, const struct parsedname *pn);
static void DS9097_setroutines(struct connection_in *in);
static GOOD_OR_BAD DS9097_send_and_get(const BYTE * bussend, BYTE * busget, const size_t length, struct connection_in *in);

#define	OneBit	0xFF
//#define ZeroBit 0xC0
// Should be all zero's when we send 8 bits. digitemp write 0xFF or 0x00
#define ZeroBit 0x00

// at slower speed of course
#define RESET_BYTE 0xF0 

/* Device-specific functions */
static void DS9097_setroutines(struct connection_in *in)
{
	in->iroutines.detect = DS9097_detect;
	in->iroutines.reset = DS9097_reset;
	in->iroutines.next_both = NO_NEXT_BOTH_ROUTINE;
	in->iroutines.PowerByte = NO_POWERBYTE_ROUTINE;
    in->iroutines.ProgramPulse = NO_PROGRAMPULSE_ROUTINE;
	in->iroutines.sendback_data = NO_SENDBACKDATA_ROUTINE;
	in->iroutines.sendback_bits = DS9097_sendback_bits;
	in->iroutines.select = NO_SELECT_ROUTINE;
	in->iroutines.select_and_sendback = NO_SELECTANDSENDBACK_ROUTINE;
	in->iroutines.reconnect = NO_RECONNECT_ROUTINE;
	in->iroutines.close = COM_close;
	in->iroutines.flags = ADAP_FLAG_default;
	in->bundling_length = UART_FIFO_SIZE / 10;
}

/* _detect is a bit of a misnomer, no detection is actually done */
// no bus locking here (higher up)
GOOD_OR_BAD DS9097_detect(struct connection_in *in)
{
	/* Set up low-level routines */
	DS9097_setroutines(in);

	in->Adapter = adapter_DS9097;
	// in->adapter_name already set, to support HA3 and HA4B
	in->busmode = bus_passive;	// in case initially tried DS9097U

	/* open the COM port in 9600 Baud  */
	SOC(in)->state = cs_virgin ;
	SOC(in)->baud = B9600 ;
	SOC(in)->vmin = 1; // minimum chars
	SOC(in)->vtime = 0; // decisec wait
	SOC(in)->parity = parity_none; // parity
	SOC(in)->stop = stop_1; // stop bits
	SOC(in)->bits = 8; // bits / byte
	
	if ( Globals.serial_hardflow ) {
		// first pass with hardware flow control
		SOC(in)->flow = flow_hard; // flow control
	} else {
		SOC(in)->flow = flow_none; // flow control
	}

	switch (SOC(in)->type) {
		case ct_telnet:
			SOC(in)->timeout.tv_sec = Globals.timeout_network ;
			SOC(in)->timeout.tv_usec = 0 ;

		case ct_serial:
		default:
			SOC(in)->timeout.tv_sec = Globals.timeout_serial ;
			SOC(in)->timeout.tv_usec = 0 ;
	}
	SOC(in)->timeout.tv_sec = Globals.timeout_serial ;
	SOC(in)->timeout.tv_usec = 0 ;
	RETURN_BAD_IF_BAD(COM_open(in)) ;

	switch( DS9097_reset_in(in) ) {
		case BUS_RESET_OK:
		case BUS_RESET_SHORT:
			return gbGOOD ;
		default:
			break ;
	}

	/* open the COM port in 9600 Baud  */
	/* Second pass */
	SOC(in)->flow = flow_none ;
	RETURN_BAD_IF_BAD(COM_change(in)) ;

	switch( DS9097_reset_in(in) ) {
		case BUS_RESET_OK:
		case BUS_RESET_SHORT:
			return gbGOOD ;
		default:
			break ;
	}

	/* open the COM port in 9600 Baud  */
	/* Third pass, hardware flow control */
	SOC(in)->flow = flow_hard ;
	RETURN_BAD_IF_BAD(COM_change(in)) ;

	switch( DS9097_reset_in(in) ) {
		case BUS_RESET_OK:
		case BUS_RESET_SHORT:
			return gbGOOD ;
		default:
			break ;
	}
	
	return gbBAD ;
}

/* DS9097 Reset -- A little different from DS2480B */
/* Puts in 9600 baud, sends 11110000 then reads response */
static RESET_TYPE DS9097_reset(const struct parsedname *pn)
{
	return DS9097_reset_in( pn->selected_connection ) ;
}

/* DS9097 Reset -- A little different from DS2480B */
/* Puts in 9600 baud, sends 11110000 then reads response */
static RESET_TYPE DS9097_reset_in( struct connection_in * in )
{
	BYTE resetbyte = RESET_BYTE;
	BYTE responsebyte;

	if ( BAD( DS9097_pre_reset( in ) ) ) {
		return BUS_RESET_ERROR ;
	}

	if ( BAD( DS9097_send_and_get(&resetbyte, &responsebyte, 1, in )) ) {
		DS9097_post_reset( in) ;
		return BUS_RESET_ERROR ;
	}

	DS9097_post_reset(in) ;
	
	switch (responsebyte) {
	case 0x00:
		return BUS_RESET_SHORT;
	case RESET_BYTE:
		// no presence
		in->AnyDevices = anydevices_no ;
		return BUS_RESET_OK;
	default:
		in->AnyDevices = anydevices_yes ;
		return BUS_RESET_OK;
	}
}

/* Puts in 9600 baud */
static GOOD_OR_BAD DS9097_pre_reset(struct connection_in *in )
{
	RETURN_BAD_IF_BAD( COM_test(in) ) ;

	/* 8 data bits */
	SOC(in)->bits = 8 ;
	SOC(in)->baud = B9600 ;

	if ( BAD( COM_change(in)) ) {
		ERROR_CONNECT("Cannot set attributes: %s", SAFESTRING(SOC(in)->devicename));
		DS9097_post_reset( in ) ;
		return gbBAD;
	}
	return gbGOOD;
}

/* Restore terminal settings (serial port settings) */
static void DS9097_post_reset(struct connection_in *in )
{
	if (Globals.eightbit_serial) {
		/* coninue with 8 data bits */
		SOC(in)->bits = 8;
	} else {
		/* 6 data bits, Receiver enabled, Hangup, Dont change "owner" */
		SOC(in)->bits = 6;
	}
#ifndef B115200
	/* MacOSX support max 38400 in termios.h ? */
	SOC(in)->baud = B38400 ;
#else
	SOC(in)->baud = B115200 ;
#endif

	/* Flush the input and output buffers */
	COM_flush(in); // Adds no appreciable time
	COM_change(in) ;
}

/* Symmetric */
/* send bits -- read bits */
/* Actually uses bit zero of each byte */
/* So each "byte" has already been expanded to 1 bit/byte */
/* Dispatches DS9097_MAX_BITS "bits" at a time */
#define DS9097_MAX_BITS 24
static GOOD_OR_BAD DS9097_sendback_bits(const BYTE * outbits, BYTE * inbits, const size_t length, const struct parsedname *pn)
{
	BYTE local_data[DS9097_MAX_BITS];
	size_t global_counter ;
	size_t local_counter ;
	size_t offset ;
	struct connection_in * in = pn->selected_connection ;

	/* Split into smaller packets? */
	for ( local_counter = global_counter = offset = 0 ; offset < length ; ) {
		// encode this bit
		local_data[local_counter] = outbits[global_counter] ? OneBit : ZeroBit;
		// point to next one
		++local_counter ;
		++global_counter ;
		// test if enough bits to send to master
		if (local_counter == DS9097_MAX_BITS || global_counter == length) {
			/* Communication with DS9097 routine */
			/* Up to DS9097_MAX_BITS bits at a time */
			if ( BAD( DS9097_send_and_get( local_data, &inbits[offset], local_counter, in )) ) {
				STAT_ADD1_BUS(e_bus_errors, in);
				return gbBAD;
			}
			offset += local_counter ;
			local_counter = 0 ;
		}
	} 

	/* Decode Bits */
	for (global_counter = 0; global_counter < length; ++global_counter) {
		inbits[global_counter] &= 0x01; // mask out all but lowest bit
	}

	return gbGOOD;
}

/* Routine to send a string of bits and get another string back */
static GOOD_OR_BAD DS9097_send_and_get(const BYTE * bussend, BYTE * busget, const size_t length, struct connection_in * in)
{
	switch( SOC(in)->type ) {
		case ct_telnet:
			RETURN_BAD_IF_BAD( telnet_write_binary( bussend, length, in) ) ;
			break ;
		case ct_serial:
		default:
			RETURN_BAD_IF_BAD( COM_write( bussend, length, in ) ) ;
			break ;
	}

	/* get back string -- with timeout and partial read loop */
	return COM_read( busget, length, in ) ;
}
