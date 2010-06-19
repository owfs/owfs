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
#include "ow_connection.h"

static void USB_monitor_close(struct connection_in *in);
static GOOD_OR_BAD browse_in_use(const struct connection_in * in_selected) ;

/* Device-specific functions */
GOOD_OR_BAD USB_monitor_detect(struct connection_in *in)
{
	in->file_descriptor = FILE_DESCRIPTOR_BAD;
	in->iroutines.detect = USB_monitor_detect;
	in->Adapter = adapter_browse_monitor;	/* OWFS assigned value */
	in->iroutines.reset = NULL;
	in->iroutines.next_both = NULL;
	in->iroutines.PowerByte = NULL;
	in->iroutines.ProgramPulse = NULL;
	in->iroutines.sendback_data = NULL;
	in->iroutines.sendback_bits = NULL;
	in->iroutines.select = NULL;
	in->iroutines.reconnect = NULL;
	in->iroutines.close = USB_monitor_close;
	in->iroutines.flags = ADAP_FLAG_sham;
	in->adapter_name = "USB scan";
	in->busmode = bus_usb_monitor ;
	
	RETURN_BAD_IF_BAD( usb_monitor_in_use(in) ) ;

	return gbGOOD ;
}

static GOOD_OR_BAD usb_monitor_in_use(const struct connection_in * in_selected)
{
	struct connection_in *in;

	for (in = Inbound_Control.head; in != NULL; in = in->next) {
		if ( in == in_selected ) {
			continue ;
		}
		if ( in->busmode != bus_usb_monitor ) {
			continue ;
		}
		return gbBAD ;
	}
	return gbGOOD;					// not found in the current inbound list
}

static void USB_monitor_close(struct connection_in *in)
{
#if OW_ZERO
	if (in->connin.browse.bonjour_browse && (libdnssd != NULL)) {
		DNSServiceRefDeallocate(in->connin.browse.bonjour_browse);
		in->connin.browse.bonjour_browse = 0 ;
	}
	if ( in->connin.browse.avahi_poll != NULL ) {
		// Signal avahi loop to quit (in ow_avahi_browse.c)
		// and clean up for itself
		avahi_simple_poll_quit(in->connin.browse.avahi_poll);
	}
#endif
}
