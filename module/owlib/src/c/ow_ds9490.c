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

#if OW_USB						/* conditional inclusion of USB */

/* Extensive FreeBSD workarounds by Robert Nilsson <rnilsson@mac.com> */
#ifdef __FreeBSD__
	// Add a few definitions we need
#undef HAVE_USB_INTERRUPT_READ	// This call in libusb is unneeded for FreeBSD (and it's broken)
#include <dev/usb/usb.h>
struct usb_dev_handle {
	int fd;
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
static int DS9490_reset(const struct parsedname *pn);
static int DS9490_open(struct usb_list *ul, const struct parsedname *pn);
static int DS9490_reconnect(const struct parsedname *pn);
int DS9490_getstatus(BYTE * buffer, int readlen,
							const struct parsedname *pn);
static int DS9490_next_both(struct device_search *ds,
							const struct parsedname *pn);
static int DS9490_sendback_data(const BYTE * data, BYTE * resp,
								const size_t len,
								const struct parsedname *pn);
static int DS9490_level(int new_level, const struct parsedname *pn);
static void DS9490_setroutines(struct interface_routines *f);
static int DS9490_detect_low(const struct parsedname *pn);
static int DS9490_detect_found(struct usb_list *ul,
							   const struct parsedname *pn);
static int DS9490_PowerByte(const BYTE byte, BYTE * resp, const UINT delay,
							const struct parsedname *pn);
static int DS9490_read(BYTE * buf, const size_t size,
					   const struct parsedname *pn);
static int DS9490_write(BYTE * buf, const size_t size,
						const struct parsedname *pn);
static int DS9490_overdrive(const UINT overdrive,
							const struct parsedname *pn);
static int DS9490_testoverdrive(const struct parsedname *pn);
static int DS9490_redetect_low(const struct parsedname *pn);
static char *DS9490_device_name(const struct usb_list *ul);

/* Device-specific routines */
static void DS9490_setroutines(struct interface_routines *f)
{
	f->detect = DS9490_detect;
	f->reset = DS9490_reset;
	f->next_both = DS9490_next_both;
	f->overdrive = DS9490_overdrive;
	f->testoverdrive = DS9490_testoverdrive;
	f->PowerByte = DS9490_PowerByte;
//    f->ProgramPulse = ;
	f->sendback_data = DS9490_sendback_data;
//    f->sendback_bits = ;
	f->select = NULL;
	f->reconnect = DS9490_reconnect;
	f->close = DS9490_close;
	f->transaction = NULL;
	f->flags = 0;
}

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

	if ((ret = ioctl(dev->fd, USB_DO_REQUEST, &ctl_req)) < 0)
		LEVEL_DATA("DS9490_clear_halt:  failed for %d", ep);

	return ret;
}
#endif							/* __FreeBSD__ */

int DS9490_enumerate(void)
{
	struct usb_list ul;
	int ret = 0;
	USB_init(&ul);
	while (!USB_next(&ul))
		++ret;
	return ret;
}

int DS9490_detect(struct connection_in *in)
{
	struct parsedname pn;
	int ret;

	DS9490_setroutines(&in->iroutines);	// set up close, reconnect, reset, ...
	in->name = badUSBname;		// initialized

	FS_ParsedName(NULL, &pn);	// minimal parsename -- no destroy needed
	pn.in = in;

	// store timeout value -- sec -> msec
	in->connin.usb.timeout = 1000 * Global.timeout_usb;

	// default is to use flexible speed. This makes it possible
	// to change slew-rate etc...
	in->use_overdrive_speed = ONEWIREBUSSPEED_FLEXIBLE;

	/* in regular and overdrive speed, slew rate is 15V/us. It's only
	 * suitable for short 1-wire busses. Use flexible speed instead. */

	// default values for bus-timing when using --altUSB
	if(Global.altUSB) {
	  in->connin.usb.pulldownslewrate = PARMSET_Slew1p37Vus;
	  in->connin.usb.writeonelowtime = PARMSET_W1L_10us;
	  in->connin.usb.datasampleoffset = PARMSET_DS0_W0R_8us;
	} else {
	  /* This seem to be the default value when reseting the ds2490 chip */
	  in->connin.usb.pulldownslewrate = PARMSET_Slew0p83Vus;
	  in->connin.usb.writeonelowtime = PARMSET_W1L_12us;
	  in->connin.usb.datasampleoffset = PARMSET_DS0_W0R_7us;
	}

	ret = DS9490_detect_low(&pn);
	if (ret) {
		fprintf(stderr,
				"Could not open the USB adapter. Is there a problem with permissions?\n");
	} else {
		in->busmode = bus_usb;
	}
	return ret;
}

/* Found a DS9490 that seems good, now check list and find a device to ID for reconnects */
static int DS9490_detect_found(struct usb_list *ul,
							   const struct parsedname *pn)
{
	struct device_search ds;
	struct parsedname pncopy;
	int ret;

	if (DS9490_open(ul, pn))
		return -EIO;

	/* We are looking for devices in the root (not the branch
	 * pn eventually points to */
	memcpy(&pncopy, pn, sizeof(struct parsedname));
	pncopy.dev = NULL;
	pncopy.pathlength = 0;

	/* Do a quick directory listing and find the DS1420 id */
	if ((ret = BUS_first(&ds, &pncopy))) {
		// clear it just in case nothing is found
		memset(pn->in->connin.usb.ds1420_address, 0, 8);
		LEVEL_DATA("BUS_first failed during connect [%s]\n", pn->in->name);
	} else {
		while (ret == 0) {
			if ((ds.sn[0] & 0x7F) == 0x01) {
				LEVEL_CONNECT("Good DS1421 tag found for %s\n",
							  SAFESTRING(pn->in->name));
				break;			// good tag
			}
			ret = BUS_next(&ds, &pncopy);
		}
		memcpy(pn->in->connin.usb.ds1420_address, ds.sn, 8);
		LEVEL_DEFAULT("Set DS9490 %s unique id to " SNformat "\n",
					  SAFESTRING(pn->in->name),
					  SNvar(pn->in->connin.usb.ds1420_address));
	}
	return 0;
}

/* Open a DS9490  -- low level code (to allow for repeats)  */
static int DS9490_detect_low(const struct parsedname *pn)
{
	struct usb_list ul;
	int useusb = pn->in->connin.usb.usb_nr;	/* usb_nr holds the number of the adapter */
	int usbnum = 0;

	USB_init(&ul);
	while (!USB_next(&ul)) {
		if (++usbnum < useusb) {
			LEVEL_CONNECT("USB DS9490 %d passed over. (Looking for %d)\n",
						  usbnum, useusb);
		} else {
			return DS9490_detect_found(&ul, pn);
		}
	}

	LEVEL_CONNECT("No available USB DS9490 adapter found\n")
		return -ENODEV;
}

int DS9490_BusParm(struct connection_in *in)
{
	int ret = 0;
	usb_dev_handle *usb = in->connin.usb.usb;
	struct connin_usb *con_usb = &in->connin.usb;

	/* Willy Robison's tweaks */
	//if (Global.altUSB) {  // always set slewrate since we use flexible speed
		if(usb == NULL) return -1;

		/* Slew Rate */
		if ((ret =
			 usb_control_msg(usb, 0x40, MODE_CMD, MOD_PULLDOWN_SLEWRATE,
					 con_usb->pulldownslewrate, NULL, 0,
					 con_usb->timeout)) < 0) {
			LEVEL_DATA("DS9490_BusParm: Error MOD_PULLDOWN_SLEWRATE\n");
			return -EIO;
		}
		/* Low Time */
		if ((ret =
			 usb_control_msg(usb, 0x40, MODE_CMD, MOD_WRITE1_LOWTIME,
					 con_usb->writeonelowtime, NULL, 0,
					 con_usb->timeout)) < 0) {
			LEVEL_DATA("DS9490_BusParm: Error MOD_WRITE1_LOWTIME\n");
			return -EIO;
		}
		/* DS0 Low */
		if ((ret =
			 usb_control_msg(usb, 0x40, MODE_CMD, MOD_DSOW0_TREC,
					 con_usb->datasampleoffset, NULL, 0,
					 con_usb->timeout)) < 0) {
			LEVEL_DATA("DS9490_BusParm: Error MOD_WRITE1_LOWTIME\n");
			return -EIO;
		}
	//}
	return ret;
}

static int DS9490_setup_adapter(const struct parsedname *pn)
{
	BYTE buffer[32];
	int ret;
	usb_dev_handle *usb = pn->in->connin.usb.usb;

	// reset the device (not the 1-wire bus)
	if ((ret =
		 usb_control_msg(usb, 0x40, CONTROL_CMD, CTL_RESET_DEVICE, 0x0000,
						 NULL, 0, pn->in->connin.usb.timeout)) < 0) {
		LEVEL_DATA("DS9490_setup_adapter: error1 ret=%d\n", ret);
		return -EIO;
	}
	// set the strong pullup duration to infinite
	if ((ret =
		 usb_control_msg(usb, 0x40, COMM_CMD, COMM_SET_DURATION | COMM_IM,
						 0x0000, NULL, 0,
						 pn->in->connin.usb.timeout)) < 0) {
		LEVEL_DATA("DS9490_setup_adapter: error2 ret=%d\n", ret);
		return -EIO;
	}
	// set the 12V pullup duration to 512us
	if ((ret =
		 usb_control_msg(usb, 0x40, COMM_CMD,
						 COMM_SET_DURATION | COMM_IM | COMM_TYPE, 0x0040,
						 NULL, 0, pn->in->connin.usb.timeout)) < 0) {
		LEVEL_DATA("DS9490_setup_adapter: error3 ret=%d\n", ret);
		return -EIO;
	}
#if 1
	// disable strong pullup, but leave program pulse enabled (faster)
	if ((ret =
		 usb_control_msg(usb, 0x40, MODE_CMD, MOD_PULSE_EN,
						 ENABLEPULSE_PRGE, NULL, 0,
						 pn->in->connin.usb.timeout)) < 0) {
		LEVEL_DATA("DS9490_setup_adapter: error4 ret=%d\n", ret);
		return -EIO;
	}
#endif

	pn->in->connin.usb.ULevel = MODE_NORMAL;

	if ((ret = DS9490_getstatus(buffer, 0, pn)) < 0) {
		LEVEL_DATA("DS9490_setup_adapter: getstatus failed ret=%d\n", ret);
		return ret;
	}

	if((ret = DS9490_BusParm(pn->in)) < 0) return ret;

	LEVEL_DATA("DS9490_setup_adapter: done (ret=%d)\n", ret);
	return 0;
}


/* Open usb device,
   unload ds9490r kernel module if it is interfering
   set all the interface magic
   set callback routines and adapter entries
*/
static int DS9490_open(struct usb_list *ul, const struct parsedname *pn)
{
	int ret = ENODEV;
	usb_dev_handle *usb;

	if (pn->in->name != badUSBname)
		free(pn->in->name);
	pn->in->name = DS9490_device_name(ul);

	pn->in->connin.usb.dev = ul->dev;

	if (pn->in->name == badUSBname) {
		ret = -ENOMEM;
	} else if (pn->in->connin.usb.usb) {
		LEVEL_DEFAULT
			("DS9490_open: usb.usb was NOT closed before DS9490_open() ?\n");
	} else if (pn->in->connin.usb.dev
			   && (usb = usb_open(pn->in->connin.usb.dev))) {
		pn->in->connin.usb.usb = usb;
#ifdef LIBUSB_HAS_DETACH_KERNEL_DRIVER_NP
		usb_detach_kernel_driver_np(usb, 0);
#endif							/* LIBUSB_HAS_DETACH_KERNEL_DRIVER_NP */
		if (usb_set_configuration(usb, 1)) {
			LEVEL_CONNECT
				("Failed to set configuration on USB DS9490 adapter at %s.\n",
				 pn->in->name);
		} else if ((ret = usb_claim_interface(usb, 0))) {
			LEVEL_CONNECT
				("Failed to claim interface on USB DS9490 adapter at %s. ret=%d\n",
				 pn->in->name, ret);
		} else {
			if (usb_set_altinterface(usb, 3)) {
				LEVEL_CONNECT
					("Failed to set alt interface on USB DS9490 adapter at %s.\n",
					 pn->in->name);
			} else {
				LEVEL_DEFAULT("Opened USB DS9490 adapter at %s.\n",
							  pn->in->name);
				DS9490_setroutines(&pn->in->iroutines);
				pn->in->Adapter = adapter_DS9490;	/* OWFS assigned value */
				pn->in->adapter_name = "DS9490";

				// clear endpoints
				if ((ret =
					 USB_CLEAR_HALT(usb, DS2490_EP3) ||
					 USB_CLEAR_HALT(usb, DS2490_EP2) ||
					 USB_CLEAR_HALT(usb, DS2490_EP1))) {
					LEVEL_DEFAULT
						("DS9490_open: USB_CLEAR_HALT failed ret=%d\n",
						 ret);
				} else if (DS9490_setup_adapter(pn)
						   || DS9490_overdrive(ONEWIREBUSSPEED_FLEXIBLE, pn)
						   || DS9490_level(MODE_NORMAL, pn)) {
					LEVEL_DEFAULT
						("Error setting up USB DS9490 adapter at %s.\n",
						 pn->in->name);
				} else {		/* All GOOD */
					return 0;
				}
			}
			ret = usb_release_interface(usb, 0);
		}
		usb_close(usb);
		pn->in->connin.usb.usb = NULL;
	}

	if (pn->in->name != badUSBname)
		free(pn->in->name);
	pn->in->name = badUSBname;

	pn->in->connin.usb.dev = NULL;	// this will force a re-scan next time
	//LEVEL_CONNECT("Failed to open USB DS9490 adapter %s\n", name);
	STAT_ADD1_BUS(BUS_open_errors, pn->in);
	return ret;
}

/* When the errors stop the USB device from functioning -- close and reopen.
 * If it fails, re-scan the USB bus and search for the old adapter */
static int DS9490_reconnect(const struct parsedname *pn)
{
	int ret;

	/* Have to protect usb_find_busses() and usb_find_devices() with
	 * a lock since libusb could crash if 2 threads call it at the same time.
	 * It's not called until DS9490_redetect_low(), but I lock here just
	 * to be sure DS9490_close() and DS9490_open() get one try first. */
	LIBUSBLOCK;
	if (!DS9490_redetect_low(pn)) {
		LEVEL_DEFAULT
			("Found USB DS9490 adapter after USB rescan as [%s]!\n",
			 pn->in->name);
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
			if (ul->dev->descriptor.idVendor != 0x04FA
				|| ul->dev->descriptor.idProduct != 0x2490)
				continue;		// not DS9490
			LEVEL_CONNECT("Adapter found: %s/%s\n", ul->bus->dirname,
						  ul->dev->filename);
			return 0;
		} else {
			ul->bus = ul->bus->next;
			ul->dev = NULL;
		}
	}
	return 1;
}

static int usbdevice_in_use(char *name)
{
	struct connection_in *in;
	CONNINLOCK;
	in = indevice;
	CONNINUNLOCK;
	while (in) {
		if ((in->busmode == bus_usb) && (in->name)
			&& (!strcmp(in->name, name)))
			return 1;			// It seems to be in use already
		in = in->next;
	}
	return 0;					// not found in the current indevice-list
}

/* Construct the device name */
static char *DS9490_device_name(const struct usb_list *ul)
{
	size_t len = 2 * PATH_MAX + 1;
	char name[len + 1];
	char *ret = NULL;
	if (snprintf(name, len, "%s/%s", ul->bus->dirname, ul->dev->filename) >
		0) {
		name[len] = '\0';		// make sufre there is a trailing null
		ret = strdup(name);
	}
	if (ret == NULL)
		return badUSBname;
	return ret;
}

/* Open a DS9490  -- low level code (to allow for repeats)  */
static int DS9490_redetect_low(const struct parsedname *pn)
{
	struct usb_list ul;
	int ret;
	struct parsedname pncopy;

	//LEVEL_CONNECT("DS9490_redetect_low: name=%s\n", pn->in->name);

	/*
	 * I don't think we need to call usb_init() or usb_find_busses() here.
	 * They are already called once in DS9490_detect_low().
	 * usb_init() is more like a os_init() function and there are probably
	 * no new USB-busses added after rebooting kernel... or?
	 * It doesn't seem to leak any memory when calling them, so they are
	 * still called.
	 */
	USB_init(&ul);
	while (!USB_next(&ul)) {
		char *name = DS9490_device_name(&ul);
		struct device_search ds;
		int found1420 =
			((pn->in->connin.usb.ds1420_address[0] & 0x7F) == 0x01);

		if (name == badUSBname)
			return -ENOMEM;

		if (usbdevice_in_use(name)) {
			free(name);
			continue;
		}

		if (DS9490_open(&ul, pn)) {
			LEVEL_CONNECT("Cant open USB adapter [%s], Find next...\n",
						  name);
			free(name);
			continue;
		}

		free(name);				// use pn->in->name created in DS9490_open instead

		/* We are looking for devices in the root (not the branch
		 * pn eventually points to */
		memcpy(&pncopy, pn, sizeof(struct parsedname));
		pncopy.dev = NULL;
		pncopy.pathlength = 0;

		if ((ret = BUS_first(&ds, &pncopy))) {
			LEVEL_DATA("BUS_first failed during reconnect [%s]\n",
					   pn->in->name);
		}
		while (ret == 0) {
			if (memcmp(ds.sn, pn->in->connin.usb.ds1420_address, 8) == 0) {
				LEVEL_DATA("Found proper tag on %s\n",
						   SAFESTRING(pn->in->name));
				return 0;
			}
			found1420 = found1420 || ((ds.sn[0] & 0x7F) == 0x01);
			ret = BUS_next(&ds, &pncopy);
		}

		if (ret == -ENODEV && !found1420) {
			/* There are still no unique id, set it to the last device if the
			 * search above ended normally.  Eg. with -ENODEV */
			LEVEL_CONNECT("Still poorly tagged adapter (%s) will use "
						  SNformat "\n", SAFESTRING(pn->in->name),
						  SNvar(ds.sn));
			memcpy(pn->in->connin.usb.ds1420_address, ds.sn, 8);
			return 0;
		}
		// Couldn't find correct ds1420 chip on this adapter
		LEVEL_CONNECT
			("Couldn't find correct ds1420 chip on this adapter [%s] (want: "
			 SNformat ")\n", SAFESTRING(pn->in->name),
			 SNvar(pn->in->connin.usb.ds1420_address));
		DS9490_close(pn->in);
	}
	//LEVEL_CONNECT("No available USB DS9490 adapter found\n");
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
		LEVEL_CONNECT("Closed USB DS9490 adapter at %s. ret=%d\n",
					  in->name, ret);
	}
	in->connin.usb.usb = NULL;
	in->connin.usb.dev = NULL;
	if (in->name && in->name != badUSBname)
		free(in->name);
	in->name = badUSBname;
}

/* DS9490_getstatus()
   return -1          if Short detected on 1-wire bus
   return -ETIMEDOUT  on timeout (will require DS9490_close())
   return -EIO        on read errors (will require DS9490_close())
   otherwise return number of status bytes in buffer
*/
int DS9490_getstatus(BYTE * buffer, int readlen,
							const struct parsedname *pn)
{
	int ret, loops = 0;
	int i;
	usb_dev_handle *usb = pn->in->connin.usb.usb;

	memset(buffer, 0, 32);		// should not be needed
	//LEVEL_DETAIL("DS9490_getstatus: readlen=%d\n", readlen);

#ifdef __FreeBSD__				// Clear the Interrupt read buffer before trying to get status
	{
		char junk[1500];
		if ((ret =
			 usb_bulk_read(usb, DS2490_EP1, (ASCII *) junk, (size_t) 1500,
						   pn->in->connin.usb.timeout)) < 0) {
			STAT_ADD1_BUS(BUS_status_errors, pn->in);
			LEVEL_DATA("DS9490_getstatus: error reading ret=%d\n", ret);
			return -EIO;
		}
	}
#endif							// __FreeBSD__
	do {
#ifdef HAVE_USB_INTERRUPT_READ
		// Fix from Wim Heirman -- kernel 2.6 is fussier about endpoint type
		if ((ret =
			 usb_interrupt_read(usb, DS2490_EP1, (ASCII *) buffer,
								(size_t) 32,
								pn->in->connin.usb.timeout)) < 0) {
            LEVEL_DATA("DS9490_getstatus: (HAVE_USB_INTERRUPT_READ) error reading ret=%d\n", ret);
#else
		if ((ret =
			 usb_bulk_read(usb, DS2490_EP1, (ASCII *) buffer, (size_t) 32,
						   pn->in->connin.usb.timeout)) < 0) {
            LEVEL_DATA("DS9490_getstatus: (no HAVE_USB_INTERRUPT_READ) error reading ret=%d\n", ret);
#endif
			STAT_ADD1_BUS(BUS_status_errors, pn->in);
			return -EIO;
		}
#if 0
#ifdef OW_DEBUG
		if (Global.error_level > 4) {	// LEVEL_DETAIL
			char s[97];	// For my fancy display in the log
			char t[8];
			s[0] = '\0';
			for (i = 0; i < ret; i++) {
				sprintf(t, "-%02x", buffer[i]);
				strcat(s, t);
			}
			LEVEL_DETAIL("DS9490_getstatus: Bytes%s\n", s);
		}
#endif							/* OW_DEBIG */
#endif							/* 0 */
		if (ret > 16) {
			if (ret == 32) {	// FreeBSD buffers the input, so this could just be two readings
				if (!memcmp(buffer, &buffer[16], 6)) {
					memmove(buffer, &buffer[16], 16);
					ret = 16;
					LEVEL_DATA("DS9490_getstatus: Corrected buffer 32 byte read\n");
				}
			}
			for (i = 16; i < ret; i++) {
				BYTE val = buffer[i];
				if (val != ONEWIREDEVICEDETECT) {
					LEVEL_DATA("DS9490_getstatus: Status byte[%X]: %X\n",
							   i - 16, val);
				}
				if (val & COMMCMDERRORRESULT_SH) {	// short detected
					LEVEL_DATA("DS9490_getstatus(): short detected\n");
					return -1;
				}
			}
		}

		if (readlen < 0)
			break;				/* Don't wait for STATUSFLAGS_IDLE if length==-1 */

		if (buffer[8] & STATUSFLAGS_IDLE) {
			if (readlen > 0) {
				// we have enough bytes to read now!
				// buffer[13] == (ReadBufferStatus)
				if (buffer[13] >= readlen)
					break;
			} else
				break;
		}
		// this value might be decreased later...
		if (++loops > 100) {
			LEVEL_DATA
				("DS9490_getstatus(): never got idle  StatusFlags=%X read=%X\n",
				 buffer[8], buffer[13]);
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

static int DS9490_testoverdrive(const struct parsedname *pn)
{
	BYTE buffer[32];
	BYTE r[8];
	BYTE p = 0x69;
	int i, ret;

	// make sure normal level
	if ((ret = DS9490_level(MODE_NORMAL, pn)) < 0)
		return ret;

	// force to normal communication speed
	if ((ret = DS9490_overdrive(ONEWIREBUSSPEED_FLEXIBLE, pn)) < 0)
		return ret;

	if ((ret = BUS_reset(pn)) < 0)
		return ret;

	if (!DS9490_sendback_data(&p, buffer, 1, pn) && (p == buffer[0])) {	// match command 0x69
		if (!DS9490_overdrive(ONEWIREBUSSPEED_OVERDRIVE, pn)) {
			for (i = 0; i < 8; i++)
				buffer[i] = pn->sn[i];
			LEVEL_DEBUG("Test overdrive "SNformat"\n",SNvar(pn->sn));
			if (!DS9490_sendback_data(buffer, r, 8, pn)) {
				for (i = 0; i < 8; i++) {
					if (r[i] != pn->sn[i])
						break;
				}
				if (i == 8) {
					if ((i = DS9490_getstatus(buffer, 0, pn)) >= 0) {
						LEVEL_DEBUG("DS9490_testoverdrive: i=%d status=%X\n", i, (i>0 ? buffer[16] : 0));
						return 0;	// ok
					}
				}
			}
		}
	}
	LEVEL_DEBUG("DS9490_testoverdrive failed\n");
	DS9490_overdrive(ONEWIREBUSSPEED_FLEXIBLE, pn);
	return -EINVAL;
}

static int DS9490_overdrive(const UINT overdrive,
							const struct parsedname *pn)
{
	int ret;
	BYTE sp = 0x3C;
	BYTE resp;
	int i;
	int oldspeed;
	struct connin_usb *con_usb = &pn->in->connin.usb;

	switch (overdrive) {
	case ONEWIREBUSSPEED_OVERDRIVE:
		if (pn->in->use_overdrive_speed != ONEWIREBUSSPEED_OVERDRIVE)
			return -1;	// adapter doesn't use overdrive

		if (con_usb->USpeed == ONEWIREBUSSPEED_OVERDRIVE)
			return 0;	// already in overdrive speed

		LEVEL_DATA("DS9490_overdrive() set overdrive speed\n");

		// set USpeed here to avoid BUS_reset to call this function
		// recursively
		oldspeed = con_usb->USpeed;
		con_usb->USpeed = ONEWIREBUSSPEED_OVERDRIVE;

		// we need to change speed to overdrive
		// 0x3C is overdriveskip command
		for (i = 0; i < 3; i++) {
			if ((ret = BUS_reset(pn)) < 0)
				continue;
			if (((ret = DS9490_sendback_data(&sp, &resp, 1, pn)) < 0)
				|| (sp != resp)) {
				LEVEL_DEBUG("overdrive: error sending ret=%d 0x3C/0x%02X\n", ret, resp);
				continue;
			}
			if((con_usb->usb) &&
			   ((ret = usb_control_msg(con_usb->usb, 0x40,
						   MODE_CMD, MOD_1WIRE_SPEED,
						   ONEWIREBUSSPEED_OVERDRIVE, NULL, 0,
						   con_usb->timeout)) == 0))
				break;
		}
		if (i == 3) {
			LEVEL_DEBUG("Error setting overdrive after 3 retries\n");
			con_usb->USpeed = oldspeed;
		  	return -EINVAL;
		}
		LEVEL_DEBUG("overdrive: speed is now set to overdrive\n");
		break;
	case ONEWIREBUSSPEED_FLEXIBLE:
		/* We should perhaps test if speed already is set, but then we
		 * might have problem when we can't reach flex-speed if
		 * overdrive failed. */
		if (con_usb->USpeed == ONEWIREBUSSPEED_FLEXIBLE)
			return 0;  // already in flexible speed

		LEVEL_DATA("DS9490_overdrive() set flexible speed\n");
		if (con_usb->usb) {
			/* Have to make sure usb isn't closed after last reconnect */
			if ((ret =
				 usb_control_msg(con_usb->usb, 0x40, MODE_CMD,
						 MOD_1WIRE_SPEED, ONEWIREBUSSPEED_FLEXIBLE,
						 NULL, 0, con_usb->timeout)) < 0)
				return ret;
		}
		break;
	default:
		/* We should perhaps test if speed already is set, but then we
		 * might have problem when we can't reach regular-speed if
		 * overdrive failed. */
		if (con_usb->USpeed == ONEWIREBUSSPEED_REGULAR)
			return 0;  // already in regular speed

		LEVEL_DATA("DS9490_overdrive() set regular speed\n");
		if (con_usb->usb) {
			/* Have to make sure usb isn't closed after last reconnect */
			if ((ret =
				 usb_control_msg(con_usb->usb, 0x40, MODE_CMD,
						 MOD_1WIRE_SPEED, ONEWIREBUSSPEED_REGULAR,
						 NULL, 0, con_usb->timeout)) < 0)
				return ret;
		}
		break;
	}
	con_usb->USpeed = overdrive;
	return 0;
}

/* Reset adapter and detect devices on the bus */
/* BUS locked at high level */
/* return 1=short, 0 good <0 error */
static int DS9490_reset(const struct parsedname *pn)
{
	int i, ret;
	BYTE buffer[32];
	//printf("9490RESET\n");
	//printf("DS9490_reset() index=%d pn->in->Adapter=%d %s\n", pn->in->index, pn->in->Adapter, pn->in->adapter_name);

	LEVEL_DATA("DS9490_reset\n");

	if (!pn->in->connin.usb.usb || !pn->in->connin.usb.dev)
		return -EIO;

	// in case timeout value changed (via settings) -- sec -> msec
	pn->in->connin.usb.timeout = 1000 * Global.timeout_usb;

	memset(buffer, 0, 32);

	if ((ret = DS9490_level(MODE_NORMAL, pn)) < 0) {
		LEVEL_DATA("DS9490_reset: level failed ret=%d\n", ret);
		return ret;
	}
	
	if((pn->in->connin.usb.USpeed != pn->in->use_overdrive_speed) &&
	   ((ret = DS9490_overdrive(pn->in->use_overdrive_speed, pn)) < 0)) {
		// revert to a flexible speed
		if(pn->in->use_overdrive_speed == ONEWIREBUSSPEED_OVERDRIVE)
			pn->in->use_overdrive_speed = ONEWIREBUSSPEED_FLEXIBLE;
		return ret;
	}

	if ((ret = usb_control_msg(pn->in->connin.usb.usb, 0x40, COMM_CMD,
							   COMM_1_WIRE_RESET | COMM_F | COMM_IM |
							   COMM_SE, pn->in->connin.usb.USpeed, NULL, 0,
							   pn->in->connin.usb.timeout)) < 0) {
		LEVEL_DATA("DS9490_reset: error sending reset ret=%d\n", ret);
		return -EIO;			// fatal error... probably closed usb-handle
	}

	if (pn->in->ds2404_compliance
		&& (pn->in->connin.usb.USpeed != ONEWIREBUSSPEED_OVERDRIVE)) {
		// extra delay for alarming DS1994/DS2404 complience
		UT_delay(5);
	}

	if ((ret = DS9490_getstatus(buffer, 0, pn)) < 0) {
		if (ret == -1) {
			/* Short detected, but otherwise no bigger "problem"?
			 * Make sure 1-wires won't be scanned */
			pn->in->AnyDevices = 0;
			STAT_ADD1_BUS(BUS_short_errors, pn->in);
			LEVEL_DATA("DS9490_reset: short detected\n", ret);
			return BUS_RESET_SHORT ;
		}
		LEVEL_DATA("DS9490_reset: getstatus failed ret=%d\n", ret);
		return ret;
	}
//    USBpowered = (buffer[8]&STATUSFLAGS_PMOD) == STATUSFLAGS_PMOD ;
	pn->in->AnyDevices = 1;
	for (i = 0; i < ret; i++) {
		BYTE val = buffer[16 + i];
		//LEVEL_DATA("Status bytes[%d]: %X\n", i, val);
		if (val != ONEWIREDEVICEDETECT) {
			// check for NRS bit (0x01)
			if (val & COMMCMDERRORRESULT_NRS) {
				// empty bus detected, no presence pulse detected
				pn->in->AnyDevices = 0;
				LEVEL_DATA("DS9490_reset: no presense pulse detected\n");
			}
		}
	}

	LEVEL_DATA("DS9490_reset: ok\n");
	return BUS_RESET_OK;
}

static int DS9490_read(BYTE * buf, const size_t size,
					   const struct parsedname *pn)
{
	int ret;
	usb_dev_handle *usb = pn->in->connin.usb.usb;
	//printf("DS9490_read\n");
	if ((ret =
		 usb_bulk_read(usb, DS2490_EP3, (ASCII *) buf, (int) size,
					   pn->in->connin.usb.timeout)) > 0)
		return ret;
	LEVEL_DATA("DS9490_read: failed ret=%d\n", ret);
	USB_CLEAR_HALT(usb, DS2490_EP3);
	STAT_ADD1_BUS(BUS_read_errors, pn->in);
	return ret;
}

static int DS9490_write(BYTE * buf, const size_t size,
						const struct parsedname *pn)
{
	int ret;
	usb_dev_handle *usb = pn->in->connin.usb.usb;
	//printf("DS9490_write\n");
	if ((ret =
		 usb_bulk_write(usb, DS2490_EP2, (ASCII *) buf, (const int) size,
						pn->in->connin.usb.timeout)) > 0)
		return ret;
	LEVEL_DATA("DS9490_write: failed ret=%d\n", ret);
	USB_CLEAR_HALT(usb, DS2490_EP2);
	STAT_ADD1_BUS(BUS_write_errors, pn->in);
	return ret;
}

static int DS9490_sendback_data(const BYTE * data, BYTE * resp,
								const size_t len,
								const struct parsedname *pn)
{
	int ret = 0;
	usb_dev_handle *usb = pn->in->connin.usb.usb;
	BYTE buffer[32];

	if (len > USB_FIFO_EACH) {
//printf("USBsendback splitting\n");
		return DS9490_sendback_data(data, resp, USB_FIFO_EACH, pn)
			|| DS9490_sendback_data(&data[USB_FIFO_EACH],
									&resp[USB_FIFO_EACH],
									len - USB_FIFO_EACH, pn);
	}

	if ((ret = DS9490_write(data, (size_t) len, pn)) < (int) len) {
		LEVEL_DATA("USBsendback bulk write problem ret=%d\n", ret);
		return ret;
	}
	// COMM_BLOCK_IO | COMM_IM | COMM_F == 0x0075
	if (((ret =
		  usb_control_msg(usb, 0x40, COMM_CMD,
						  COMM_BLOCK_IO | COMM_IM | COMM_F, len, NULL, 0,
						  pn->in->connin.usb.timeout)) < 0)
		|| ((ret = DS9490_getstatus(buffer, len, pn)) < 0)	// wait for len bytes
		) {
		LEVEL_DATA("USBsendback control problem ret=%d\n", ret);
		STAT_ADD1_BUS(BUS_byte_errors, pn->in);
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

static int DS9490_next_both(struct device_search *ds,
							const struct parsedname *pn)
{
	BYTE buffer[32];
	BYTE *cb = pn->in->combuffer;
	usb_dev_handle *usb = pn->in->connin.usb.usb;
	int ret;
	int i;
	size_t buflen;

	//LEVEL_DATA("DS9490_next_both SN in: %.2X %.2X %.2X %.2X %.2X %.2X %.2X %.2X\n",serialnumber[0],serialnumber[1],serialnumber[2],serialnumber[3],serialnumber[4],serialnumber[5],serialnumber[6],serialnumber[7]) ;
	// if the last call was not the last one
	if (!pn->in->AnyDevices)
		ds->LastDevice = 1;

	/* DS1994/DS2404 might need an extra reset */
	if (pn->in->ExtraReset) {
		if (BUS_reset(pn) < 0)
			ds->LastDevice = 1;
		pn->in->ExtraReset = 0;
	}

	if (ds->LastDevice)
		return -ENODEV;

	/** Play LastDescrepancy games with bitstream */
	memcpy(cb, ds->sn, 8);		/* set bufferto zeros */
	if (ds->LastDiscrepancy > -1)
		UT_setbit(cb, ds->LastDiscrepancy, 1);
	/* This could be more efficiently done than bit-setting, but probably wouldnt make a difference */
	for (i = ds->LastDiscrepancy + 1; i < 64; i++)
		UT_setbit(cb, i, 0);
	//LEVEL_DATA("DS9490_next_both EP2: %.2X %.2X %.2X %.2X %.2X %.2X %.2X %.2X\n",cb[0],cb[1],cb[2],cb[3],cb[4],cb[5],cb[6],cb[7]) ;

	buflen = 8;
	if ((ret = DS9490_write(cb, buflen, pn)) < (int) buflen) {
		LEVEL_DATA("USBnextboth bulk write problem = %d\n", ret);
		return -EIO;
	}
	// COMM_SEARCH_ACCESS | COMM_IM | COMM_SM | COMM_F | COMM_RTS
	// 0xF4 + +0x1 + 0x8 + 0x800 + 0x4000 = 0x48FD
	if ((ret =
		 usb_control_msg(usb, 0x40, COMM_CMD, 0x48FD,
						 0x0100 | (ds->search), NULL, 0,
						 pn->in->connin.usb.timeout)) < 0) {
		LEVEL_DATA("USBnextboth control problem ret=%d\n", ret);
		return -EIO;
	}

	if ((ret = DS9490_getstatus(buffer, 0, pn)) < 0) {
		LEVEL_DATA("USBnextboth: getstatus error\n");
		return -EIO;
	}

	if (buffer[13] == 0) {		// (ReadBufferStatus)
		/* Nothing found on the bus. Have to return something != 0 to avoid
		 * getting stuck in loop in FS_realdir() and FS_alarmdir()
		 * which ends when ret!=0 */
		LEVEL_DATA("USBnextboth: ReadBufferstatus == 0\n");
		return -ENOENT;
	}
	//buflen = 16 ;  // try read 16 bytes
	buflen = buffer[13];
	//LEVEL_DATA("USBnextboth len=%d (available=%d)\n", buflen, buffer[13]);
	if ((ret = DS9490_read(cb, buflen, pn)) <= 0) {
		LEVEL_DATA("USBnextboth: bulk read problem ret=%d\n", ret);
		return -EIO;
	}

	memcpy(ds->sn, cb, 8);
	//LEVEL_DATA("DS9490_next_both SN out: %.2X %.2X %.2X %.2X %.2X %.2X %.2X %.2X\n",serialnumber[0],serialnumber[1],serialnumber[2],serialnumber[3],serialnumber[4],serialnumber[5],serialnumber[6],serialnumber[7]) ;
	ds->LastDevice = (ret == 8);

	for (i = 63; i >= 0; i--) {
		if (UT_getbit(cb, i + 64) && (UT_getbit(cb, i) == 0)) {
			ds->LastDiscrepancy = i;
			//printf("DS9490_next_both lastdiscrepancy=%d\n",si->LastDiscrepancy) ;
			break;
		}
	}

	/* test for CRC error */
	if (CRC8(ds->sn, 8)) {
		LEVEL_DATA("USBnextboth: CRC error\n");
		return -EIO;
	}

	/* test for special device families */
	switch (ds->sn[0]) {
	case 0x00:
		LEVEL_DATA("USBnextboth: NULL family found\n");
		return -EIO;
	case 0x04:
	case 0x84:
		/* We found a DS1994/DS2404 which require longer delays */
		pn->in->ds2404_compliance = 1;
		break;
	default:
		break;
	}
	LEVEL_DEBUG("DS9490_next_both SN found: " SNformat "\n",
				SNvar(ds->sn));
	return 0;
}

//--------------------------------------------------------------------------
// Send 8 bits of communication to the 1-Wire Net and verify that the
// 8 bits read from the 1-Wire Net is the same (write operation).
// The parameter 'byte' least significant 8 bits are used.  After the
// 8 bits are sent change the level of the 1-Wire net.
// Delay delay msec and return to normal
//
/* Returns 0=good
   bad = -EIO
 */
static int DS9490_PowerByte(const BYTE byte, BYTE * resp, const UINT delay,
							const struct parsedname *pn)
{
	usb_dev_handle *usb = pn->in->connin.usb.usb;
	int ret;

	LEVEL_DATA("DS9490_Powerbyte\n");

	/* This is more likely to be the correct way to handle powerbytes */
	if (pn->in->connin.usb.ULevel == MODE_STRONG5) {
		DS9490_level(MODE_NORMAL, pn);
	}
	// set the strong pullup
	if ((ret =
		 usb_control_msg(usb, 0x40, MODE_CMD, MOD_PULSE_EN,
						 ENABLEPULSE_SPUE, NULL, 0,
						 pn->in->connin.usb.timeout)) < 0) {
		LEVEL_DATA("DS9490_Powerbyte: Error usb_control_msg 3\n");
	} else
		if ((ret =
			 usb_control_msg(usb, 0x40, COMM_CMD,
							 COMM_BYTE_IO | COMM_IM | COMM_SPU,
							 byte & 0xFF, NULL, 0,
							 pn->in->connin.usb.timeout)) < 0) {
		LEVEL_DATA("DS9490_Powerbyte: Error usb_control_msg 4\n");
	} else {
		/* strong pullup is now enabled */
		pn->in->connin.usb.ULevel = MODE_STRONG5;

		/* Read back the result (should be the same as "byte") */
		if ((ret = DS9490_read(resp, 1, pn)) < 0) {
			LEVEL_DATA("DS9490_Powerbyte: Error DS9490_read ret=%d\n",
					   ret);
		} else {
			/* Delay with strong pullup */
			UT_delay(delay);

			/* Turn off strong pullup */
			if ((ret = DS9490_level(MODE_NORMAL, pn)) < 0) {
				LEVEL_DATA("DS9490_Powerbyte: DS9490_level, ret=%d\n",
						   ret);
				return ret;
			} else {
				return 0;
			}
		}
	}
	STAT_ADD1_BUS(BUS_PowerByte_errors, pn->in);
	return ret;
}

static int DS9490_HaltPulse(const struct parsedname *pn)
{
	BYTE buffer[32];
	struct timeval tv;
	time_t endtime, now;
	int ret;

	LEVEL_DATA("DS9490_HaltPulse\n");

	if (gettimeofday(&tv, NULL) < 0)
		return -1;
	endtime = (tv.tv_sec & 0xFFFF) * 1000 + tv.tv_usec / 1000 + 300;

	do {
		LEVEL_DATA("DS9490_HaltPulse: loop\n");

		if ((ret =
			 usb_control_msg(pn->in->connin.usb.usb, 0x40, CONTROL_CMD,
							 CTL_HALT_EXE_IDLE, 0, NULL, 0,
							 pn->in->connin.usb.timeout)) < 0) {
			LEVEL_DEFAULT("DS9490_HaltPulse: err1\n");
			break;
		}
		if ((ret =
			 usb_control_msg(pn->in->connin.usb.usb, 0x40, CONTROL_CMD,
							 CTL_RESUME_EXE, 0, NULL, 0,
							 pn->in->connin.usb.timeout)) < 0) {
			LEVEL_DEFAULT("DS9490_HaltPulse: err2\n");
			break;
		}

		/* Can't wait for STATUSFLAGS_IDLE... just get first availalbe status flag */
		if ((ret = DS9490_getstatus(buffer, -1, pn)) < 0) {
			LEVEL_DEFAULT("DS9490_HaltPulse: err3 ret=%d\n", ret);
			break;
		}
		// check the SPU flag
		if (!(buffer[8] & STATUSFLAGS_SPUA)) {
			//printf("DS9490_HaltPulse: SPU not set\n");
			if ((ret =
				 usb_control_msg(pn->in->connin.usb.usb, 0x40, MODE_CMD,
								 MOD_PULSE_EN, 0, NULL, 0,
								 pn->in->connin.usb.timeout)) < 0) {
				LEVEL_DEFAULT("DS9490_HaltPulse: err4\n");
				break;
			}
			LEVEL_DATA("DS9490_HaltPulse: ok\n");
			return 0;
		}
		if (gettimeofday(&tv, NULL) < 0)
			return -1;
		now = (tv.tv_sec & 0xFFFF) * 1000 + tv.tv_usec / 1000;
	} while (endtime > now);

	LEVEL_DATA("DS9490_HaltPulse: timeout\n");
	return -1;
}


/* Set the 1-Wire Net line level.  The values for new_level are
// 'new_level' - new level defined as
//                MODE_NORMAL     0x00
//                MODE_STRONG5    0x02
//                MODE_PROGRAM    0x04
//                MODE_BREAK      0x08 (not supported)
//
// Returns:    0 GOOD, !0 Error
   Actually very simple for passive adapter
*/
/* return 0=good
  -EIO not supported
 */
static int DS9490_level(int new_level, const struct parsedname *pn)
{
	int ret;
	int lev;
	usb_dev_handle *usb = pn->in->connin.usb.usb;

	if (new_level == pn->in->connin.usb.ULevel) {	// check if need to change level
		return 0;
	}

	if (!usb)
		return -EIO;

	LEVEL_DATA("DS9490_level %d (old = %d)\n", new_level,
			   pn->in->connin.usb.ULevel);

	switch (new_level) {
	case MODE_NORMAL:
		if (pn->in->connin.usb.ULevel == MODE_STRONG5) {
			if (DS9490_HaltPulse(pn) == 0) {
				pn->in->connin.usb.ULevel = MODE_NORMAL;
				return 0;
			}
		}
		return 0;
	case MODE_STRONG5:
		lev = ENABLEPULSE_SPUE;
		break;
	case MODE_PROGRAM:
		//lev = ENABLEPULSE_PRGE ;
		// Don't support Program pulse right now
		return -EIO;
	case MODE_BREAK:
	default:
		return 1;
	}

	// set pullup to strong5 or program
	// set the strong pullup duration to infinite
	if (((ret =
		  usb_control_msg(usb, 0x40, MODE_CMD, MOD_PULSE_EN, lev, NULL, 0,
						  pn->in->connin.usb.timeout)) < 0)
		||
		((ret =
		  usb_control_msg(usb, 0x40, COMM_CMD, COMM_PULSE | COMM_IM, 0,
						  NULL, 0, pn->in->connin.usb.timeout)) < 0)) {
		STAT_ADD1_BUS(BUS_level_errors, pn->in);
		return ret;
	}
	pn->in->connin.usb.ULevel = new_level;
	return 0;
}

#endif							/* OW_USB */
