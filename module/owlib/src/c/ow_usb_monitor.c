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

#if OW_USB && OW_MT

static void USB_monitor_close(struct connection_in *in);
static GOOD_OR_BAD usb_monitor_in_use(const struct connection_in * in_selected) ;
static void USB_scan_for_adapters(void) ;
static void * USB_monitor_loop( void * v );

/* Device-specific functions */
GOOD_OR_BAD USB_monitor_detect(struct port_in *pin)
{
	struct connection_in * in = pin->first ;
	struct address_pair ap ;
	pthread_t thread ;
	
	Parse_Address( SOC(in)->devicename, &ap ) ;
	SOC(in)->type = ct_none ;
	if ( ap.first.type == address_numeric ) {
		Globals.usb_scan_interval = ap.first.number ;
	} else {
		Globals.usb_scan_interval = DEFAULT_USB_SCAN_INTERVAL ;
	}
	Free_Address( &ap ) ;

	SAFEFREE(SOC(in)->devicename) ;
	SOC(in)->devicename = owstrdup("USB bus monitor") ;

	SOC(in)->file_descriptor = FILE_DESCRIPTOR_BAD;
	in->iroutines.detect = USB_monitor_detect;
	in->Adapter = adapter_browse_monitor;	/* OWFS assigned value */
	in->iroutines.reset = NO_RESET_ROUTINE;
	in->iroutines.next_both = NO_NEXT_BOTH_ROUTINE;
	in->iroutines.PowerByte = NO_POWERBYTE_ROUTINE;
	in->iroutines.ProgramPulse = NO_PROGRAMPULSE_ROUTINE;
	in->iroutines.sendback_data = NO_SENDBACKDATA_ROUTINE;
	in->iroutines.sendback_bits = NO_SENDBACKBITS_ROUTINE;
	in->iroutines.select = NO_SELECT_ROUTINE;
	in->iroutines.select_and_sendback = NO_SELECTANDSENDBACK_ROUTINE ;
	in->iroutines.reconnect = NO_RECONNECT_ROUTINE;
	in->iroutines.close = USB_monitor_close;
	in->iroutines.flags = ADAP_FLAG_sham;
	in->adapter_name = "USB scan";
	in->busmode = bus_usb_monitor ;
	
	Init_Pipe( in->master.usb_monitor.shutdown_pipe ) ;
	if ( pipe( in->master.usb_monitor.shutdown_pipe ) != 0 ) {
		ERROR_DEFAULT("Cannot allocate a shutdown pipe. The program shutdown may be messy");
		Init_Pipe( in->master.usb_monitor.shutdown_pipe ) ;
	}
	fcntl (in->master.usb_monitor.shutdown_pipe[fd_pipe_read], F_SETFD, FD_CLOEXEC); // for safe forking
	fcntl (in->master.usb_monitor.shutdown_pipe[fd_pipe_write], F_SETFD, FD_CLOEXEC); // for safe forking

	RETURN_BAD_IF_BAD( usb_monitor_in_use(in) ) ;

	if ( pthread_create(&thread, DEFAULT_THREAD_ATTR, USB_monitor_loop, (void *) in) != 0 ) {
		ERROR_CALL("Cannot create the USB monitoring program thread");
		return gbBAD ;
	}

	return gbGOOD ;
}

static GOOD_OR_BAD usb_monitor_in_use(const struct connection_in * in_selected)
{
	struct port_in * pin ;
	
	for ( pin = Inbound_Control.head_port ; pin != NULL ; pin = pin->next ) {
		struct connection_in * cin;

		for (cin = pin->first; cin != NO_CONNECTION; cin = cin->next) {
			if ( cin == in_selected ) {
				continue ;
			}
			if ( cin->busmode != bus_usb_monitor ) {
				continue ;
			}
			return gbBAD ;
		}
	}
	return gbGOOD;					// not found in the current inbound list
}

static void USB_monitor_close(struct connection_in *in)
{
	SAFEFREE(SOC(in)->devicename) ;

	if ( FILE_DESCRIPTOR_VALID( in->master.usb_monitor.shutdown_pipe[fd_pipe_write] ) ) {
		ignore_result = write( in->master.usb_monitor.shutdown_pipe[fd_pipe_write],"X",1) ; //dummy payload
	}		
	Test_and_Close_Pipe(in->master.usb_monitor.shutdown_pipe) ;
}

static void * USB_monitor_loop( void * v )
{
	struct connection_in * in = v ;
	FILE_DESCRIPTOR_OR_ERROR file_descriptor = in->master.usb_monitor.shutdown_pipe[fd_pipe_read] ;

	DETACH_THREAD;
	
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
	
	return VOID_RETURN ;
}

/* Open a DS9490  -- low level code (to allow for repeats)  */
static void USB_scan_for_adapters(void)
{
	struct usb_list ul;

	MONITOR_RLOCK ;
	
	LEVEL_DEBUG("USB SCAN!");
	USB_first(&ul);
	while ( GOOD(USB_next(&ul)) ) {
		struct port_in * pin = AllocPort(NULL) ;
		struct connection_in * in ;
		if ( pin == NULL ) {
			return ;
		}
		in = pin->first ;
		SOC(in)->devicename = DS9490_device_name(&ul) ;
		SOC(in)->type = ct_usb ;
		
		// Can do detect. Becasue the name makes this a specific adapter (USB pair)
		// we won't do a directory and won't add the directory and devices with the wrong index
		if ( BAD( DS9490_detect(pin)) ) {
			// Remove the extra connection
			RemovePort(pin);
		} else {
			// Add the device, but no need to check for bad match
			Add_InFlight( NULL, pin ) ;
		}
	}

	MONITOR_RUNLOCK ;
}

#else /*  OW_USB && OW_MT */

GOOD_OR_BAD USB_monitor_detect(struct port_in *pin)
{
	(void) pin ;
	LEVEL_CALL( "USB monitoring support not enabled" ) ;
	return gbBAD ;
}

#endif /*  OW_USB && OW_MT */
