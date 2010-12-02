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
#include "ow_connection.h"
#include "ow_usb_cycle.h"
#include "ow_usb_msg.h"

#if OW_USB						/* conditional inclusion of USB */

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

#define CONTROL_REQUEST_TYPE  0x40

/** EP1 -- control read */
#define DS2490_EP1              0x81
/** EP2 -- bulk write */
#define DS2490_EP2              0x02
/** EP3 -- bulk read */
#define DS2490_EP3              0x83

char badUSBname[] = "-1:-1";

/* ------------------------------------------------------------ */
/* --- USB low-level communication -----------------------------*/

// Call to USB EP1 (control channel)
// Names (bRequest, wValue wIndex) are from datasheet http://datasheets.maxim-ic.com/en/ds/DS2490.pdf
GOOD_OR_BAD USB_Control_Msg(BYTE bRequest, UINT wValue, UINT wIndex, const struct parsedname *pn)
{
	struct connection_in * in = pn->selected_connection ;
	usb_dev_handle *usb = in->master.usb.usb;
	int ret ;
	if (usb == NULL) {
		return gbBAD;
	}
	ret = usb_control_msg(usb, CONTROL_REQUEST_TYPE, bRequest, wValue, wIndex, NULL, 0, in->master.usb.timeout);
#if OW_SHOW_TRAFFIC
	fprintf(stderr, "TRAFFIC OUT <control> bus=%d (%s)\n", in->index, in->name ) ;
	fprintf(stderr, "\tbus name=%s request type=0x%.2X, wValue=0x%X, wIndex=0x%X, return code=%d\n",in->adapter_name, bRequest, wValue, wIndex, ret) ;
#endif /* OW_SHOW_TRAFFIC */
	if( ret < 0 ) {
		LEVEL_DEBUG("USB control problem = %d", ret) ;
		return gbBAD ;
	}
	return gbGOOD ;
}

#ifdef __FreeBSD__
// This is in here for versions of libusb on FreeBSD that do not support the usb_clear_halt function
#if __FreeBSD__ < 8
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
#endif /* __FreeBSD__ < 8 */
#endif							/* __FreeBSD__ */

RESET_TYPE DS9490_getstatus(BYTE * buffer, int * readlen, const struct parsedname *pn)
{
	int ret, loops = 0;
	int i;
	struct connection_in * in = pn->selected_connection ;
	usb_dev_handle *usb = in->master.usb.usb;
	
	// Pretty ugly, but needed for SUSE at least
	// Try both HAVE_USB_INTERRUPT_READ
#ifdef HAVE_USB_INTERRUPT_READ
	static int have_usb_interrupt_read = (1 == 1);
#else							/* HAVE_USB_INTERRUPT_READ */
	static int have_usb_interrupt_read = (1 == 0);
#endif							/* HAVE_USB_INTERRUPT_READ */
	static int count_have_usb_interrupt_read = 0;

	memset(buffer, 0, DS9490_getstatus_BUFFER_LENGTH );		// should not be needed

#ifdef __FreeBSD__				// Clear the Interrupt read buffer before trying to get status
	{
		char junk[1500];
		ret = usb_bulk_read(usb, DS2490_EP1, (ASCII *) junk, (size_t) 1500, in->master.usb.timeout) ;
		if ( ret < 0) {
			STAT_ADD1_BUS(e_bus_status_errors, in);
			LEVEL_DATA("error reading ret=%d", ret);
			return BUS_RESET_ERROR;
		}
	}
#endif							// __FreeBSD__
	do {
		// Fix from Wim Heirman -- kernel 2.6 is fussier about endpoint type
#ifdef HAVE_USB_INTERRUPT_READ
		if (have_usb_interrupt_read) {
			ret = usb_interrupt_read(usb, DS2490_EP1, (ASCII *) buffer, (size_t) 32, in->master.usb.timeout) ;
			if ( ret < 0 ) {
				LEVEL_DATA("(HAVE_USB_INTERRUPT_READ) error reading ret=%d", ret);
			}
		} else
#endif
		{
			ret = usb_bulk_read(usb, DS2490_EP1, (ASCII *) buffer, (size_t) 32, in->master.usb.timeout) ;
		}

		if ( ret < 0 ) {
			LEVEL_DATA("(no HAVE_USB_INTERRUPT_READ) error reading ret=%d", ret);
			if (count_have_usb_interrupt_read != 0) {
				STAT_ADD1_BUS(e_bus_status_errors, in);
				return BUS_RESET_ERROR;
			}
			have_usb_interrupt_read = !have_usb_interrupt_read;
			count_have_usb_interrupt_read = 1;
			continue;
		}
		if (ret > 32 ) {
			LEVEL_DATA("Bad DS2490 status %d > 32",ret) ;
			return BUS_RESET_ERROR;
		}
		if (ret > 16) {
			if (ret == 32) {	// FreeBSD buffers the input, so this could just be two readings
				if (!memcmp(buffer, &buffer[16], 6)) {
					memmove(buffer, &buffer[16], 16);
					ret = 16;
					LEVEL_DATA("Corrected buffer 32 byte read");
				}
			}
			for (i = 16; i < ret; i++) {
				BYTE val = buffer[i];
				if (val != ONEWIREDEVICEDETECT) {
					LEVEL_DATA("Status byte[%X]: %X", i - 16, val);
				}
				if (val & COMMCMDERRORRESULT_SH) {	// short detected
					LEVEL_DATA("short detected");
					return BUS_RESET_SHORT;
				}
			}
		}

		if (*readlen < 0) {
			break;				/* Don't wait for STATUSFLAGS_IDLE if length==-1 */
		}

		if (buffer[8] & STATUSFLAGS_IDLE) {
#if OW_SHOW_TRAFFIC
			_Debug_Bytes("USB status registers ---- idle ----", buffer, ret) ; // debugging
#endif /* OW_SHOW_TRAFFIC */
			if (*readlen > 0) {
				// we have enough bytes to read now!
				// buffer[13] == (ReadBufferStatus)
				if (buffer[13] >= *readlen) {
					break;
				}
				LEVEL_DEBUG("Problem with buffer[13]=%d and readlen[0]=%d",(int) buffer[13], (int) readlen[0] ) ;
			} else {
				break;
			}
		} else {
#if OW_SHOW_TRAFFIC
			_Debug_Bytes("USB status registers ---- Not idle -----", buffer, ret) ; // debugging
#endif /* OW_SHOW_TRAFFIC */
		}
		// this value might be decreased later...
		if (++loops > 100) {
			LEVEL_DATA("never got idle  StatusFlags=%X read=%X", buffer[8], buffer[13]);
			//reset USB device
			// probably should reset speed and any other parameters.
			USB_Control_Msg(CONTROL_CMD, CTL_RESET_DEVICE, 0x0000, pn) ;
			return BUS_RESET_ERROR;	// adapter never got idle
		}
		/* Since result seem to be on the usb bus very quick, I sleep
		 * sleep 0.1ms or something like that instead... It seems like
		 * result is there after 0.2-0.3ms
		 */
		UT_delay_us(100);
	} while (1);

	if (ret < 16) {
		LEVEL_DATA("incomplete packet ret=%d", ret);
		return BUS_RESET_ERROR;			// incomplete packet??
	}
	*readlen = ret ; // pass this data back (used by reset)
	return BUS_RESET_OK ;
}

GOOD_OR_BAD DS9490_open(struct usb_list *ul, struct connection_in *in)
{
	usb_dev_handle *usb;

	in->master.usb.dev = ul->dev;

	if (in->master.usb.dev ==NULL ) {
		return gbBAD ;
	}

	usb = usb_open(in->master.usb.dev) ;
	if (usb == NULL) {
		// intentional print to console
		fprintf(stderr, "Could not open the USB bus master. Is there a problem with permissions?\n");
		// And log
		LEVEL_DEFAULT("Could not open the USB bus master. Is there a problem with permissions?");
		STAT_ADD1_BUS(e_bus_open_errors, in);
		return gbBAD ;
	}

	in->master.usb.usb = usb;
	in->master.usb.usb_bus_number = ul->usb_bus_number ;
	in->master.usb.usb_dev_number = ul->usb_dev_number ;
#ifdef LIBUSB_HAS_DETACH_KERNEL_DRIVER_NP
	usb_detach_kernel_driver_np(usb, 0);
#endif							/* LIBUSB_HAS_DETACH_KERNEL_DRIVER_NP */

	// store timeout value -- sec -> msec
	in->master.usb.timeout = 1000 * Globals.timeout_usb;

	if ( usb_set_configuration(usb, 1) != 0 ) {
		LEVEL_CONNECT("Failed to set configuration on USB DS9490 bus master at %s.", in->name);
	} else if ( usb_claim_interface(usb, 0) != 0 ) {
		LEVEL_CONNECT("Failed to claim interface on USB DS9490 bus master at %s.", in->name);
	} else {
		if ( usb_set_altinterface(usb, 3) != 0 ) {
			LEVEL_CONNECT("Failed to set alt interface on USB DS9490 bus master at %s.", in->name);
		} else {
			LEVEL_DEFAULT("Opened USB DS9490 bus master at %s.", in->name);

			// clear endpoints
			if ( USB_CLEAR_HALT(usb, DS2490_EP3) || USB_CLEAR_HALT(usb, DS2490_EP2) || USB_CLEAR_HALT(usb, DS2490_EP1) ) {
				LEVEL_DEFAULT("USB_CLEAR_HALT failed");
			} else {		/* All GOOD */
				return gbGOOD;
			}
		}
		usb_release_interface(usb, 0);
	}
	usb_close(usb);
	in->master.usb.usb = NULL;

	LEVEL_DEBUG("Did not successfully open DS9490 %s",in->name) ;
	STAT_ADD1_BUS(e_bus_open_errors, in);
	return gbBAD;
}

void DS9490_close(struct connection_in *in)
{
	usb_dev_handle *usb = in->master.usb.usb;

	if (usb != NULL) {
		int ret = usb_release_interface(usb, 0);
		if (ret) {
			in->master.usb.dev = NULL;	// force a re-scan
			LEVEL_CONNECT("Release interface (USB) failed ret=%d", ret);
		}

		/* It might already be closed? (returning -ENODEV)
		 * I have seen problem with calling usb_close() twice, so we
		 * might perhaps skip it if usb_release_interface() fails */
		ret = usb_close(usb);
		if (ret) {
			in->master.usb.dev = NULL;	// force a re-scan
			LEVEL_CONNECT("usb_close() failed ret=%d", ret);
		}
		LEVEL_CONNECT("Closed USB DS9490 bus master at %s. ret=%d", in->name, ret);
	}
	in->master.usb.usb = NULL;
	in->master.usb.dev = NULL;
	SAFEFREE(in->name) ;
	in->name = owstrdup(badUSBname);
}

/* ------------------------------------------------------------ */
/* --- USB read and write --------------------------------------*/

SIZE_OR_ERROR DS9490_read(BYTE * buf, size_t size, const struct parsedname *pn)
{
	SIZE_OR_ERROR ret;
	struct connection_in * in = pn->selected_connection ;
	usb_dev_handle *usb = in->master.usb.usb;
	if ((ret = usb_bulk_read(usb, DS2490_EP3, (ASCII *) buf, (int) size, in->master.usb.timeout)) > 0) {
		TrafficIn("read",buf,size,pn->selected_connection) ;
		return ret;
	}
	LEVEL_DATA("failed ret=%d", ret);
	USB_CLEAR_HALT(usb, DS2490_EP3);
	STAT_ADD1_BUS(e_bus_read_errors, in);
	return ret;
}

/* Fills the EP2 buffer in the USB adapter
   returns number of bytes (size)
   or <0 for an error */
SIZE_OR_ERROR DS9490_write(const BYTE * buf, size_t size, const struct parsedname *pn)
{
	SIZE_OR_ERROR ret;
	struct connection_in * in = pn->selected_connection ;
	usb_dev_handle *usb = in->master.usb.usb;

	if (size == 0) {
		return 0;
	}

	ret = usb_bulk_write(usb, DS2490_EP2, (ASCII *) buf, (int) size, in->master.usb.timeout) ;
	if ( ret < 0 ) {
		LEVEL_DATA("failed ret=%d", ret);
		USB_CLEAR_HALT(usb, DS2490_EP2);
		STAT_ADD1_BUS(e_bus_write_errors, in);
	}
	TrafficOut("write",buf,size,pn->selected_connection);
	return ret;
}

#endif							/* OW_USB */
