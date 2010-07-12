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
#include "ow_usb_msg.h"

static void USB_monitor_close(struct connection_in *in);
static GOOD_OR_BAD usb_monitor_in_use(const struct connection_in * in_selected) ;
static void USB_scan_for_adapters(void) ;
static void * USB_monitor_loop( void * v );

/* Device-specific functions */
GOOD_OR_BAD USB_monitor_detect(struct connection_in *in)
{
	struct address_pair ap ;
	pthread_t thread ;
	
	Parse_Address( in->name, &ap ) ;
	if ( ap.first.type == address_numeric ) {
		Globals.usb_scan_interval = ap.first.number ;
	} else {
		Globals.usb_scan_interval = DEFAULT_USB_SCAN_INTERVAL ;
	}
	Free_Address( &ap ) ;

	SAFEFREE(in->name) ;
	in->name = owstrdup("USB bus monitor") ;

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
	
	if ( pipe( in->connin.usb_monitor.shutdown_pipe ) != 0 ) {
		ERROR_DEFAULT("Cannot allocate a shutdown pipe. The program shutdown may be messy");
		in->connin.usb_monitor.shutdown_pipe[fd_pipe_read] = FILE_DESCRIPTOR_BAD ;
		in->connin.usb_monitor.shutdown_pipe[fd_pipe_write] = FILE_DESCRIPTOR_BAD ;
	}

	RETURN_BAD_IF_BAD( usb_monitor_in_use(in) ) ;

	if ( pthread_create(&thread, NULL, USB_monitor_loop, (void *) in) != 0 ) {
		ERROR_CALL("Cannot create the USB monitoring program thread");
		return gbBAD ;
	}

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
	SAFEFREE(in->name) ;

	if ( FILE_DESCRIPTOR_VALID( in->connin.usb_monitor.shutdown_pipe[fd_pipe_write] ) ) {
		ignore_result = write( in->connin.usb_monitor.shutdown_pipe[fd_pipe_write],"X",1) ; //dummy payload
	}		
	Test_and_Close_Pipe(in->connin.usb_monitor.shutdown_pipe) ;
}

static void * USB_monitor_loop( void * v )
{
	struct connection_in * in = v ;
	FILE_DESCRIPTOR_OR_ERROR file_descriptor = in->connin.usb_monitor.shutdown_pipe[fd_pipe_read] ;

	pthread_detach(pthread_self());
	
	do {
		fd_set readset;
		struct timeval tv = { Globals.usb_scan_interval, 0, };
		
		/* Initialize readset */
		FD_ZERO(&readset);
		if ( FILE_DESCRIPTOR_VALID( file_descriptor ) ) {
			FD_SET(file_descriptor, &readset);
		}

		if ( select( file_descriptor+1, &readset, NULL, NULL, &tv ) != 0 ) {
			break ; // don't scan any more -- perhaps a close?
		}
		USB_scan_for_adapters() ;
	} while (1) ;
	
	return NULL ;
}

/* Open a DS9490  -- low level code (to allow for repeats)  */
static void USB_scan_for_adapters(void)
{
	struct usb_list ul;

	MONITOR_RLOCK ;
	
	LEVEL_DEBUG("USB SCAN!");
	USB_first(&ul);
	while ( GOOD(USB_next(&ul)) ) {
		struct connection_in * in = NewIn(NULL) ;
		if ( in == NULL ) {
			return ;
		}
		in->name = DS9490_device_name(&ul) ;
		if ( BAD( DS9490_detect(in)) ) {
			// Remove the extra connection
			RemoveIn(in);
		}
	}

	MONITOR_RUNLOCK ;
}
