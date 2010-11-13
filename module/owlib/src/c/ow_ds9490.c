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

/* DS9490R-W USB 1-Wire master

   USB parameters:
       Vendor ID: 04FA
       ProductID: 2490

   Dallas controller DS2490

*/

#include <config.h>
#include "owfs_config.h"
#include "ow.h"
#include "ow_counters.h"
#include "ow_connection.h"
#include "ow_codes.h"

#if OW_USB						/* conditional inclusion of USB */
#include "ow_usb_msg.h"
#include "ow_usb_cycle.h"

/* Extensive FreeBSD workarounds by Robert Nilsson <rnilsson@mac.com> */
/* Peter Radcliffe updated support for FreeBSD >= 8 */
#ifdef __FreeBSD__
	// Add a few definitions we need
#undef HAVE_USB_INTERRUPT_READ	// This call in libusb is unneeded for FreeBSD (and it's broken)
#if __FreeBSD__ < 8
#include <sys/types.h>
#include <dev/usb/usb.h>
#define USB_CLEAR_HALT BSD_usb_clear_halt
#endif /* __FreeBSD__ < 8 */
struct usb_dev_handle {
	FILE_DESCRIPTOR_OR_ERROR file_descriptor;
	struct usb_bus *bus;
	struct usb_device *device;
	int config;
	int interface;
	int altsetting;
	void *impl_info;
};
#endif /* __FreeBSD__ */

#ifndef USB_CLEAR_HALT
#define USB_CLEAR_HALT usb_clear_halt
#endif /* USB_CLEAR_HALT */

/* All the rest of the code sees is the DS9490_detect routine and the iroutine structure */

static RESET_TYPE DS9490_reset(const struct parsedname *pn);

static GOOD_OR_BAD DS9490_detect_single_adapter(int usb_nr, struct connection_in *in);
static GOOD_OR_BAD DS9490_detect_all_adapters(struct connection_in * in_first);
static GOOD_OR_BAD DS9490_detect_specific_adapter(int bus_nr, int dev_nr, struct connection_in * in) ;

static GOOD_OR_BAD DS9490_reconnect(const struct parsedname *pn);
static GOOD_OR_BAD DS9490_redetect_low(struct connection_in * in);
static GOOD_OR_BAD DS9490_redetect_match(struct connection_in * in);
static GOOD_OR_BAD DS9490_redetect_specific_adapter( struct connection_in * in) ;
static void DS9490_setroutines(struct connection_in *in);
static GOOD_OR_BAD DS9490_setup_adapter(struct connection_in * in) ;

static enum search_status DS9490_next_both(struct device_search *ds, const struct parsedname *pn);
static GOOD_OR_BAD DS9490_sendback_data(const BYTE * data, BYTE * resp, size_t len, const struct parsedname *pn);
static GOOD_OR_BAD DS9490_HaltPulse(const struct parsedname *pn);
static GOOD_OR_BAD DS9490_PowerByte(BYTE byte, BYTE * resp, UINT delay, const struct parsedname *pn);
static GOOD_OR_BAD DS9490_ProgramPulse(const struct parsedname *pn);
static GOOD_OR_BAD DS9490_overdrive(const struct parsedname *pn);
static void SetupDiscrepancy(const struct device_search *ds, BYTE * discrepancy);
static int FindDiscrepancy(BYTE * last_sn, BYTE * discrepancy_sn);
static enum search_status DS9490_directory(struct device_search *ds, struct dirblob *db, const struct parsedname *pn);
static GOOD_OR_BAD DS9490_SetSpeed(const struct parsedname *pn);
static void DS9490_SetFlexParameters(struct connection_in *in) ;

/* Device-specific routines */
static void DS9490_setroutines(struct connection_in *in)
{
	in->iroutines.detect = DS9490_detect;
	in->iroutines.reset = DS9490_reset;
	in->iroutines.next_both = DS9490_next_both;
	in->iroutines.PowerByte = DS9490_PowerByte;
	in->iroutines.ProgramPulse = DS9490_ProgramPulse;
	in->iroutines.sendback_data = DS9490_sendback_data;
	in->iroutines.sendback_bits = NO_SENDBACKBITS_ROUTINE;
	in->iroutines.select = NO_SELECT_ROUTINE;
	in->iroutines.select_and_sendback = NO_SELECTANDSENDBACK_ROUTINE;
	in->iroutines.reconnect = DS9490_reconnect;
	in->iroutines.close = DS9490_close;
	in->iroutines.flags = ADAP_FLAG_default;

	in->bundling_length = USB_FIFO_SIZE;
}

#define DS2490_BULK_BUFFER_SIZE     64
//#define DS2490_DIR_GULP_ELEMENTS     ((DS2490_BULK_BUFFER_SIZE/SERIAL_NUMBER_SIZE) - 1)
#define DS2490_DIR_GULP_ELEMENTS     (1)

#define CONTROL_CMD     0x00
#define COMM_CMD        0x01
#define MODE_CMD        0x02
#define TEST_CMD        0x03

#define CTL_RESET_DEVICE        0x0000
#define CTL_START_EXE           0x0001
#define CTL_RESUME_EXE          0x0002
#define CTL_HALT_EXE_IDLE       0x0003
#define CTL_HALT_EXE_DONE       0x0004
#define CTL_FLUSH_COMM_CMDS     0x0007
#define CTL_FLUSH_CV_BUFFER     0x0008
#define CTL_FLUSH_CMT_BUFFER    0x0009
#define CTL_GET_COMM_CMDS       0x000A

#define MOD_PULSE_EN            0x0000
#define MOD_SPEED_CHANGE_EN     0x0001
#define MOD_1WIRE_SPEED         0x0002
#define MOD_STRONG_PU_DURATION  0x0003
#define MOD_PULLDOWN_SLEWRATE   0x0004
#define MOD_PROG_PULSE_DURATION 0x0005
#define MOD_WRITE1_LOWTIME      0x0006
#define MOD_DSOW0_TREC          0x0007

//
// Value field COMM Command options
//
// COMM Bits (bitwise or into COMM commands to build full value byte pairs)
// Byte 1
#define COMM_TYPE                   0x0008
#define COMM_SE                     0x0008
#define COMM_D                      0x0008
#define COMM_Z                      0x0008
#define COMM_CH                     0x0008
#define COMM_SM                     0x0008

#define COMM_R                      0x0008
#define COMM_IM                     0x0001

// Byte 2
#define COMM_PS                     0x4000
#define COMM_PST                    0x4000
#define COMM_CIB                    0x4000
#define COMM_RTS                    0x4000
#define COMM_DT                     0x2000
#define COMM_SPU                    0x1000
#define COMM_F                      0x0800
#define COMM_ICP                    0x0200
#define COMM_RST                    0x0100

// Read Straight command, special bits
#define COMM_READ_STRAIGHT_NTF          0x0008
#define COMM_READ_STRAIGHT_ICP          0x0004
#define COMM_READ_STRAIGHT_RST          0x0002
#define COMM_READ_STRAIGHT_IM           0x0001

//
// Value field COMM Command options (0-F plus assorted bits)
//
#define COMM_ERROR_ESCAPE               0x0601
#define COMM_SET_DURATION               0x0012
#define COMM_BIT_IO                     0x0020
#define COMM_PULSE                      0x0030
#define COMM_1_WIRE_RESET               0x0042
#define COMM_BYTE_IO                    0x0052
#define COMM_MATCH_ACCESS               0x0064
#define COMM_BLOCK_IO                   0x0074
#define COMM_READ_STRAIGHT              0x0080
#define COMM_DO_RELEASE                 0x6092
#define COMM_SET_PATH                   0x00A2
#define COMM_WRITE_SRAM_PAGE            0x00B2
#define COMM_WRITE_EPROM                0x00C4
#define COMM_READ_CRC_PROT_PAGE         0x00D4
#define COMM_READ_REDIRECT_PAGE_CRC     0x21E4
#define COMM_SEARCH_ACCESS              0x00F4

// Mode Command Code Constants
// Enable Pulse Constants
#define ENABLEPULSE_PRGE         0x01	// programming pulse
#define ENABLEPULSE_SPUE         0x02	// strong pull-up

// Define our combinations:
#define ENABLE_PROGRAM_ONLY         (ENABLEPULSE_PRGE)
#define ENABLE_PROGRAM_AND_PULSE    (ENABLEPULSE_PRGE | ENABLEPULSE_SPUE)


// 1Wire Bus Speed Setting Constants
#define ONEWIREBUSSPEED_REGULAR        0x00
#define ONEWIREBUSSPEED_FLEXIBLE       0x01
#define ONEWIREBUSSPEED_OVERDRIVE      0x02

#define PARMSET_Slew15Vus   0x0
#define PARMSET_Slew2p20Vus 0x1
#define PARMSET_Slew1p65Vus 0x2
#define PARMSET_Slew1p37Vus 0x3
#define PARMSET_Slew1p10Vus 0x4
#define PARMSET_Slew0p83Vus 0x5
#define PARMSET_Slew0p70Vus 0x6
#define PARMSET_Slew0p55Vus 0x7

#define PARMSET_W1L_08us 0x0
#define PARMSET_W1L_09us 0x1
#define PARMSET_W1L_10us 0x2
#define PARMSET_W1L_11us 0x3
#define PARMSET_W1L_12us 0x4
#define PARMSET_W1L_13us 0x5
#define PARMSET_W1L_14us 0x6
#define PARMSET_W1L_15us 0x7

#define PARMSET_DS0_W0R_3us 0x0
#define PARMSET_DS0_W0R_4us 0x1
#define PARMSET_DS0_W0R_5us 0x2
#define PARMSET_DS0_W0R_6us 0x3
#define PARMSET_DS0_W0R_7us 0x4
#define PARMSET_DS0_W0R_8us 0x5
#define PARMSET_DS0_W0R_9us 0x6
#define PARMSET_DS0_W0R_10us 0x7

/* From datasheet http://datasheets.maxim-ic.com/en/ds/DS2490.pdf page 15 */
/* 480 usec = 8usec * 60 and 60(decimal) = 0x3C */
/* 480 usec is the recommended program pulse duration in the DS2406 DS2502 DS2505 datasheets */
#define PROGRAM_PULSE_DURATION_CODE	0x3C

/* ------------------------------------------------------------ */
/* --- USB detection routines ----------------------------------*/

/* Main routine for detecting (and setting up) the DS2490 1-wire USB chip */
GOOD_OR_BAD DS9490_detect(struct connection_in *in)
{
	struct address_pair ap ;
	GOOD_OR_BAD gbResult = gbBAD;
	
	/* uses "name" before it's cleared by connection_init */
	Parse_Address( in->name, &ap ) ;

	DS9490_connection_init( in ) ;

	switch ( ap.entries ) {
		case 0:
			// Minimal specification, so use first USB device
			gbResult = DS9490_detect_single_adapter( 1, in) ;
			break ;
		case 1:
			switch( ap.first.type ) {
				case address_all:
					LEVEL_DEBUG("Look for all USB adapters");
					gbResult = DS9490_detect_all_adapters(in) ;
					break ;
				case address_numeric:
					LEVEL_DEBUG("Look for USB adapter number %d",ap.first.number);
					gbResult = DS9490_detect_single_adapter( ap.first.number, in) ;
					break ;
				case address_alpha:
					if ( strncasecmp(ap.first.alpha,"scan",4) == 0 ) {
						SAFEFREE(in->name) ;
						LEVEL_DEBUG("Add USB scanning capability");
						gbResult = USB_monitor_detect(in) ;
						break ;
					}
					// fall through
				default:
					LEVEL_DEFAULT("Unclear what <%s> means in USB specification, will use first adapter.",ap.first.alpha) ;
					gbResult = DS9490_detect_single_adapter( 1, in) ;
					break ;
			}
			break ;
		case 2:
			if ( ap.first.type != address_numeric || ap.second.type != address_numeric ) {
				LEVEL_DEFAULT("USB address <%s:%s> not in number:number format",ap.first.alpha,ap.second.alpha) ;
			} else {
				gbResult = DS9490_detect_specific_adapter( ap.first.number, ap.second.number, in ) ;
			}
			break ;
	}
	Free_Address( &ap ) ;
	return gbResult;
}

/* Open a DS9490  -- low level code (to allow for repeats)  */
static GOOD_OR_BAD DS9490_detect_single_adapter(int usb_nr, struct connection_in * in)
{
	struct usb_list ul;

	USB_first(&ul);

	if ( usb_nr == 1 ) {
		// return the first free
		RETURN_BAD_IF_BAD( USB_next( &ul ) ) ;
	} else {
		// return the nth
		RETURN_BAD_IF_BAD( USB_next_until_n( &ul, usb_nr ) ) ;
	}

	RETURN_BAD_IF_BAD( DS9490_open_and_name(&ul, in)) ;
	if ( BAD(DS9490_ID_this_master(in)) ) {
		DS9490_close(in) ;
		return gbBAD;
	}

	return gbGOOD ;
}

/* Open a DS9490  -- low level code (to allow for repeats)  */
static GOOD_OR_BAD DS9490_detect_specific_adapter(int bus_nr, int dev_nr, struct connection_in * in)
{
	struct usb_list ul;
	
	// Mark this connection as taking only this address pair. Important for reconnections.
	in->master.usb.specific_usb_address = 1 ;

	USB_first(&ul);
	while ( GOOD(USB_next(&ul)) ) {
		if ( ul.usb_bus_number != bus_nr || ul.usb_dev_number != dev_nr ) {
			LEVEL_CONNECT("USB DS9490 %d:%d passed over. (Looking for %d:%d)", ul.usb_bus_number, ul.usb_dev_number, bus_nr, dev_nr );
			continue ;
		}
		return DS9490_open_and_name(&ul, in) ;
	}

	LEVEL_CONNECT("No matching USB DS9490 bus master found");
	return gbBAD;
}

/* Open a DS9490  -- low level code (to allow for repeats)  */
static GOOD_OR_BAD DS9490_detect_all_adapters(struct connection_in * in_first)
{
	struct usb_list ul;
	struct connection_in * in = in_first ;

	USB_first(&ul);
	while ( GOOD(USB_next(&ul)) ) {
		if ( BAD(DS9490_open_and_name(&ul, in)) ) {
			LEVEL_DEBUG("Cannot open USB device %.d:%.d", ul.usb_bus_number, ul.usb_dev_number );
			continue ;
		} else if ( BAD(DS9490_ID_this_master(in)) ) {
			DS9490_close(in) ;
			LEVEL_DEBUG("Cannot access USB device %.d:%.d", ul.usb_bus_number, ul.usb_dev_number );
			continue;
		} else{
			in = NewIn(in_first) ;
			if ( in == NO_CONNECTION ) {
				return gbGOOD ;
			}
			// set up the new connection for the next adapter
			DS9490_connection_init(in) ;
		}
	}
	if ( in == in_first ) {
		LEVEL_CONNECT("No USB DS9490 bus masters used");
		return gbBAD;
	}
	// Remove the extra connection
	RemoveIn(in);
	return gbGOOD ;
}

void DS9490_connection_init( struct connection_in * in )
{
	if ( in == NO_CONNECTION ) {
		return ;
	}
	SAFEFREE( in->name ) ;
	in->name = owstrdup(badUSBname);		// initialized

	DS9490_setroutines(in);		// set up close, reconnect, reset, ...

	in->busmode = bus_usb;
	in->master.usb.usb = NULL ; // no handle yet
	in->master.usb.usb_bus_number = in->master.usb.usb_dev_number = -1 ;
	memset( in->master.usb.ds1420_address, 0, SERIAL_NUMBER_SIZE ) ;
	DS9490_setroutines(in);
	in->Adapter = adapter_DS9490;	/* OWFS assigned value */
	in->adapter_name = "DS9490";
}

/* ------------------------------------------------------------ */
/* --- USB redetect routines -----------------------------------*/

/* When the errors stop the USB device from functioning -- close and reopen.
 * If it fails, re-scan the USB bus and search for the old adapter */
static GOOD_OR_BAD DS9490_reconnect(const struct parsedname *pn)
{
	GOOD_OR_BAD ret;
	struct connection_in * in = pn->selected_connection ;

	if ( in->master.usb.specific_usb_address ) { 
		// special case where a usb bus:dev pair was given
		// only connect to the same spot
		return DS9490_redetect_specific_adapter( in ) ; 
	}

	/* Have to protect usb_find_busses() and usb_find_devices() with
	 * a lock since libusb could crash if 2 threads call it at the same time.
	 * It's not called until DS9490_redetect_low(), but I lock here just
	 * to be sure DS9490_close() and DS9490_open() get one try first. */
	LIBUSBLOCK;
	DS9490_close( in ) ;
	ret = DS9490_redetect_low(in) ;
	LIBUSBUNLOCK;

	if ( GOOD(ret) ) {
		LEVEL_DEFAULT("Found USB DS9490 bus master after USB rescan as [%s]", SAFESTRING(in->name));
	}
	return ret;
}

/* Open a DS9490  -- low level code (to allow for repeats)  */
static GOOD_OR_BAD DS9490_redetect_low(struct connection_in * in)
{
	struct usb_list ul;

	USB_first(&ul);
		
	while ( GOOD(USB_next(&ul)) ) {
		// try to open the DS9490
		if ( BAD(DS9490_open_and_name(&ul, in)) ) {
			LEVEL_CONNECT("Cannot open USB bus master, Find next...");
			continue;
		}
		RETURN_GOOD_IF_GOOD( DS9490_redetect_match( in ) ) ;
		DS9490_close(in);
	}
	//LEVEL_CONNECT("No available USB DS9490 bus master found");
	return gbBAD;
}

/* Open a DS9490  -- low level code (to allow for repeats)  */
static GOOD_OR_BAD DS9490_redetect_specific_adapter( struct connection_in * in)
{
	struct address_pair ap ;
	GOOD_OR_BAD gbResult ;
	
	Parse_Address( in->name, &ap ) ;
	if ( ap.first.type != address_numeric || ap.second.type != address_numeric ) {
		LEVEL_DEBUG("Cannot understand the specific usb address pair to reconnect <%s>",in->name) ;
		gbResult = gbBAD ;
	} else {
		char * name_copy = owstrdup( in->name ) ; // since DS9490_close clears this value and we may need to restore on error

		/* Have to protect usb_find_busses() and usb_find_devices() with
		 * a lock since libusb could crash if 2 threads call it at the same time.
		 * It's not called until DS9490_redetect_low(), but I lock here just
		 * to be sure DS9490_close() and DS9490_open() get one try first. */
		LIBUSBLOCK;
		DS9490_close( in ) ;
		gbResult = DS9490_detect_specific_adapter( ap.first.number, ap.second.number, in) ;
		LIBUSBUNLOCK;
		
		if ( BAD(gbResult) ) {
			SAFEFREE( in->name ) ;
			in->name = name_copy ; // so no need to free here
		} else {
			owfree( name_copy ) ;
		}
	}

	Free_Address( &ap ) ;
	return gbResult ;
}

/* Open a DS9490  -- low level code (to allow for repeats)  */
static GOOD_OR_BAD DS9490_redetect_match( struct connection_in * in)
{
	struct dirblob db ;
	BYTE sn[SERIAL_NUMBER_SIZE] ;
	int device_number ;

	LEVEL_DEBUG("Attempting reconnect on %s",SAFESTRING(in->name));

	// Special case -- originally untagged adapter
	if ( in->master.usb.ds1420_address[0] == '\0' ) {
		LEVEL_CONNECT("Since originally untagged bus master, we will use first available slot.");
		return gbGOOD ;
	}

	// Generate a root directory
	RETURN_BAD_IF_BAD( DS9490_root_dir( &db, in ) ) ;

	// This adapter has no tags, so not the one we want
	if ( DirblobElements( &db) == 0 ) {
		DirblobClear( &db ) ;
		LEVEL_DATA("Empty directory on [%s] (Doesn't match initial scan).", SAFESTRING(in->name));
		return gbBAD ;
	}

	// Scan directory for a match to the original tag
	device_number = 0 ;
	while ( DirblobGet( device_number, sn, &db ) == 0 ) {
		if (memcmp(sn, in->master.usb.ds1420_address, SERIAL_NUMBER_SIZE) == 0) {	// same tag device?
			LEVEL_DATA("Matching device [%s].", SAFESTRING(in->name));
			DirblobClear( &db ) ;
			return gbGOOD ;
		}
		++device_number ;
	}
	// Couldn't find correct ds1420 chip on this adapter
	LEVEL_CONNECT("Couldn't find correct ds1420 chip on this bus master [%s] (want: " SNformat ")", SAFESTRING(in->name), SNvar(in->master.usb.ds1420_address));
	DirblobClear( &db ) ;
	return gbBAD;
}

/* ------------------------------------------------------------ */
/* --- USB reset routines --------------------------------------*/

/* Reset adapter and detect devices on the bus */
/* BUS locked at high level */
/* return 1=short, 0 good <0 error */
static RESET_TYPE DS9490_reset(const struct parsedname *pn)
{
	int i; 
	BYTE buffer[ DS9490_getstatus_BUFFER_LENGTH ];
	int USpeed;
	struct connection_in * in = pn->selected_connection ;
	int readlen = 0 ;
	
	if (in->master.usb.usb == NULL || in->master.usb.dev == NULL) {
		LEVEL_DEBUG("Attempting RESET on null bus") ;
		return BUS_RESET_ERROR;
	}

	// Do we need to change settings?
	if (in->changed_bus_settings > 0) {
		// Prevent recursive loops on reset
		DS9490_SetSpeed(pn);	// reset paramters
		in->changed_bus_settings = 0 ;
	}

	memset(buffer, 0, 32);

	USpeed = (in->speed == bus_speed_slow) ?
		(in->flex==bus_yes_flex ? ONEWIREBUSSPEED_FLEXIBLE : ONEWIREBUSSPEED_REGULAR)
		: ONEWIREBUSSPEED_OVERDRIVE;

	if ( BAD( USB_Control_Msg(COMM_CMD, COMM_1_WIRE_RESET | COMM_F | COMM_IM | COMM_SE, USpeed, pn)) ) {
		LEVEL_DATA("Reset command rejected");
		return BUS_RESET_ERROR;			// fatal error... probably closed usb-handle
	}

	if (in->ds2404_found && (in->speed == bus_speed_slow)) {
		// extra delay for alarming DS1994/DS2404 complience
		UT_delay(5);
	}

	switch( DS9490_getstatus(buffer, &readlen, pn) ) {
		case BUS_RESET_SHORT:
			/* Short detected, but otherwise no bigger problem */
			return BUS_RESET_SHORT ;
		case BUS_RESET_OK:
			break ;
		case BUS_RESET_ERROR:
		default:
			return BUS_RESET_ERROR;
	}
	//USBpowered = (buffer[8]&STATUSFLAGS_PMOD) == STATUSFLAGS_PMOD ;
	in->AnyDevices = anydevices_yes ;
	for (i = 16; i < readlen; i++) {
		BYTE val = buffer[i];
		LEVEL_DEBUG("Status bytes[%d]: %X", i, val);
		if (val != ONEWIREDEVICEDETECT) {
			// check for NRS bit (0x01)
			if (val & COMMCMDERRORRESULT_NRS) {
				// empty bus detected, no presence pulse detected
				in->AnyDevices = anydevices_no;
				LEVEL_DATA("no presense pulse detected");
			}
		}
	}
	return BUS_RESET_OK;
}

/* ------------------------------------------------------------ */
/* --- USB Directory functions  --------------------------------*/

static enum search_status DS9490_next_both(struct device_search *ds, const struct parsedname *pn)
{
	struct dirblob *db = (ds->search == _1W_CONDITIONAL_SEARCH_ROM) ?
		&(pn->selected_connection->alarm) : &(pn->selected_connection->main);
	int dir_gulp_elements = (pn->pathlength==0) ? DS2490_DIR_GULP_ELEMENTS : 1 ;

	if (pn->selected_connection->AnyDevices == anydevices_no) {
		return search_done;
	}

	// LOOK FOR NEXT ELEMENT
	++ds->index;
	LEVEL_DEBUG("Index %d", ds->index);

	if (ds->index % dir_gulp_elements == 0) {
		if (ds->LastDevice) {
			return search_done;
		}
		switch ( DS9490_directory(ds, db, pn) ) {
			case search_done:
				return search_done;
			case search_error:
				return search_error;
			case search_good:
				break;
		}
	}

	switch ( DirblobGet(ds->index % dir_gulp_elements, ds->sn, db) ) {
		case 0:
			LEVEL_DEBUG("SN found: " SNformat, SNvar(ds->sn));
			/* test for special device families */
			switch (ds->sn[0]) {
				case 0x00:
					LEVEL_DATA("NULL family found");
					return search_error;
				default:
					break;
			}
			return search_good;
		case -ENODEV:
		default:
			LEVEL_DEBUG("SN finished");
			return search_done;
	}
}

// Read up to 7 (DS2490_DIR_GULP_ELEMENTS) at a time, and  place into
// a dirblob. Called from DS9490_next_both every 7 devices to fill.
static enum search_status DS9490_directory(struct device_search *ds, struct dirblob *db, const struct parsedname *pn)
{
	BYTE status_buffer[ DS9490_getstatus_BUFFER_LENGTH ];
	BYTE EP2_data[SERIAL_NUMBER_SIZE] ; //USB endpoint 3 buffer
	union {
		BYTE b[DS2490_BULK_BUFFER_SIZE] ;
		BYTE sn[DS2490_BULK_BUFFER_SIZE/SERIAL_NUMBER_SIZE][SERIAL_NUMBER_SIZE];
	} EP3 ; //USB endpoint 3 buffer
	SIZE_OR_ERROR ret;
	int bytes_back;
	int devices_found;
	int device_index;
	int dir_gulp_elements = (pn->pathlength==0) ? DS2490_DIR_GULP_ELEMENTS : 1 ;
	int readlen = 0 ;
	
	DirblobClear(db);

	if ( BAD( BUS_select(pn) ) ) {
		LEVEL_DEBUG("Selection problem before a directory listing") ;
		return search_error ;
	}

	SetupDiscrepancy(ds, EP2_data);

	// set the search start location
	ret = DS9490_write(EP2_data, SERIAL_NUMBER_SIZE, pn) ;
	if ( ret < SERIAL_NUMBER_SIZE ) {
		LEVEL_DATA("bulk write problem = %d", ret);
		return search_error;
	}
	// Send the search request
	if ( BAD( USB_Control_Msg(COMM_CMD, COMM_SEARCH_ACCESS | COMM_IM | COMM_SM | COMM_F | COMM_RTS, (dir_gulp_elements << 8) | (ds->search), pn) ) ) {
		LEVEL_DATA("control error");
		return search_error;
	}
	// read the search status
	if ( DS9490_getstatus(status_buffer, &readlen, pn)  != BUS_RESET_OK ) {
		return search_error;
	}

	// test the buffer size waiting for us
	bytes_back = status_buffer[13];
	LEVEL_DEBUG("Got %d bytes from USB search", bytes_back);
	if (bytes_back == 0) {
		/* Nothing found on the bus. Have to return something != 0 to avoid
		 * getting stuck in loop in FS_realdir() and FS_alarmdir()
		 * which ends when ret!=0 */
		LEVEL_DATA("ReadBufferstatus == 0");
		return search_done;
	} else if ( bytes_back % SERIAL_NUMBER_SIZE != 0 ) {
		LEVEL_DATA("ReadBufferstatus size %d not a multiple of %d", bytes_back,SERIAL_NUMBER_SIZE);
		return search_error;
	} else if ( bytes_back > (dir_gulp_elements + 1) * SERIAL_NUMBER_SIZE ) {
		LEVEL_DATA("ReadBufferstatus size %d too large", bytes_back);
		return search_error;
	}
	devices_found = bytes_back / SERIAL_NUMBER_SIZE;
	if (devices_found > dir_gulp_elements) {
		devices_found = dir_gulp_elements;
	}

	// read in the buffer that holds the devices found
	if ((ret = DS9490_read(EP3.b, bytes_back, pn)) <= 0) {
		LEVEL_DATA("bulk read problem ret=%d", ret);
		return search_error;
	}

	// analyze each device found
	for (device_index = 0; device_index < devices_found; ++device_index) {
		/* test for CRC error */
		LEVEL_DEBUG("gulp. Adding element %d:" SNformat, device_index, SNvar(EP3.sn[device_index]));
		if (CRC8(EP3.sn[device_index], SERIAL_NUMBER_SIZE) != 0 || EP3.sn[device_index][0] == 0) {
			LEVEL_DATA("CRC error");
			return search_error;
		}
	}
	// all ok, so add the devices
	for (device_index = 0; device_index < devices_found; ++device_index) {
		DirblobAdd(EP3.sn[device_index], db);
	}
	ds->LastDiscrepancy = FindDiscrepancy(EP3.sn[devices_found-1], EP3.sn[devices_found]);
	ds->LastDevice = (bytes_back == devices_found * SERIAL_NUMBER_SIZE);	// no more to read

	return search_good;
}

static void SetupDiscrepancy(const struct device_search *ds, BYTE * discrepancy)
{
	int i;

	/** Play LastDescrepancy games with bitstream */
	memcpy(discrepancy, ds->sn, SERIAL_NUMBER_SIZE);	/* set buffer to zeros */

	if (ds->LastDiscrepancy > -1) {
		UT_setbit(discrepancy, ds->LastDiscrepancy, 1);
	}

	/* This could be more efficiently done than bit-setting, but probably wouldn't make a difference */
	for (i = ds->LastDiscrepancy + 1; i < 64; i++) {
		UT_setbit(discrepancy, i, 0);
	}
}

static int FindDiscrepancy(BYTE * last_sn, BYTE * discrepancy_sn)
{
	int i;
	for (i = 63; i >= 0; i--) {
		if ((UT_getbit(discrepancy_sn, i) != 0)
			&& (UT_getbit(last_sn, i) == 0)) {
			return i;
		}
	}
	return 0;
}

/* ------------------------------------------------------------ */
/* --- USB Open and Close --------------------------------------*/

/* Open usb device,
   unload ds9490r kernel module if it is interfering
   set all the interface magic
   set callback routines and adapter entries
*/
GOOD_OR_BAD DS9490_open_and_name(struct usb_list *ul, struct connection_in *in)
{
	if (in->master.usb.usb) {
		LEVEL_DEFAULT("DS9490 %s was NOT closed?", in->name);
		return gbBAD ;
	}

	SAFEFREE(in->name) ;
	in->name = DS9490_device_name(ul);

	if (in->name == NULL) {
		return gbBAD;
	}
	
	if ( BAD( DS9490_open( ul, in ) ) ) {
		STAT_ADD1_BUS(e_bus_open_errors, in);
		return gbBAD ;
	} else if ( BAD( DS9490_setup_adapter(in)) ) {
		LEVEL_DEFAULT("Error setting up USB DS9490 bus master at %s.", in->name);
		DS9490_close( in ) ;
		return gbBAD ;
	}
	DS9490_SetFlexParameters(in);

	return gbGOOD ;
}

/* ------------------------------------------------------------ */
/* --- USB read and write --------------------------------------*/

static GOOD_OR_BAD DS9490_sendback_data(const BYTE * data, BYTE * resp, size_t len, const struct parsedname *pn)
{
	size_t location = 0 ;

	while ( location < len ) {
		BYTE buffer[ DS9490_getstatus_BUFFER_LENGTH ];
		int readlen ;

		size_t block = len - location ;
		if ( block > USB_FIFO_EACH ) {
			block = USB_FIFO_EACH ;
		}

		if ( DS9490_write( &data[location], block, pn) < (int) block) {
			LEVEL_DATA("USBsendback bulk write problem");
			return gbBAD;
		}

		// COMM_BLOCK_IO | COMM_IM | COMM_F == 0x0075
		readlen = block ;
		if (( BAD( USB_Control_Msg(COMM_CMD, COMM_BLOCK_IO | COMM_IM | COMM_F, block, pn)) )
			|| ( DS9490_getstatus(buffer, &readlen, pn)  != BUS_RESET_OK )	// wait for len bytes
			) {
			LEVEL_DATA("USBsendback control error");
			STAT_ADD1_BUS(e_bus_errors, pn->selected_connection);
			return gbBAD;
		}

		if ( DS9490_read( &resp[location], block, pn) < 0) {
			LEVEL_DATA("USBsendback bulk read error");
			return gbBAD;
		}

		location += block ;
	}
	return gbGOOD;
}


/* ------------------------------------------------------------ */
/* --- USB Power byte for temperature conversion ---------------*/

// Send 8 bits of communication to the 1-Wire Net and read the
// 8 bits back from the 1-Wire Net.
// The parameter 'byte' least significant 8 bits are used.  After the
// 8 bits are sent change the level of the 1-Wire net.
// Delay delay msec and return to normal
static GOOD_OR_BAD DS9490_PowerByte(BYTE byte, BYTE * resp, UINT delay, const struct parsedname *pn)
{
	LEVEL_DATA("start");

	/* This is more likely to be the correct way to handle powerbytes */
	if ( BAD( USB_Control_Msg(COMM_CMD, COMM_BYTE_IO | COMM_IM | COMM_SPU, byte & 0xFF, pn)) ) {
		DS9490_HaltPulse(pn);
		return gbBAD ;
	} else if ( DS9490_read(resp, 1, pn) < 0) {
		/* Read back the result (may be the same as "byte") */
		DS9490_HaltPulse(pn);
		return gbBAD ;
	}
	/* Delay with strong pullup */
	UT_delay(delay);
	DS9490_HaltPulse(pn);
	return gbGOOD ;
}

/* ------------------------------------------------------------ */
/* --- USB Program Pulse ---------------------------------------*/

static GOOD_OR_BAD DS9490_ProgramPulse(const struct parsedname *pn)
{
	GOOD_OR_BAD ret;

	// set pullup to strong5 or program
	// set the strong pullup duration to infinite
	ret = USB_Control_Msg(COMM_CMD, COMM_PULSE | COMM_TYPE | COMM_IM, 0, pn);
	if ( GOOD(ret) ) {
		UT_delay_us(520);		// 520 usec (480 usec would be enough)
	}

	if ( BAD(DS9490_HaltPulse(pn)) ) {
		LEVEL_DEBUG("Couldn't reset the program pulse level back to normal");
		return gbBAD;
	}
	return ret ;
}

static GOOD_OR_BAD DS9490_HaltPulse(const struct parsedname *pn)
{
	BYTE buffer[ DS9490_getstatus_BUFFER_LENGTH ];
	struct timeval tv;
	struct timeval tvtarget;
	struct timeval tvlimit = { 0, 300000 } ; // 300 millisec from PDKit /ib/other/libUSB/libusbds2490.c

	if ( timernow( &tv ) < 0) {
		return gbBAD;
	}
	timeradd( &tv, &tvlimit, &tvtarget ) ;

	do {
		int readlen = -1 ;
		
		if ( BAD(USB_Control_Msg(CONTROL_CMD, CTL_HALT_EXE_IDLE, 0, pn)) ) {
			break;
		}
		if ( BAD(USB_Control_Msg(CONTROL_CMD, CTL_RESUME_EXE, 0, pn)) ) {
			break;
		}

		/* Can't wait for STATUSFLAGS_IDLE... just get first availalbe status flag */
		if ( DS9490_getstatus(buffer, &readlen, pn) != BUS_RESET_OK ) {
			break;
		}
		// check the SPU flag
		if (!(buffer[8] & STATUSFLAGS_SPUA)) {
			return gbGOOD;
		}
		if ( timernow( &tv ) < 0) {
			return gbBAD;
		}
	} while ( timercmp( &tv, &tvtarget, >) ) ;
	LEVEL_DATA("timeout");
	return gbBAD;
}

/* ------------------------------------------------------------ */
/* --- USB Various Parameters ----------------------------------*/

static GOOD_OR_BAD DS9490_SetSpeed(const struct parsedname *pn)
{
	struct connection_in * in = pn->selected_connection ;
	int ret = 0;

	// in case timeout value changed (via settings) -- sec -> msec
	in->master.usb.timeout = 1000 * Globals.timeout_usb;

	// Failed overdrive, switch to slow

	if (in->changed_bus_settings | CHANGED_USB_SPEED) {
		in->changed_bus_settings ^= CHANGED_USB_SPEED ;
		if (in->speed == bus_speed_overdrive) {
			if ( BAD(DS9490_overdrive(pn)) ) {
				++ ret; 
				in->speed = bus_speed_slow;
			} else {
				LEVEL_DATA("set overdrive speed");
			}
		} else if (in->flex==bus_yes_flex) {
			if ( BAD(USB_Control_Msg(MODE_CMD, MOD_1WIRE_SPEED, ONEWIREBUSSPEED_FLEXIBLE, pn)) ) {
				++ ret;
			} else {
				LEVEL_DATA("set flexible speed");
			}
		} else {
			if ( BAD(USB_Control_Msg(MODE_CMD, MOD_1WIRE_SPEED, ONEWIREBUSSPEED_REGULAR, pn)) ) {
				++ ret;
			} else {
				LEVEL_DATA("set regular speed");
			}
		}
	}
	if (in->changed_bus_settings | CHANGED_USB_SLEW) {
		in->changed_bus_settings ^= CHANGED_USB_SLEW ;
		/* Slew Rate */
		if ( BAD(USB_Control_Msg(MODE_CMD, MOD_PULLDOWN_SLEWRATE, in->master.usb.pulldownslewrate, pn)) ) {
			LEVEL_DATA("MOD_PULLDOWN_SLEWRATE error");
			++ret;
		}
	}
	if (in->changed_bus_settings | CHANGED_USB_LOW) {
		in->changed_bus_settings ^= CHANGED_USB_LOW ;
		/* Low Time */
		if ( BAD(USB_Control_Msg(MODE_CMD, MOD_WRITE1_LOWTIME, in->master.usb.writeonelowtime, pn)) ) {
			LEVEL_DATA("MOD_WRITE1_LOWTIME error");
			++ret;
		}
	}
	if (in->changed_bus_settings | CHANGED_USB_OFFSET) {
		in->changed_bus_settings ^= CHANGED_USB_OFFSET ;
		/* DS0 Low */
		if ( BAD(USB_Control_Msg(MODE_CMD, MOD_DSOW0_TREC, in->master.usb.datasampleoffset, pn)) ) {
			LEVEL_DATA("MOD_DS0W0 error");
			++ret;
		}
	}
	return ret>0 ? gbBAD : gbGOOD;
}

// Switch to overdrive speed -- 3 tries
static GOOD_OR_BAD DS9490_overdrive(const struct parsedname *pn)
{
	BYTE sp = _1W_OVERDRIVE_SKIP_ROM;
	BYTE resp;
	int i;

	// we need to change speed to overdrive
	for (i = 0; i < 3; i++) {
		LEVEL_DATA("set overdrive speed. Attempt %d",i);
		if ( BAD( gbRESET(BUS_reset(pn)) ) ) {
			continue;
		}
		if ( BAD( DS9490_sendback_data(&sp, &resp, 1, pn) ) || (_1W_OVERDRIVE_SKIP_ROM != resp) ) {
			LEVEL_DEBUG("error setting overdrive %.2X/0x%02X", _1W_OVERDRIVE_SKIP_ROM, resp);
			continue;
		}
		if ( GOOD( USB_Control_Msg(MODE_CMD, MOD_1WIRE_SPEED, ONEWIREBUSSPEED_OVERDRIVE, pn)) ) {
			LEVEL_DEBUG("speed is now set to overdrive");
			return gbGOOD;
		}
	}

	LEVEL_DEBUG("Error setting overdrive after 3 retries");
	return gbBAD;
}

static GOOD_OR_BAD DS9490_setup_adapter(struct connection_in * in)
{
	struct parsedname s_pn;
	struct parsedname * pn = &s_pn ;
	BYTE buffer[ DS9490_getstatus_BUFFER_LENGTH ];
	int readlen = 0 ;

	FS_ParsedName_Placeholder(pn);	// minimal parsename -- no destroy needed
	pn->selected_connection = in;

	// reset the device (not the 1-wire bus)
	if ( BAD(USB_Control_Msg(CONTROL_CMD, CTL_RESET_DEVICE, 0x0000, pn))) {
		LEVEL_DATA("ResetDevice error");
		return gbBAD;
	}
	// set the strong pullup duration to infinite
	if ( BAD( USB_Control_Msg(COMM_CMD, COMM_SET_DURATION | COMM_IM, 0x0000, pn)) ) {
		LEVEL_DATA("StrongPullup error");
		return gbBAD;
	}
	// set the 12V pullup duration to 512us
	if ( BAD( USB_Control_Msg(COMM_CMD, COMM_SET_DURATION | COMM_IM | COMM_TYPE, PROGRAM_PULSE_DURATION_CODE, pn)) ) {
		LEVEL_DATA("12VPullup error");
		return gbBAD;
	}
	// enable both program and strong pulses
	if ( BAD( USB_Control_Msg(MODE_CMD, MOD_PULSE_EN, ENABLE_PROGRAM_AND_PULSE, pn)) ) {
		LEVEL_DATA("EnableProgram error");
		return gbBAD;
	}
	// enable speed changes
	if ( BAD( USB_Control_Msg(MODE_CMD, MOD_SPEED_CHANGE_EN, 1, pn)) ) {
		LEVEL_DATA("SpeedEnable error");
		return gbBAD;
	}

	if ( DS9490_getstatus(buffer, &readlen, pn) != BUS_RESET_OK ) {
		LEVEL_DATA("getstatus failed error");
		return gbBAD;
	}

	return gbGOOD;
}

static void DS9490_SetFlexParameters(struct connection_in *in)
{
	/* in regular and overdrive speed, slew rate is 15V/us. It's only
	* suitable for short 1-wire busses. Use flexible speed instead. */

	// default values for bus-timing when using --altUSB
	if (Globals.altUSB) {
		in->master.usb.pulldownslewrate = PARMSET_Slew1p37Vus;
		in->master.usb.writeonelowtime = PARMSET_W1L_10us;
		in->master.usb.datasampleoffset = PARMSET_DS0_W0R_8us;
	} else {
		/* This seem to be the default value when reseting the ds2490 chip */
		in->master.usb.pulldownslewrate = PARMSET_Slew0p83Vus;
		in->master.usb.writeonelowtime = PARMSET_W1L_12us;
		in->master.usb.datasampleoffset = PARMSET_DS0_W0R_7us;
	}
	in->changed_bus_settings |= CHANGED_USB_SLEW | CHANGED_USB_LOW | CHANGED_USB_OFFSET ;
}

#endif							/* OW_USB */
