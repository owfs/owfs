/*
$Id$
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

#ifndef OW_USB_MSG_H			/* tedious wrapper */
#define OW_USB_MSG_H

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

/* All the rest of the code sees is the DS9490_detect routine and the iroutine structure */

GOOD_OR_BAD USB_Control_Msg(BYTE bRequest, UINT wValue, UINT wIndex, const struct parsedname *pn);
GOOD_OR_BAD DS9490_open(struct usb_list *ul, struct connection_in *in);

#define DS9490_getstatus_BUFFER_LENGTH ( 32 + 1 )
RESET_TYPE DS9490_getstatus(BYTE * buffer, int * readlen, const struct parsedname *pn);
SIZE_OR_ERROR DS9490_read(BYTE * buf, size_t size, const struct parsedname *pn);
SIZE_OR_ERROR DS9490_write(const BYTE * buf, size_t size, const struct parsedname *pn);
void DS9490_close(struct connection_in *in);

// Mode Command Code Constants
#define ONEWIREDEVICEDETECT               0xA5
#define COMMCMDERRORRESULT_NRS            0x01
#define COMMCMDERRORRESULT_SH             0x02

extern char badUSBname[] ;

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

// Device Status Flags
#define STATUSFLAGS_SPUA                       0x01	// if set Strong Pull-up is active
#define STATUSFLAGS_PRGA                       0x02	// if set a 12V programming pulse is being generated
#define STATUSFLAGS_12VP                       0x04	// if set the external 12V programming voltage is present
#define STATUSFLAGS_PMOD                       0x08	// if set the DS2490 powered from USB and external sources
#define STATUSFLAGS_HALT                       0x10	// if set the DS2490 is currently halted
#define STATUSFLAGS_IDLE                       0x20	// if set the DS2490 is currently idle

#endif							/* OW_USB */

#endif /* OW_USB_MSG_H */
