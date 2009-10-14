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

/* Extensive FreeBSD workarounds by Robert Nilsson <rnilsson@mac.com> */
#ifdef __FreeBSD__
	// Add a few definitions we need
#undef HAVE_USB_INTERRUPT_READ	// This call in libusb is unneeded for FreeBSD (and it's broken)
#include <dev/usb/usb.h>
struct usb_dev_handle {
	int file_descriptor;
	struct usb_bus *bus;
	struct usb_device *device;
	int config;
	int interface;
	int altsetting;
	void *impl_info;
};
#define USB_CLEAR_HALT BSD_usb_clear_halt
#else
#define USB_CLEAR_HALT usb_clear_halt
#endif

#include <usb.h>

/* All the rest of the code sees is the DS9490_detect routine and the iroutine structure */

struct usb_list {
	struct usb_bus *bus;
	struct usb_device *dev;
};

static int USB_init(struct usb_list *ul);
static int USB_next(struct usb_list *ul);
static int USB_Control_Msg(BYTE bRequest, UINT wValue, UINT wIndex, const struct parsedname *pn);
static int usbdevice_in_use(const struct connection_in * in_selected) ;
static char *DS9490_device_name(const struct usb_list *ul);

static int DS9490_reset(const struct parsedname *pn);

static int DS9490_open(struct usb_list *ul, struct connection_in *in);
static int DS9490_sub_open(struct usb_list *ul, struct connection_in *in);

static int DS9490_detect_low(struct connection_in *in);
static int DS9490_detect_found(struct connection_in *in);
static int DS9490_reconnect(const struct parsedname *pn);
static int DS9490_redetect_low(struct connection_in * in);
static int DS9490_redetect_found(struct connection_in * in);
static void DS9490_setroutines(struct connection_in *in);
static int DS9490_root_dir( struct dirblob * db, struct connection_in * in ) ;
static void DS9490_dir_callback( void * v, const struct parsedname * pn_entry );
static int DS9490_setup_adapter(struct connection_in * in) ;

int DS9490_getstatus(BYTE * buffer, int readlen, const struct parsedname *pn);
static int DS9490_next_both(struct device_search *ds, const struct parsedname *pn);
static int DS9490_sendback_data(const BYTE * data, BYTE * resp, size_t len, const struct parsedname *pn);
static int DS9490_HaltPulse(const struct parsedname *pn);
static int DS9490_PowerByte(BYTE byte, BYTE * resp, UINT delay, const struct parsedname *pn);
static int DS9490_ProgramPulse(const struct parsedname *pn);
static int DS9490_read(BYTE * buf, size_t size, const struct parsedname *pn);
static int DS9490_write(const BYTE * buf, size_t size, const struct parsedname *pn);
static int DS9490_overdrive(const struct parsedname *pn);
static void SetupDiscrepancy(const struct device_search *ds, BYTE * discrepancy);
static int FindDiscrepancy(BYTE * last_sn);
static int DS9490_directory(struct device_search *ds, struct dirblob *db, const struct parsedname *pn);
static int DS9490_SetSpeed(const struct parsedname *pn);
static void DS9490_SetFlexParameters(const struct parsedname *pn) ;

/* Device-specific routines */
static void DS9490_setroutines(struct connection_in *in)
{
	in->iroutines.detect = DS9490_detect;
	in->iroutines.reset = DS9490_reset;
	in->iroutines.next_both = DS9490_next_both;
	in->iroutines.PowerByte = DS9490_PowerByte;
	in->iroutines.ProgramPulse = DS9490_ProgramPulse;
	in->iroutines.sendback_data = DS9490_sendback_data;
	//    in->iroutines.sendback_bits = ;const struct parsedname *pn
	in->iroutines.select = NULL;
	in->iroutines.reconnect = DS9490_reconnect;
	in->iroutines.close = DS9490_close;
	in->iroutines.transaction = NULL;
	in->iroutines.flags = 0;

	in->bundling_length = USB_FIFO_SIZE;
}

//#define DS2490_DIR_GULP_ELEMENTS     ((64/8) - 1)
#define DS2490_DIR_GULP_ELEMENTS     (1)

#define CONTROL_REQUEST_TYPE  0x40

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
#define COMM_ICP                    0x0200t
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

#define ONEWIREDEVICEDETECT               0xA5
#define COMMCMDERRORRESULT_NRS            0x01
#define COMMCMDERRORRESULT_SH             0x02

// Device Status Flags
#define STATUSFLAGS_SPUA                       0x01	// if set Strong Pull-up is active
#define STATUSFLAGS_PRGA                       0x02	// if set a 12V programming pulse is being generated
#define STATUSFLAGS_12VP                       0x04	// if set the external 12V programming voltage is present
#define STATUSFLAGS_PMOD                       0x08	// if set the DS2490 powered from USB and external sources
#define STATUSFLAGS_HALT                       0x10	// if set the DS2490 is currently halted
#define STATUSFLAGS_IDLE                       0x20	// if set the DS2490 is currently idle

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

/** EP1 -- control read */
#define DS2490_EP1              0x81
/** EP2 -- bulk write */
#define DS2490_EP2              0x02
/** EP3 -- bulk read */
#define DS2490_EP3              0x83

/* From datasheet http://datasheets.maxim-ic.com/en/ds/DS2490.pdf page 15 */
/* 480 usec = 8usec * 60 and 60(decimal) = 0x3C */
/* 480 usec is the recommended program pulse duration in the DS2406 DS2502 DS2505 datasheets */
#define PROGRAM_PULSE_DURATION_CODE	0x3C

char badUSBname[] = "-1/-1";

#ifdef __FreeBSD__
// This is in here until the libusb on FreeBSD supports the usb_clear_halt function
int BSD_usb_clear_halt(usb_dev_handle * dev, unsigned int ep)
{
	int ret;
	struct usb_ctl_request ctl_req;

	ctl_req.ucr_addr = 0;		// Not used for this type of request
	ctl_req.ucr_request.bmRequestType = UT_WRITE_ENDPOINT;
	ctl_req.ucr_request.bRequest = UR_CLEAR_FEATURE;
	USETW(ctl_req.ucr_request.wValue, UF_ENDPOINT_HALT);
	USETW(ctl_req.ucr_request.wIndex, ep);
	USETW(ctl_req.ucr_request.wLength, 0);
	ctl_req.ucr_flags = 0;
	if ((ret = ioctl(dev->file_descriptor, USB_DO_REQUEST, &ctl_req)) < 0) {
		LEVEL_DATA("DS9490_clear_halt:  failed for %d", ep);
	}
	return ret;
}
#endif							/* __FreeBSD__ */

/* Step through USB devices (looking for the DS2490 chip) */
int DS9490_enumerate(void)
{
	struct usb_list ul;
	int ret = 0;
	USB_init(&ul);
	while (!USB_next(&ul))
		++ret;
		
	return ret;
}

/* Main routine for detecting (and setting up) the DS2490 1-wire USB chip */
int DS9490_detect(struct connection_in *in)
{
	int ret;

	DS9490_setroutines(in);		// set up close, reconnect, reset, ...
	in->name = owstrdup(badUSBname);		// initialized

	// store timeout value -- sec -> msec
	in->connin.usb.timeout = 1000 * Globals.timeout_usb;
	in->busmode = bus_usb;
	
	ret = DS9490_detect_low(in);
	if (ret) {
		fprintf(stderr, "Could not open the USB adapter. Is there a problem with permissions?\n");
		LEVEL_DEFAULT("Could not open the USB adapter. Is there a problem with permissions?\n");
	}
	return ret;
}

/* Open a DS9490  -- low level code (to allow for repeats)  */
static int DS9490_detect_low(struct connection_in * in)
{
	struct usb_list ul;
	/* usb_nr holds the number of the adapter */
	int useusb = in->connin.usb.usb_nr;
	int usbnum = 0;

	USB_init(&ul);
	while (!USB_next(&ul)) {
		if (++usbnum < useusb) {
			LEVEL_CONNECT("USB DS9490 %d passed over. (Looking for %d)\n", usbnum, useusb);
		} else if (DS9490_open(&ul, in)) {
			return -EIO;
		} else if (DS9490_detect_found(in)) {
			DS9490_close(in) ;
			LEVEL_CONNECT("USB DS9490 %d unsuccessful. (Looking for %d)\n", usbnum, useusb);
		} else{
			LEVEL_CONNECT("USB DS9490 %d/%d successful bound\n", usbnum, useusb);
			return 0 ;
		}
	}

	LEVEL_CONNECT("No available USB DS9490 adapter found\n");
	return -ENODEV;
}

static void DS9490_dir_callback( void * v, const struct parsedname * pn_entry )
{
	struct dirblob * db = v ;

	LEVEL_DEBUG("Callback on %s\n",SAFESTRING(pn_entry->path));
	if ( pn_entry->sn[0] != '\0' ) {
		DirblobAdd( pn_entry->sn, db ) ;
	}
}

static int DS9490_root_dir( struct dirblob * db, struct connection_in * in )
{
	ASCII path[PATH_MAX] ;
	struct parsedname pn_root ;
	int ret ;

	UCLIBCLOCK;
	snprintf(path, PATH_MAX, "/uncached/bus.%d", in->index);
	UCLIBCUNLOCK;

	if ( FS_ParsedName(path, &pn_root) ) {
		return 1 ;
	}
	DirblobInit( db ) ;

	/* First time pretend there are devices */
	pn_root.selected_connection->changed_bus_settings |= CHANGED_USB_SPEED ;	// Trigger needing new configuration
	pn_root.selected_connection->speed = bus_speed_slow;	// not overdrive at start
	pn_root.selected_connection->flex = Globals.usb_flextime ? bus_yes_flex : bus_no_flex ;
	
	SetReconnect(&pn_root) ;
	ret = FS_dir( DS9490_dir_callback, db, &pn_root ) ;
	LEVEL_DEBUG("Finished FS_dir\n");
	FS_ParsedName_destroy(&pn_root) ;
	
	
	if ( ret ) {
		DirblobClear(db) ;
		return 1 ;
	}
	return 0 ;
	// Dirblob must be cleared by recipient.
}

/* Found a DS9490 that seems good, now check list and find a device to ID for reconnects */
static int DS9490_detect_found(struct connection_in *in)
{
	struct dirblob db ;
	BYTE sn[8] ;
	int device_number ;
	
	if ( DS9490_root_dir( &db, in ) ) {
		LEVEL_DATA("Cannot get root directory on [%s] (Probably non-DS9490 device and empty bus).\n", SAFESTRING(in->name));
		return -EIO ;
	}


	// Use 0x00 if no devices (homegrown adapters?)
	if ( DirblobElements( &db) == 0 ) {
		DirblobClear( &db ) ;
		memset( in->connin.usb.ds1420_address, 0, 8 ) ;
		LEVEL_DEFAULT("Set DS9490 %s unique id 0x00 (no devices at all)\n", SAFESTRING(in->name)) ;
		return 0 ;
	}
	
	// look for the special 0x81 device
	device_number = 0 ;
	while ( DirblobGet( device_number, sn, &db ) == 0 ) {
		if (sn[0] == 0x81) {	// 0x81 family code
			memcpy(in->connin.usb.ds1420_address, sn, 8);
			LEVEL_DEFAULT("Set DS9490 %s unique id to " SNformat "\n", SAFESTRING(in->name), SNvar(in->connin.usb.ds1420_address));
			DirblobClear( &db ) ;
			return 0 ;
		}
		++device_number ;
	}

	// look for the (less specific, but older DS9490s) 0x01 device
	device_number = 0 ;
	while ( DirblobGet( device_number, sn, &db ) == 0 ) {
		if (sn[0] == 0x01) {	// 0x01 family code
			memcpy(in->connin.usb.ds1420_address, sn, 8);
			LEVEL_DEFAULT("Set DS9490 %s unique id to " SNformat "\n", SAFESTRING(in->name), SNvar(in->connin.usb.ds1420_address));
			DirblobClear( &db ) ;
			return 0 ;
		}
		++device_number ;
	}

	// Take the first device, whatever it is
	DirblobGet( 0, sn, &db ) ;
	memcpy(in->connin.usb.ds1420_address, sn, 8);
	LEVEL_DEFAULT("Set DS9490 %s unique id to " SNformat "\n", SAFESTRING(in->name), SNvar(in->connin.usb.ds1420_address));
	DirblobClear( &db ) ;
	return 0;
}

static int DS9490_setup_adapter(struct connection_in * in)
{
	struct parsedname s_pn;
	struct parsedname * pn = &s_pn ;
	BYTE buffer[32];
	int ret;

	FS_ParsedName(NULL, pn);	// minimal parsename -- no destroy needed
	pn->selected_connection = in;

	// reset the device (not the 1-wire bus)
	if ((ret = USB_Control_Msg(CONTROL_CMD, CTL_RESET_DEVICE, 0x0000, pn)) < 0) {
		LEVEL_DATA("ResetDevice ret=%d\n", ret);
		return -EIO;
	}
	// set the strong pullup duration to infinite
	if ((ret = USB_Control_Msg(COMM_CMD, COMM_SET_DURATION | COMM_IM, 0x0000, pn)) < 0) {
		LEVEL_DATA("StrongPullup ret=%d\n", ret);
		return -EIO;
	}
	// set the 12V pullup duration to 512us
	if ((ret = USB_Control_Msg(COMM_CMD, COMM_SET_DURATION | COMM_IM | COMM_TYPE, PROGRAM_PULSE_DURATION_CODE, pn)) < 0) {
		LEVEL_DATA("12VPullup ret=%d\n", ret);
		return -EIO;
	}
	// enable both program and strong pulses
	if ((ret = USB_Control_Msg(MODE_CMD, MOD_PULSE_EN, ENABLE_PROGRAM_AND_PULSE, pn)) < 0) {
		LEVEL_DATA("EnableProgram ret=%d\n", ret);
		return -EIO;
	}
	// enable speed changes
	if ((ret = USB_Control_Msg(MODE_CMD, MOD_SPEED_CHANGE_EN, 1, pn)) < 0) {
		LEVEL_DATA("SpeedEnable ret=%d\n", ret);
		return -EIO;
	}

	if ((ret = DS9490_getstatus(buffer, 0, pn)) < 0) {
		LEVEL_DATA("getstatus failed ret=%d\n", ret);
		return ret;
	}

	LEVEL_DATA("done (ret=%d)\n", ret);
	return 0;
}


/* Open usb device,
   unload ds9490r kernel module if it is interfering
   set all the interface magic
   set callback routines and adapter entries
*/
static int DS9490_open(struct usb_list *ul, struct connection_in *in)
{
	int ret ;

	if (in->name) {
		owfree(in->name);
		in->name = NULL;
	}

	if (in->connin.usb.usb) {
		LEVEL_DEFAULT("usb.usb was NOT closed before DS9490_open() ?\n");
		return -ENODEV ;
	}

	in->name = DS9490_device_name(ul);

	if (in->name == NULL) {
		return -ENOMEM;
	}

	ret = DS9490_sub_open( ul, in ) ;
	if ( ret ) {
		owfree(in->name);
		in->name = owstrdup(badUSBname) ;
		in->connin.usb.dev = NULL;	// this will force a re-scan next time
		//LEVEL_CONNECT("Failed to open USB DS9490 adapter %s\n", name);
		STAT_ADD1_BUS(e_bus_open_errors, in);
		return ret ;
	}

	return 0 ;
}

static int DS9490_sub_open(struct usb_list *ul, struct connection_in *in)
{
	int ret = ENODEV;
	usb_dev_handle *usb;

	in->connin.usb.dev = ul->dev;

	if (in->connin.usb.dev ==NULL ) {
		return -ENODEV ;
	}

	usb = usb_open(in->connin.usb.dev) ;
	if (usb == NULL) {
		return -ENODEV ;
	}

	in->connin.usb.usb = usb;
	#ifdef LIBUSB_HAS_DETACH_KERNEL_DRIVER_NP
	usb_detach_kernel_driver_np(usb, 0);
	#endif							/* LIBUSB_HAS_DETACH_KERNEL_DRIVER_NP */

	if ((ret = usb_set_configuration(usb, 1))) {
		LEVEL_CONNECT("Failed to set configuration on USB DS9490 adapter at %s.\n", in->name);
	} else if ((ret = usb_claim_interface(usb, 0))) {
		LEVEL_CONNECT("Failed to claim interface on USB DS9490 adapter at %s. ret=%d\n", in->name, ret);
	} else {
		if ((ret = usb_set_altinterface(usb, 3))) {
			LEVEL_CONNECT("Failed to set alt interface on USB DS9490 adapter at %s.\n", in->name);
		} else {
			LEVEL_DEFAULT("Opened USB DS9490 adapter at %s.\n", in->name);
			DS9490_setroutines(in);
			in->Adapter = adapter_DS9490;	/* OWFS assigned value */
			in->adapter_name = "DS9490";

			// clear endpoints
			if ((ret = (USB_CLEAR_HALT(usb, DS2490_EP3) || USB_CLEAR_HALT(usb, DS2490_EP2) || USB_CLEAR_HALT(usb, DS2490_EP1)))) {
				LEVEL_DEFAULT("DS9490_open: USB_CLEAR_HALT failed ret=%d\n", ret);
			} else if ((ret = DS9490_setup_adapter(in))) {
				LEVEL_DEFAULT("Error setting up USB DS9490 adapter at %s.\n", in->name);
			} else {		/* All GOOD */
				return 0;
			}
		}
		usb_release_interface(usb, 0);
	}
	usb_close(usb);
	in->connin.usb.usb = NULL;

	return ret;
}

/* When the errors stop the USB device from functioning -- close and reopen.
 * If it fails, re-scan the USB bus and search for the old adapter */
static int DS9490_reconnect(const struct parsedname *pn)
{
	int ret;
	struct connection_in * in = pn->selected_connection ;
	
	/* Have to protect usb_find_busses() and usb_find_devices() with
	 * a lock since libusb could crash if 2 threads call it at the same time.
	 * It's not called until DS9490_redetect_low(), but I lock here just
	 * to be sure DS9490_close() and DS9490_open() get one try first. */
	LIBUSBLOCK;
	DS9490_close( in) ;
	if (DS9490_redetect_low(in)==0) {
		LEVEL_DEFAULT("Found USB DS9490 adapter after USB rescan as [%s]\n", SAFESTRING(in->name));
		ret = 0;
	} else {
		ret = -EIO;
	}
	LIBUSBUNLOCK;
	return ret;
}

static int USB_init(struct usb_list *ul)
{
	usb_init();
	usb_find_busses();
	usb_find_devices();
	ul->bus = usb_get_busses();
	ul->dev = NULL;
	return 0;
}

static int USB_next(struct usb_list *ul)
{
	while (ul->bus) {
		// First pass, look for next device
		if (ul->dev == NULL) {
			ul->dev = ul->bus->devices;
		} else {				// New bus, find first device
			ul->dev = ul->dev->next;
		}
		if (ul->dev) {			// device found
			if (ul->dev->descriptor.idVendor != 0x04FA || ul->dev->descriptor.idProduct != 0x2490) {
				continue;		// not DS9490
			}
			LEVEL_CONNECT("Adapter found: %s/%s\n", ul->bus->dirname, ul->dev->filename);
			return 0;
		} else {
			ul->bus = ul->bus->next;
			ul->dev = NULL;
		}
	}
	return 1;
}

static int usbdevice_in_use(const struct connection_in * in_selected)
{
	struct connection_in *in;

	for (in = Inbound_Control.head; in != NULL; in = in->next) {
		if ( in == in_selected ) {
			continue ;
		}
		if ( in->busmode != bus_usb ) {
			continue ;
		}
		if ( in->name == NULL ) {
			continue ;
		}
		LEVEL_DEBUG("Comparing %s with bus.%d %s\n",in_selected->name,in->index,SAFESTRING(in->name));
		if ( strcmp(in->name, in_selected->name) == 0 ) {
			return 1;			// It seems to be in use already
		}
	}
	return 0;					// not found in the current inbound list
}

/* Construct the device name */
/* Return NULL if there is a problem */
static char *DS9490_device_name(const struct usb_list *ul)
{
	size_t len = 2 * PATH_MAX + 1;
	char name[len + 1];
	char *ret = NULL;
	int sn_ret ;
	UCLIBCLOCK ;
	sn_ret = snprintf(name, len, "%s/%s", ul->bus->dirname, ul->dev->filename) ;
	UCLIBCUNLOCK ;
	if (sn_ret > 0) {
		name[len] = '\0';		// make sufre there is a trailing null
		ret = owstrdup(name);
	}
	return ret;
}

/* Open a DS9490  -- low level code (to allow for repeats)  */
static int DS9490_redetect_low(struct connection_in * in)
{
	struct usb_list ul;
	
	//LEVEL_CONNECT("DS9490_redetect_low: name=%s\n", pn->selected_connection->name);
	
	/*
	* I don't think we need to call usb_init() or usb_find_busses() here.
	* They are already called once in DS9490_detect_low().
	* usb_init() is more like a os_init() function and there are probably
	* no new USB-busses added after rebooting kernel... or?
	* It doesn't seem to leak any memory when calling them, so they are
	* still called.
	*/
	USB_init(&ul);
	
	while (USB_next(&ul)==0) {
		// try to open the DS9490
		if (DS9490_open(&ul, in)) {
			LEVEL_CONNECT("Cant open USB adapter, Find next...\n");
			continue;
		}

		if ( DS9490_redetect_found( in ) == 0 ) {
			return 0 ;
		}
		DS9490_close(in);
	}
	//LEVEL_CONNECT("No available USB DS9490 adapter found\n");
	return -ENODEV;
	
}

/* Open a DS9490  -- low level code (to allow for repeats)  */
static int DS9490_redetect_found( struct connection_in * in)
{
	struct dirblob db ;
	BYTE sn[8] ;
	int device_number ;

	LEVEL_DEBUG("Attempting reconnect on %s\n",SAFESTRING(in->name));
	if (usbdevice_in_use(in)) {
		return EAGAIN ;
	}

	// Special case -- originally untagged adapter
	if ( in->connin.usb.ds1420_address[0] == '\0' ) {
		LEVEL_CONNECT("Since originally untagged adapter, we will use first available slot.\n");
		return 0 ;
	}

	// Generate a root directory
	if ( DS9490_root_dir( &db, in ) ) {
		LEVEL_DATA("Cannot get root directory on [%s] (Probably non-DS9490 device and empty bus).\n", SAFESTRING(in->name));
		return -EAGAIN ;
	}

	// This adapter has no tags, so not the one we want
	if ( DirblobElements( &db) == 0 ) {
		DirblobClear( &db ) ;
		LEVEL_DATA("Empty directory on [%s] (Doesn't match initial scan).\n", SAFESTRING(in->name));
		return -EAGAIN ;
	}

	// Scan directory for a match to the original tag
	device_number = 0 ;
	while ( DirblobGet( device_number, sn, &db ) == 0 ) {
		if (memcmp(sn, in->connin.usb.ds1420_address, 8) == 0) {	// same tag device?
			LEVEL_DATA("Matching device [%s].\n", SAFESTRING(in->name));
			DirblobClear( &db ) ;
			return 0 ;
		}
		++device_number ;
	}
	// Couldn't find correct ds1420 chip on this adapter
	LEVEL_CONNECT("Couldn't find correct ds1420 chip on this adapter [%s] (want: " SNformat ")\n", SAFESTRING(in->name), SNvar(in->connin.usb.ds1420_address));
	DirblobClear( &db ) ;
	return -ENODEV;
}

void DS9490_close(struct connection_in *in)
{
	usb_dev_handle *usb = in->connin.usb.usb;

	if (usb) {
		int ret = usb_release_interface(usb, 0);
		if (ret) {
			in->connin.usb.dev = NULL;	// force a re-scan
			LEVEL_CONNECT("Release interface failed ret=%d\n", ret);
		}

		/* It might already be closed? (returning -ENODEV)
		 * I have seen problem with calling usb_close() twice, so we
		 * might perhaps skip it if usb_release_interface() fails */
		ret = usb_close(usb);
		if (ret) {
			in->connin.usb.dev = NULL;	// force a re-scan
			LEVEL_CONNECT("usb_close() failed ret=%d\n", ret);
		}
		LEVEL_CONNECT("Closed USB DS9490 adapter at %s. ret=%d\n", in->name, ret);
	}
	in->connin.usb.usb = NULL;
	in->connin.usb.dev = NULL;
	if (in->name) {
		owfree(in->name);
	}
	in->name = owstrdup(badUSBname);
}

/* DS9490_getstatus()
   return -1          if Short detected on 1-wire bus
   return -ETIMEDOUT  on timeout (will require DS9490_close())
   return -EIO        on read errors (will require DS9490_close())
   otherwise return number of status bytes in buffer
*/
int DS9490_getstatus(BYTE * buffer, int readlen, const struct parsedname *pn)
{
	int ret, loops = 0;
	int i;
	struct connection_in * in = pn->selected_connection ;
	usb_dev_handle *usb = in->connin.usb.usb;
	
	// Pretty ugly, but needed for SUSE at least
	// Try both HAVE_USB_INTERRUPT_READ
#ifdef HAVE_USB_INTERRUPT_READ
	static int have_usb_interrupt_read = (1 == 1);
#else							/* HAVE_USB_INTERRUPT_READ */
	static int have_usb_interrupt_read = (1 == 0);
#endif							/* HAVE_USB_INTERRUPT_READ */
	static int count_have_usb_interrupt_read = 0;

	memset(buffer, 0, 32);		// should not be needed
	//LEVEL_DETAIL("DS9490_getstatus: readlen=%d\n", readlen);

#ifdef __FreeBSD__				// Clear the Interrupt read buffer before trying to get status
	{
		char junk[1500];
		if ((ret = usb_bulk_read(usb, DS2490_EP1, (ASCII *) junk, (size_t) 1500, in->connin.usb.timeout)) < 0) {
			STAT_ADD1_BUS(e_bus_status_errors, in);
			LEVEL_DATA("error reading ret=%d\n", ret);
			return -EIO;
		}
	}
#endif							// __FreeBSD__
	do {
		// Fix from Wim Heirman -- kernel 2.6 is fussier about endpoint type
#ifdef HAVE_USB_INTERRUPT_READ
		if (have_usb_interrupt_read) {
			if ((ret = usb_interrupt_read(usb, DS2490_EP1, (ASCII *) buffer, (size_t) 32, in->connin.usb.timeout)) < 0) {
				LEVEL_DATA("(HAVE_USB_INTERRUPT_READ) error reading ret=%d\n", ret);
			}
		} else
#endif
		if ((ret = usb_bulk_read(usb, DS2490_EP1, (ASCII *) buffer, (size_t) 32, in->connin.usb.timeout)) < 0) {
			LEVEL_DATA("(no HAVE_USB_INTERRUPT_READ) error reading ret=%d\n", ret);
		}

		if (ret < 0) {
			if (count_have_usb_interrupt_read != 0) {
				STAT_ADD1_BUS(e_bus_status_errors, in);
				return -EIO;
			}
			have_usb_interrupt_read = !have_usb_interrupt_read;
			count_have_usb_interrupt_read = 1;
			continue;
		}
		if (ret > 16) {
			if (ret == 32) {	// FreeBSD buffers the input, so this could just be two readings
				if (!memcmp(buffer, &buffer[16], 6)) {
					memmove(buffer, &buffer[16], 16);
					ret = 16;
					LEVEL_DATA("Corrected buffer 32 byte read\n");
				}
			}
			for (i = 16; i < ret; i++) {
				BYTE val = buffer[i];
				if (val != ONEWIREDEVICEDETECT) {
					LEVEL_DATA("Status byte[%X]: %X\n", i - 16, val);
				}
				if (val & COMMCMDERRORRESULT_SH) {	// short detected
					LEVEL_DATA("short detected\n");
					return -1;
				}
			}
		}

		if (readlen < 0) {
			break;				/* Don't wait for STATUSFLAGS_IDLE if length==-1 */
		}

		if (buffer[8] & STATUSFLAGS_IDLE) {
			if (readlen > 0) {
				// we have enough bytes to read now!
				// buffer[13] == (ReadBufferStatus)
				if (buffer[13] >= readlen) {
					break;
				}
			} else
				break;
		}
		// this value might be decreased later...
		if (++loops > 100) {
			LEVEL_DATA("never got idle  StatusFlags=%X read=%X\n", buffer[8], buffer[13]);
			return -ETIMEDOUT;	// adapter never got idle
		}
		/* Since result seem to be on the usb bus very quick, I sleep
		 * sleep 0.1ms or something like that instead... It seems like
		 * result is there after 0.2-0.3ms
		 */
		UT_delay_us(100);
	} while (1);

	//printf("DS9490_getstatus: read %d bytes\n", ret);
	//printf("  "SNformat"\n",SNvar(&(buffer[ 0])));
	//printf("  "SNformat"\n",SNvar(&(buffer[ 8])));

	if (ret < 16) {
		LEVEL_DATA("incomplete packet ret=%d\n", ret);
		return -EIO;			// incomplete packet??
	}
	return (ret - 16);			// return number of status bytes in buffer
}

// Switch to overdrive speed -- 3 tries
static int DS9490_overdrive(const struct parsedname *pn)
{
	int ret;
	BYTE sp = _1W_OVERDRIVE_SKIP_ROM;
	BYTE resp;
	int i;

	LEVEL_DATA("set overdrive speed\n");

	// we need to change speed to overdrive
	for (i = 0; i < 3; i++) {
		if ((ret = BUS_reset(pn)) < 0) {
			continue;
		}
		if (((ret = DS9490_sendback_data(&sp, &resp, 1, pn)) < 0)
			|| (_1W_OVERDRIVE_SKIP_ROM != resp)) {
			LEVEL_DEBUG("error sending ret=%d %.2X/0x%02X\n", ret, _1W_OVERDRIVE_SKIP_ROM, resp);
			continue;
		}
		if ((ret = USB_Control_Msg(MODE_CMD, MOD_1WIRE_SPEED, ONEWIREBUSSPEED_OVERDRIVE, pn)) == 0) {
			LEVEL_DEBUG("speed is now set to overdrive\n");
			return 0;
		}
	}

	LEVEL_DEBUG("Error setting overdrive after 3 retries\n");
	return -EIO;
}

/* Reset adapter and detect devices on the bus */
/* BUS locked at high level */
/* return 1=short, 0 good <0 error */
static int DS9490_reset(const struct parsedname *pn)
{
	int i, ret;
	BYTE buffer[32];
	int USpeed;
	struct connection_in * in = pn->selected_connection ;
	
	//printf("9490RESET\n");
	//printf("DS9490_reset() index=%d in->Adapter=%d %s\n", in->index, in->Adapter, in->adapter_name);

	LEVEL_DATA("start\n");

	if (in->connin.usb.usb == NULL || in->connin.usb.dev == NULL) {
		return -EIO;
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

	if ((ret = USB_Control_Msg(COMM_CMD, COMM_1_WIRE_RESET | COMM_F | COMM_IM | COMM_SE, USpeed, pn)) < 0) {
		LEVEL_DATA("error sending reset ret=%d\n", ret);
		return -EIO;			// fatal error... probably closed usb-handle
	}

	if (in->ds2404_compliance && (in->speed == bus_speed_slow)) {
		// extra delay for alarming DS1994/DS2404 complience
		UT_delay(5);
	}

	if ((ret = DS9490_getstatus(buffer, 0, pn)) < 0) {
		if (ret == -1) {
			/* Short detected, but otherwise no bigger "problem"?
			 * Make sure 1-wires won't be scanned */
			LEVEL_DATA("short detected\n", ret);
			return BUS_RESET_SHORT;
		}
		LEVEL_DATA("getstatus failed ret=%d\n", ret);
		return ret;
	}
	//USBpowered = (buffer[8]&STATUSFLAGS_PMOD) == STATUSFLAGS_PMOD ;
	in->AnyDevices = anydevices_yes ;
	for (i = 0; i < ret; i++) {
		BYTE val = buffer[16 + i];
		//LEVEL_DATA("Status bytes[%d]: %X\n", i, val);
		if (val != ONEWIREDEVICEDETECT) {
			// check for NRS bit (0x01)
			if (val & COMMCMDERRORRESULT_NRS) {
				// empty bus detected, no presence pulse detected
				in->AnyDevices = anydevices_no;
				LEVEL_DATA("no presense pulse detected\n");
			}
		}
	}
	LEVEL_DATA("ok\n");
	return BUS_RESET_OK;
}

static int DS9490_read(BYTE * buf, size_t size, const struct parsedname *pn)
{
	int ret;
	struct connection_in * in = pn->selected_connection ;
	usb_dev_handle *usb = in->connin.usb.usb;
	//printf("DS9490_read\n");
	if ((ret = usb_bulk_read(usb, DS2490_EP3, (ASCII *) buf, (int) size, in->connin.usb.timeout)) > 0) {
		return ret;
	}
	LEVEL_DATA("failed ret=%d\n", ret);
	USB_CLEAR_HALT(usb, DS2490_EP3);
	STAT_ADD1_BUS(e_bus_read_errors, in);
	return ret;
}

/* Fills the EP2 buffer in the USB adapter
   returns number of bytes (size)
   or <0 for an error */
static int DS9490_write(const BYTE * buf, size_t size, const struct parsedname *pn)
{
	int ret;
	struct connection_in * in = pn->selected_connection ;
	usb_dev_handle *usb = in->connin.usb.usb;
	//printf("DS9490_write\n");

	if (size == 0) {
		return 0;
	}

	if ((ret = usb_bulk_write(usb, DS2490_EP2, (ASCII *) buf, (int) size, in->connin.usb.timeout)) > 0) {
		return ret;
	}
	LEVEL_DATA("failed ret=%d\n", ret);
	USB_CLEAR_HALT(usb, DS2490_EP2);
	STAT_ADD1_BUS(e_bus_write_errors, in);
	return ret;
}

static int DS9490_sendback_data(const BYTE * data, BYTE * resp, size_t len, const struct parsedname *pn)
{
	int ret = 0;
	BYTE buffer[32];

	if (len > USB_FIFO_EACH) {
		return DS9490_sendback_data(data, resp, USB_FIFO_EACH, pn)
			|| DS9490_sendback_data(&data[USB_FIFO_EACH], &resp[USB_FIFO_EACH], len - USB_FIFO_EACH, pn);
	}

	if ((ret = DS9490_write(data, (size_t) len, pn)) < (int) len) {
		LEVEL_DATA("USBsendback bulk write problem ret=%d\n", ret);
		return ret;
	}
	// COMM_BLOCK_IO | COMM_IM | COMM_F == 0x0075
	if (((ret = USB_Control_Msg(COMM_CMD, COMM_BLOCK_IO | COMM_IM | COMM_F, len, pn)) < 0)
		|| ((ret = DS9490_getstatus(buffer, len, pn)) < 0)	// wait for len bytes
		) {
		LEVEL_DATA("USBsendback control problem ret=%d\n", ret);
		STAT_ADD1_BUS(e_bus_errors, pn->selected_connection);
		return ret;
	}

	if ((ret = DS9490_read(resp, (size_t) len, pn)) < 0) {
		LEVEL_DATA("USBsendback bulk read problem ret=%d\n", ret);
		return ret;
	}
	return 0;
}

/*
 * return 0       if success
 * return -ENODEV if no more devices
 * return -ENOENT if no devices at all
 * return -EIO    on errors
 */

static int DS9490_next_both(struct device_search *ds, const struct parsedname *pn)
{
	struct dirblob *db = (ds->search == _1W_CONDITIONAL_SEARCH_ROM) ?
		&(pn->selected_connection->alarm) : &(pn->selected_connection->main);
	int ret;
	int dir_gulp_elements = (pn->pathlength==0) ? DS2490_DIR_GULP_ELEMENTS : 1 ;

	if (pn->selected_connection->AnyDevices == anydevices_no) {
		return -ENODEV;
	}

	// LOOK FOR NEXT ELEMENT
	++ds->index;

	LEVEL_DEBUG("Index %d\n", ds->index);

	if (ds->index % dir_gulp_elements == 0) {
		if (ds->LastDevice) {
			return -ENODEV;
		}
		if ((ret = DS9490_directory(ds, db, pn))) {
			return ret;
		}
	}

	if ((ret = DirblobGet(ds->index % dir_gulp_elements, ds->sn, db))) {
		return ret;
	}

	/* test for special device families */
	switch (ds->sn[0]) {
	case 0x00:
		LEVEL_DATA("NULL family found\n");
		return -EIO;
	case 0x04:
	case 0x84:
		/* We found a DS1994/DS2404 which require longer delays */
		pn->selected_connection->ds2404_compliance = 1;
		break;
	default:
		break;
	}
	LEVEL_DEBUG("SN found: " SNformat "\n", SNvar(ds->sn));
	return 0;
}

// Read up to 7 (DS2490_DIR_GULP_ELEMENTS) at a time, and  place into
// a dirblob. Called from DS9490_next_both every 7 devices to fill.
static int DS9490_directory(struct device_search *ds, struct dirblob *db, const struct parsedname *pn)
{
	BYTE buffer[32];
	BYTE *cb = pn->selected_connection->combuffer;
	int ret;
	int bytes_back;
	int devices_found;
	int device_index;
	int dir_gulp_elements = (pn->pathlength==0) ? DS2490_DIR_GULP_ELEMENTS : 1 ;
	
	DirblobClear(db);

	if ( BUS_select(pn) ) {
		LEVEL_DEBUG("Selection problem before a directory listing\n") ;
		return -EIO ;
	}
	// Note that USB_FIFO_SIZE >>= 64

	/* DS1994/DS2404 might need an extra reset */
	if (pn->selected_connection->ExtraReset) {
		if (BUS_reset(pn) < 0) {
			return -EIO;
		}
		pn->selected_connection->ExtraReset = 0;
	}

	SetupDiscrepancy(ds, cb);

	if ((ret = DS9490_write(cb, 8, pn)) < 8) {
		LEVEL_DATA("bulk write problem = %d\n", ret);
		return -EIO;
	}
	if ((ret = USB_Control_Msg(COMM_CMD, COMM_SEARCH_ACCESS | COMM_IM | COMM_SM | COMM_F | COMM_RTS, (dir_gulp_elements << 8) | (ds->search), pn) ) < 0) {
		LEVEL_DATA("control problem ret=%d\n", ret);
		return -EIO;
	}

	if ((ret = DS9490_getstatus(buffer, 0, pn)) < 0) {
		LEVEL_DATA("getstatus error\n");
		return -EIO;
	}

	bytes_back = buffer[13];
	LEVEL_DEBUG("Got %d bytes from USB search\n", bytes_back);
	if (bytes_back == 0) {
		/* Nothing found on the bus. Have to return something != 0 to avoid
		 * getting stuck in loop in FS_realdir() and FS_alarmdir()
		 * which ends when ret!=0 */
		LEVEL_DATA("ReadBufferstatus == 0\n");
		return -ENODEV;
	} else if ((bytes_back % 8 != 0)
		|| (bytes_back > (dir_gulp_elements + 1) * 8)) {
		LEVEL_DATA("ReadBufferstatus %d not valid\n", bytes_back);
		return -EIO;
	}

	devices_found = bytes_back / 8;
	if (devices_found > dir_gulp_elements) {
		devices_found = dir_gulp_elements;
	}

	if ((ret = DS9490_read(cb, bytes_back, pn)) <= 0) {
		LEVEL_DATA("bulk read problem ret=%d\n", ret);
		return -EIO;
	}

	for (device_index = 0; device_index < devices_found; ++device_index) {
		BYTE sn[8];
		memcpy(sn, &cb[device_index * 8], 8);
		/* test for CRC error */
		LEVEL_DEBUG("gulp. Adding element %d:" SNformat "\n", device_index, SNvar(&cb[device_index * 8]));
		if (CRC8(sn, 8) != 0 || sn[0] == 0) {
			LEVEL_DATA("CRC error\n");
			return -EIO;
		}
		DirblobAdd(sn, db);
	}
	ds->LastDiscrepancy = FindDiscrepancy(&cb[bytes_back - 16]);
	ds->LastDevice = (bytes_back == devices_found * 8);	// no more to read

	return 0;
}

static void SetupDiscrepancy(const struct device_search *ds, BYTE * discrepancy)
{
	int i;

	/** Play LastDescrepancy games with bitstream */
	memcpy(discrepancy, ds->sn, 8);	/* set buffer to zeros */

	if (ds->LastDiscrepancy > -1) {
		UT_setbit(discrepancy, ds->LastDiscrepancy, 1);
	}

	/* This could be more efficiently done than bit-setting, but probably wouldn't make a difference */
	for (i = ds->LastDiscrepancy + 1; i < 64; i++) {
		UT_setbit(discrepancy, i, 0);
	}
}

static int FindDiscrepancy(BYTE * last_sn)
{
	int i;
	BYTE *discrepancy_sn = last_sn + 8;
	for (i = 63; i >= 0; i--) {
		if ((UT_getbit(discrepancy_sn, i) != 0)
			&& (UT_getbit(last_sn, i) == 0)) {
			return i;
		}
	}
	return 0;
}

//--------------------------------------------------------------------------
// Send 8 bits of communication to the 1-Wire Net and read the
// 8 bits back from the 1-Wire Net.
// The parameter 'byte' least significant 8 bits are used.  After the
// 8 bits are sent change the level of the 1-Wire net.
// Delay delay msec and return to normal
//
/* Returns 0=goodDS9490_SetSpeed
   bad = -EIO
 */
static int DS9490_PowerByte(BYTE byte, BYTE * resp, UINT delay, const struct parsedname *pn)
{
	int ret;

	LEVEL_DATA("start\n");

	/* This is more likely to be the correct way to handle powerbytes */
	if ((ret = USB_Control_Msg(COMM_CMD, COMM_BYTE_IO | COMM_IM | COMM_SPU, byte & 0xFF, pn)) < 0) {
		LEVEL_DATA("Error usb_control_msg 4\n");
	} else if ((ret = DS9490_read(resp, 1, pn)) < 0) {
		/* Read back the result (may be the same as "byte") */
		LEVEL_DATA("Error DS9490_read ret=%d\n", ret);
	} else {
		/* Delay with strong pullup */
		UT_delay(delay);
		ret = 0;
	}
	DS9490_HaltPulse(pn);
	return ret;
}

static int DS9490_ProgramPulse(const struct parsedname *pn)
{
	int ret;

	// set pullup to strong5 or program
	// set the strong pullup duration to infinite
	ret = USB_Control_Msg(COMM_CMD, COMM_PULSE | COMM_TYPE | COMM_IM, 0, pn);
	if (ret == 0) {
		UT_delay_us(520);		// 520 usec (480 usec would be enough)
	}

	if (DS9490_HaltPulse(pn) != 0) {
		LEVEL_DEBUG("Couldn't reset the program pulse level back to normal\n");
		return -EIO;
	}
	return ret;
}

static int DS9490_HaltPulse(const struct parsedname *pn)
{
	BYTE buffer[32];
	struct timeval tv;
	time_t endtime, now;
	int ret;

	LEVEL_DATA("start\n");

	if (gettimeofday(&tv, NULL) < 0) {
		return -1;
	}
	endtime = (tv.tv_sec & 0xFFFF) * 1000 + tv.tv_usec / 1000 + 300;

	do {
		LEVEL_DATA("loop\n");

		if ((ret = USB_Control_Msg(CONTROL_CMD, CTL_HALT_EXE_IDLE, 0, pn)) < 0) {
			LEVEL_DEFAULT("err1\n");
			break;
		}
		if ((ret = USB_Control_Msg(CONTROL_CMD, CTL_RESUME_EXE, 0, pn)) < 0) {
			LEVEL_DEFAULT("err2\n");
			break;
		}

		/* Can't wait for STATUSFLAGS_IDLE... just get first availalbe status flag */
		if ((ret = DS9490_getstatus(buffer, -1, pn)) < 0) {
			LEVEL_DEFAULT("err3 ret=%d\n", ret);
			break;
		}
		// check the SPU flag
		if (!(buffer[8] & STATUSFLAGS_SPUA)) {
			//printf("DS9490_HaltPulse: SPU not set\n");
			LEVEL_DATA("ok\n");
			return 0;
		}
		if (gettimeofday(&tv, NULL) < 0) {
			return -1;
		}
		now = (tv.tv_sec & 0xFFFF) * 1000 + tv.tv_usec / 1000;
	} while (endtime > now);

	LEVEL_DATA("timeout\n");
	return -1;
}


// Call to USB EP1 (control channel)
// Names (bRequest, wValue wIndex) are from datasheet http://datasheets.maxim-ic.com/en/ds/DS2490.pdf
static int USB_Control_Msg(BYTE bRequest, UINT wValue, UINT wIndex, const struct parsedname *pn)
{
	usb_dev_handle *usb = pn->selected_connection->connin.usb.usb;
	if (usb == NULL) {
		return -EIO;
	}
	return usb_control_msg(usb, CONTROL_REQUEST_TYPE, bRequest, wValue, wIndex, NULL, 0, pn->selected_connection->connin.usb.timeout);
}

static int DS9490_SetSpeed(const struct parsedname *pn)
{
	struct connection_in * in = pn->selected_connection ;
	int ret = 0;

	// in case timeout value changed (via settings) -- sec -> msec
	in->connin.usb.timeout = 1000 * Globals.timeout_usb;

	// Failed overdrive, switch to slow

	if (in->changed_bus_settings | CHANGED_USB_SPEED) {
		in->changed_bus_settings ^= CHANGED_USB_SPEED ;
		if (in->speed == bus_speed_overdrive) {
			if (DS9490_overdrive(pn) != 0) {
				++ ret; 
				in->speed = bus_speed_slow;
			} else {
				LEVEL_DATA("set overdrive speed\n");
			}
		} else if (in->flex==bus_yes_flex) {
			DS9490_SetFlexParameters(pn) ;
			if (USB_Control_Msg(MODE_CMD, MOD_1WIRE_SPEED, ONEWIREBUSSPEED_FLEXIBLE, pn) < 0) {
				++ ret;
			} else {
				LEVEL_DATA("set flexible speed\n");
			}
		} else {
			if (USB_Control_Msg(MODE_CMD, MOD_1WIRE_SPEED, ONEWIREBUSSPEED_REGULAR, pn) < 0) {
				++ ret;
			} else {
				LEVEL_DATA("set flexible speed\n");
			}
		}
	}
	if (in->changed_bus_settings | CHANGED_USB_SLEW) {
		in->changed_bus_settings ^= CHANGED_USB_SLEW ;
		/* Slew Rate */
		if (USB_Control_Msg(MODE_CMD, MOD_PULLDOWN_SLEWRATE, in->connin.usb.pulldownslewrate, pn) < 0) {
			LEVEL_DATA("Error MOD_PULLDOWN_SLEWRATE\n");
			++ret;
		}
	}
	if (in->changed_bus_settings | CHANGED_USB_LOW) {
		in->changed_bus_settings ^= CHANGED_USB_LOW ;
		/* Low Time */
		if (USB_Control_Msg(MODE_CMD, MOD_WRITE1_LOWTIME, in->connin.usb.writeonelowtime, pn) < 0) {
			LEVEL_DATA("Error MOD_WRITE1_LOWTIME\n");
			++ret;
		}
	}
	if (in->changed_bus_settings | CHANGED_USB_OFFSET) {
		in->changed_bus_settings ^= CHANGED_USB_OFFSET ;
		/* DS0 Low */
		if (USB_Control_Msg(MODE_CMD, MOD_DSOW0_TREC, in->connin.usb.datasampleoffset, pn) < 0) {
			LEVEL_DATA("Error MOD_DS0W0\n");
			++ret;
		}
	}
	return ret;
}

static void DS9490_SetFlexParameters(const struct parsedname *pn)
{
	struct connection_in * in = pn->selected_connection ;
	
	/* in regular and overdrive speed, slew rate is 15V/us. It's only
	* suitable for short 1-wire busses. Use flexible speed instead. */

	// default values for bus-timing when using --altUSB
	if (Globals.altUSB) {
		in->connin.usb.pulldownslewrate = PARMSET_Slew1p37Vus;
		in->connin.usb.writeonelowtime = PARMSET_W1L_10us;
		in->connin.usb.datasampleoffset = PARMSET_DS0_W0R_8us;
	} else {
		/* This seem to be the default value when reseting the ds2490 chip */
		in->connin.usb.pulldownslewrate = PARMSET_Slew0p83Vus;
		in->connin.usb.writeonelowtime = PARMSET_W1L_12us;
		in->connin.usb.datasampleoffset = PARMSET_DS0_W0R_7us;
	}
	in->changed_bus_settings |= CHANGED_USB_SLEW | CHANGED_USB_LOW | CHANGED_USB_OFFSET ;
}

#endif							/* OW_USB */
