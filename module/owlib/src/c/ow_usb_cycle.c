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
#include "ow_usb_msg.h"
#include "ow_usb_cycle.h"

#if OW_USB

/* ------------------------------------------------------------ */
/* --- USB bus scaning -----------------------------------------*/

void USB_first(struct usb_list *ul)
{
	usb_init();
	usb_find_busses();
	usb_find_devices();
	ul->bus = usb_get_busses();
	ul->dev = NULL;
}

GOOD_OR_BAD USB_next(struct usb_list *ul)
{
	while (ul->bus) {
		// First pass, look for next device
		if (ul->dev == NULL) {
			ul->dev = ul->bus->devices;
		} else {				// New bus, find first device
			ul->dev = ul->dev->next;
		}
		if (ul->dev) {			// device found
			if (ul->dev->descriptor.idVendor != DS2490_USB_VENDOR || ul->dev->descriptor.idProduct != DS2490_USB_PRODUCT) {
				continue;		// not DS9490
			}
			LEVEL_CONNECT("Bus master found: %s/%s", ul->bus->dirname, ul->dev->filename);
			return gbGOOD;
		} else {
			ul->bus = ul->bus->next;
			ul->dev = NULL;
		}
	}
	return gbBAD;
}

#endif							/* OW_USB */
