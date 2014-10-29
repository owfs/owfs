/*
    OWFS -- One-Wire filesystem
    OWHTTPD -- One-Wire Web Server
    Written 2003 Paul H Alfille
    email: paul.alfille@gmail.com
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

#define CONTROL_REQUEST_TYPE  0x40

/** EP1 -- control read */
#define DS2490_EP1              0x81
/** EP2 -- bulk write */
#define DS2490_EP2              0x02
/** EP3 -- bulk read */
#define DS2490_EP3              0x83

char badUSBname[] = "-1:-1";

static int usb_transfer( int (*transfer_function) (struct libusb_device_handle *dev_handle, unsigned char endpoint, BYTE *data, int length, int *transferred, unsigned int timeout),  unsigned char endpoint, BYTE * data, int length, struct connection_in * in ) ;
static void usb_buffer_traffic( BYTE * buffer ) ;

/* ------------------------------------------------------------ */
/* --- USB low-level communication -----------------------------*/

// Call to USB EP1 (control channel)
// Names (bRequest, wValue wIndex) are from datasheet http://datasheets.maxim-ic.com/en/ds/DS2490.pdf
// libusb version.
GOOD_OR_BAD USB_Control_Msg(BYTE bRequest, UINT wValue, UINT wIndex, const struct parsedname *pn)
{
	struct connection_in * in = pn->selected_connection ;
	libusb_device_handle *usb = in->master.usb.lusb_handle;
	int ret ;
	if (usb == NULL) {
		return gbBAD;
	}
	ret = libusb_control_transfer(usb, CONTROL_REQUEST_TYPE, bRequest, wValue, wIndex, NULL, 0, in->master.usb.timeout);
	if (Globals.traffic) {
		fprintf(stderr, "TRAFFIC OUT <control> bus=%d (%s)\n", in->index, DEVICENAME(in) ) ;
		fprintf(stderr, "\tbus name=%s request type=0x%.2X, wValue=0x%X, wIndex=0x%X, return code=%d\n",in->adapter_name, bRequest, wValue, wIndex, ret) ;
	}
	if( ret < 0 ) {
		LEVEL_DEBUG("USB control problem = %d", ret) ;
		return gbBAD ;
	}
	return gbGOOD ;
}

RESET_TYPE DS9490_getstatus(BYTE * buffer, int * readlen, const struct parsedname *pn)
{
	int ret ;
	int loops = 0;
	struct connection_in * in = pn->selected_connection ;
	
	memset(buffer, 0, DS9490_getstatus_BUFFER_LENGTH );		// should not be needed

	do {
		ret = usb_transfer( libusb_interrupt_transfer, DS2490_EP1, buffer, readlen[0], in ) ;

		if ( ret < 0 ) {
			LEVEL_DATA("USB_INTERRUPT_READ error reading ret=%d <%s>", ret, libusb_error_name(ret));
			STAT_ADD1_BUS(e_bus_status_errors, in);
			return BUS_RESET_ERROR;
		} else if (ret > 32 ) {
			LEVEL_DATA("Bad DS2490 status %d > 32",ret) ;
			return BUS_RESET_ERROR;
		} else if (ret > 16) {
			int i ;
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

		if (readlen[0] < 0) {
			break;				/* Don't wait for STATUSFLAGS_IDLE if length==-1 */
		}

		usb_buffer_traffic( buffer ) ;
		if (buffer[8] & STATUSFLAGS_IDLE) {
			if (readlen[0] > 0) {
				// we have enough bytes to read now!
				// buffer[13] == (ReadBufferStatus)
				if (buffer[13] >= readlen[0]) {
					break;
				}
				LEVEL_DEBUG("Problem with buffer[13]=%d and readlen[0]=%d",(int) buffer[13], (int) readlen[0] ) ;
			} else {
				break;
			}
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
	readlen[0] = ret ; // pass this data back (used by reset)
	return BUS_RESET_OK ;
}

static void usb_buffer_traffic( BYTE * buffer )
{
	if (Globals.traffic) {
		// See note below for register info
		LEVEL_DEBUG("USB status registers (Idle) EFlags:%u->SPU:%u Dspeed:%u,Speed:%u,SPUdur:%u, PDslew:%u, W1lowtime:%u, W0rectime:%u, DevState:%u, CC1:%u, CC2:%u, CCState:%u, DataOutState:%u, DataInState:%u", 
			buffer[0], (buffer[0]&0x01), (buffer[0]&0x04 ? 1 : 0), 
			buffer[1],
			buffer[2], 
			buffer[4],
			buffer[5],
			buffer[6],
			buffer[8],
			buffer[9],
			buffer[10],
			buffer[11],
			buffer[12],
			buffer[13]
			);
	}
}


// libusb version
// dev already set
GOOD_OR_BAD DS9490_open( struct connection_in *in )
{
	libusb_device_handle * usb ;
	libusb_device * dev = in->master.usb.lusb_dev ;
	int usb_err ;

	if ( dev == NULL ) {
		return gbBAD ;
	}

	if ( (usb_err=libusb_open( dev, &usb )) != 0 ) {
		// intentional print to console
		fprintf(stderr, "Could not open the USB bus master. Is there a problem with permissions? <%s>\n",libusb_error_name(usb_err));
		// And log
		LEVEL_DEFAULT("Could not open the USB bus master. Is there a problem with permissions? <%s>",libusb_error_name(usb_err));
		STAT_ADD1_BUS(e_bus_open_errors, in);
		return gbBAD ;
	}

	in->master.usb.lusb_handle = usb;
	in->master.usb.bus_number = libusb_get_bus_number( dev ) ;
	in->master.usb.address = libusb_get_device_address( dev ) ;
//	if ( libusb_set_auto_detach_kernel_driver( usb, 1) != 0 ) {
//		LEVEL_CONNECT( "Could not set automatic USB driver management option" ) ;
//	}
	if ( (usb_err=libusb_detach_kernel_driver( usb, 0))!= 0 ) {
		LEVEL_CONNECT( "Could not release kernal module <%s>",libusb_error_name(usb_err) ) ;
	}

	// store timeout value -- sec -> msec
	in->master.usb.timeout = 1000 * Globals.timeout_usb;

	if ( (usb_err=libusb_set_configuration(usb, 1)) != 0 ) {
		LEVEL_CONNECT("Failed to set configuration on USB DS9490 bus master at %s. <%s>", DEVICENAME(in),libusb_error_name(usb_err));
	} else if ( (usb_err=libusb_claim_interface(usb, 0)) != 0 ) {
		LEVEL_CONNECT("Failed to claim interface on USB DS9490 bus master at %s. <%s>", DEVICENAME(in),libusb_error_name(usb_err));
	} else {
		if ( (usb_err=libusb_set_interface_alt_setting(usb, 0, 3)) != 0 ) {
			LEVEL_CONNECT("Failed to set alt interface on USB DS9490 bus master at %s. <%s>", DEVICENAME(in),libusb_error_name(usb_err));
		} else {
			LEVEL_DEFAULT("Opened USB DS9490 bus master at %s.", DEVICENAME(in));

			// clear endpoints
			if ( (usb_err=libusb_clear_halt(usb, DS2490_EP3)) || (usb_err=libusb_clear_halt(usb, DS2490_EP2)) || (usb_err=libusb_clear_halt(usb, DS2490_EP1)) ) {
				LEVEL_DEFAULT("USB_CLEAR_HALT failed <%s>",libusb_error_name(usb_err));
			} else {		/* All GOOD */
				return gbGOOD;
			}
		}
		if ((usb_err=libusb_release_interface(usb, 0)) != 0 ) {
			LEVEL_DEBUG("%s",libusb_error_name(usb_err)) ;
		}
		if ((usb_err=libusb_attach_kernel_driver( usb,0 )) != 0 ) {
			LEVEL_DEBUG("%s",libusb_error_name(usb_err));
		}
	}
	libusb_close(usb);
	in->master.usb.usb = NULL;

	LEVEL_DEBUG("Did not successfully open DS9490 %s -- permission problem?",DEVICENAME(in)) ;
	STAT_ADD1_BUS(e_bus_open_errors, in);
	return gbBAD;
}

void DS9490_close(struct connection_in *in)
{
	libusb_device_handle *usb = in->master.usb.lusb_handle;

	if (usb != NULL) {
		int ret = libusb_release_interface(usb, 0);
		libusb_attach_kernel_driver( usb,0 ) ;
		if (ret) {
			in->master.usb.dev = NULL;	// force a re-scan
			LEVEL_CONNECT("Release interface (USB) failed ret=%d", ret);
		}

		/* It might already be closed? (returning -ENODEV)
		 * I have seen problem with calling usb_close() twice, so we
		 * might perhaps skip it if usb_release_interface() fails */
		libusb_close(usb);
		in->master.usb.lusb_handle = NULL ;
		LEVEL_CONNECT("Closed USB DS9490 bus master at %s. ret=%d", DEVICENAME(in), ret);
	}
	in->master.usb.dev = NULL;
	SAFEFREE(DEVICENAME(in)) ;
	DEVICENAME(in) = owstrdup(badUSBname);
}

/* ------------------------------------------------------------ */
/* --- USB read and write --------------------------------------*/

// libusb version
SIZE_OR_ERROR DS9490_read(BYTE * buf, size_t size, const struct parsedname *pn)
{
	int ret;
	struct connection_in * in = pn->selected_connection ;
	
	ret = usb_transfer( libusb_bulk_transfer, DS2490_EP3, buf, size, in ) ;
	if ( ret == 0 ) {
		TrafficIn("read",buf,size,pn->selected_connection) ;
		return size;
	}
	
	LEVEL_DATA("failed DS9490 read: ret=%d", ret);
	STAT_ADD1_BUS(e_bus_read_errors, in);
	return ret;
}

/* Fills the EP2 buffer in the USB adapter
   returns number of bytes (size)
   or <0 for an error */
// libusb version
SIZE_OR_ERROR DS9490_write(BYTE * buf, size_t size, const struct parsedname *pn)
{
	int ret;
	struct connection_in * in = pn->selected_connection ;

	if (size == 0) {
		return 0;
	}

	// usb library doesn't require a const data type for writing
	ret = usb_transfer( libusb_bulk_transfer, DS2490_EP2, buf, size, in ) ;
	TrafficOut("write",buf,size,pn->selected_connection);
	if ( ret != 0 ) {
		LEVEL_DATA("failed DS9490 write: ret=%d", ret);
		STAT_ADD1_BUS(e_bus_write_errors, in);
		return ret ;
	}
	return size ;
}

// Notes from Michael Markstaller:
/*
        Datasheet DS2490 page 29 table 16
        0: Enable Flags: SPUE=1(bit0) If set to 1, the strong pullup to 5V is enabled, if set to 0, it is disabled. 
        bit1 should be 0 but is 1 ?! SPCE = 4(bit2) If set to 1, a dynamic 1-Wire bus speed change through a Communication command is enabled, if set to 0, it is disabled.
        1: 1-Wire Speed
        2: Strong Pullup Duration
        3: (Reserved)
        4: Pulldown Slew Rate
        5: Write-1 Low Time
        6: Data Sample Offset / Write-0 Recovery Time
        7: reserved
        8: Device Status Flags: bit0: SPUA if set to 1, the strong pullup to 5V is currently active, if set to 0, it is inactive.
            bit3(8): PMOD if set to 1, the DS2490 is powered from USB and external sources, if set to 0, all DS2490 power is provided from USB. FIXME: expose this to clients to check!
            bit4(16): HALT if set to 1, the DS2490 is currently halted, if set to 0, the device is not halted.
            bit5(32): IDLE if set to 1, the DS2490 is currently idle, if set to 0, the device is not idle.
            bit5(64): EPOF: Endpoint 0 FIFO status, see: If EP0F is set to 1, the Endpoint 0 FIFO was full when a new control transfer setup packet was
                received. This is an error condition in that the setup packet received is discarded due to the full
                condition. To recover from this state the USB host must send a CTL_RESET_DEVICE command; the
                device will also recover with a power on reset cycle. Note that the DS2490 will accept and process a
                CTL_RESET_DEVICE command if the EP0F = 1 state occurs. If EP0F = 0, no FIFO error condition exists.
        9: Communication Command, Byte 1
        10: Communication Command, Byte 2
        11: Communication Command Buffer Status
        12: 1-Wire Data Out Buffer Status
        13: 1-Wire Data In Buffer Status
*/

static int usb_transfer( int (*transfer_function) (struct libusb_device_handle *dev_handle, unsigned char endpoint, BYTE *data, int length, int *transferred, unsigned int timeout),  unsigned char endpoint, BYTE * data, int length, struct connection_in * in )
{
	int transferred_so_far = 0 ;
	libusb_device_handle *usb = in->master.usb.lusb_handle;
	int timeout = in->master.usb.timeout ;

	do {
		int transferred ;
		int ret = transfer_function(usb, endpoint, (BYTE *) data+transferred_so_far, (int) length-transferred_so_far, &transferred, timeout) ;
		
		switch (ret ) {
			case 0:
				return 0 ;
			case LIBUSB_ERROR_TIMEOUT:
				if ( transferred == 0 ) {
					// timeout with no data
					libusb_clear_halt( usb, endpoint ) ;
					return ret ;
				}
				transferred_so_far += transferred ;
				break ;
			default:
				// error
				libusb_clear_halt( usb, endpoint ) ;
				return ret ;
		}
	} while (1) ;
}	

void DS9490_port_setup( libusb_device * dev, struct port_in * pin )
{
	struct connection_in * in = pin->first ;
	
	in->master.usb.lusb_handle = NULL ;
	in->master.usb.lusb_dev = dev ;
	pin->type = ct_usb ;
	pin->busmode = bus_usb;

	in->flex = 1 ; // Michael Markstaller suggests this
	in->Adapter = adapter_DS9490;	/* OWFS assigned value */
	in->adapter_name = "DS9490";
	memset( in->master.usb.ds1420_address, 0, SERIAL_NUMBER_SIZE ) ;
	
	SAFEFREE(DEVICENAME(in)) ;

	if ( dev == NULL ) {
		in->master.usb.address = -1 ;
		in->master.usb.bus_number = -1 ;
		DEVICENAME(in) = owstrdup("") ;
	} else {
		size_t len = 32 ;
		int sn_ret ;
		
		in->master.usb.address = libusb_get_device_address( dev ) ;
		in->master.usb.bus_number = libusb_get_bus_number( dev ) ;

		DEVICENAME(in) = owmalloc( len+1 ) ;
		if ( DEVICENAME(in) == NULL ) {
			return ;
		}

		UCLIBCLOCK ;
		sn_ret = snprintf(DEVICENAME(in), len, "%.d:%.d", in->master.usb.bus_number, in->master.usb.address) ;
		UCLIBCUNLOCK ;

		if (sn_ret <= 0) {
			DEVICENAME(in)[0] = '\0' ;
		}
	}
}

#endif							/* OW_USB */

