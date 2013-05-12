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

static void ENET_monitor_close(struct connection_in *in);
static GOOD_OR_BAD ENET_monitor_in_use(const struct port_in * pin) ;
static void ENET_scan_for_adapters(void) ;
static void * ENET_monitor_loop( void * v );

/* Device-specific functions */
GOOD_OR_BAD ENET_monitor_detect(struct port_in *pin)
{
	struct connection_in * in = pin->first ;
	struct address_pair ap ;
	pthread_t thread ;
	
	/* init_data has form "scan" or "scan:15" (15 seconds) */
	Parse_Address( pin->init_data, &ap ) ;
	switch ( ap.entries ) {
		case 0:
			Globals.enet_scan_interval = DEFAULT_ENET_SCAN_INTERVAL ;
			break ;
		case 1:
			switch( ap.first.type ) {
				case address_numeric:
					Globals.enet_scan_interval = ap.first.number ;
					break ;
				default:
					Globals.enet_scan_interval = DEFAULT_ENET_SCAN_INTERVAL ;
					break ;
			}
			break ;
		case 2:
			switch( ap.second.type ) {
				case address_numeric:
					Globals.enet_scan_interval = ap.second.number ;
					break ;
				default:
					Globals.enet_scan_interval = DEFAULT_ENET_SCAN_INTERVAL ;
					break ;
			}
			break ;
	}
	Free_Address( &ap ) ;

	pin->type = ct_none ;

	// Device name will not be init_data copy
	SAFEFREE(DEVICENAME(in)) ;
	DEVICENAME(in) = owstrdup("ENET bus monitor") ;

	pin->file_descriptor = FILE_DESCRIPTOR_BAD;
	in->iroutines.detect = ENET_monitor_detect;
	in->Adapter = adapter_enet_monitor;
	in->iroutines.reset = NO_RESET_ROUTINE;
	in->iroutines.next_both = NO_NEXT_BOTH_ROUTINE;
	in->iroutines.PowerByte = NO_POWERBYTE_ROUTINE;
	in->iroutines.ProgramPulse = NO_PROGRAMPULSE_ROUTINE;
	in->iroutines.sendback_data = NO_SENDBACKDATA_ROUTINE;
	in->iroutines.sendback_bits = NO_SENDBACKBITS_ROUTINE;
	in->iroutines.select = NO_SELECT_ROUTINE;
	in->iroutines.select_and_sendback = NO_SELECTANDSENDBACK_ROUTINE ;
	in->iroutines.set_config = NO_SET_CONFIG_ROUTINE;
	in->iroutines.get_config = NO_GET_CONFIG_ROUTINE;
	in->iroutines.reconnect = NO_RECONNECT_ROUTINE;
	in->iroutines.close = ENET_monitor_close;
	in->iroutines.flags = ADAP_FLAG_sham;
	in->adapter_name = "ENET scan";
	pin->busmode = bus_enet_monitor ; // repeat since can come via usb=scan
	
	Init_Pipe( in->master.enet_monitor.shutdown_pipe ) ;
	if ( pipe( in->master.enet_monitor.shutdown_pipe ) != 0 ) {
		ERROR_DEFAULT("Cannot allocate a shutdown pipe. The program shutdown may be messy");
		Init_Pipe( in->master.enet_monitor.shutdown_pipe ) ;
	}
	if ( BAD( ENET_monitor_in_use(pin) ) ) {
		LEVEL_CONNECT("Second call for ENET scanning ignored") ;
		return gbBAD ;
	}

	if ( pthread_create(&thread, DEFAULT_THREAD_ATTR, ENET_monitor_loop, (void *) in) != 0 ) {
		ERROR_CALL("Cannot create the ENET monitoring program thread");
		return gbBAD ;
	}

	return gbGOOD ;
}

static GOOD_OR_BAD ENET_monitor_in_use(const struct port_in * pin)
{
	struct port_in * p_index = Inbound_Control.head_port ;
	
	while ( p_index != NULL ) {
		if ( p_index->busmode == bus_enet_monitor ) {
			if ( pin != p_index ) {
				return gbBAD ;
			}
		}
		p_index = p_index->next ; 
	}
	return gbGOOD;					// not found in the current inbound list
}

static void ENET_monitor_close(struct connection_in *in)
{
	if ( FILE_DESCRIPTOR_VALID( in->master.enet_monitor.shutdown_pipe[fd_pipe_write] ) ) {
		ignore_result = write( in->master.enet_monitor.shutdown_pipe[fd_pipe_write],"X",1) ; //dummy payload
	}		
	Test_and_Close_Pipe(in->master.enet_monitor.shutdown_pipe) ;
}

static void * ENET_monitor_loop( void * v )
{
	struct connection_in * in = v ;
	FILE_DESCRIPTOR_OR_ERROR file_descriptor = in->master.enet_monitor.shutdown_pipe[fd_pipe_read] ;

	DETACH_THREAD;
	
	do {
		fd_set readset;
		struct timeval tv = { Globals.enet_scan_interval, 0, };
		
		/* Initialize readset */
		FD_ZERO(&readset);
		if ( FILE_DESCRIPTOR_VALID( file_descriptor ) ) {
			FD_SET(file_descriptor, &readset);
		}

		ENET_scan_for_adapters() ;

		if ( select( file_descriptor+1, &readset, NULL, NULL, &tv ) != 0 ) {
			break ; // don't scan any more -- perhaps a close?
		}
	} while (1) ;
	
	return VOID_RETURN ;
}

static void ENET_scan_for_adapters(void)
{
	struct enet_list elist ;
	struct enet_member * em ;

	MONITOR_RLOCK ;
	
	LEVEL_DEBUG("ENET SCAN!");

	enet_list_init( &elist ) ;
	Find_ENET_all( &elist ) ;

	em = elist.head ;
	if ( em == NULL ) {
	} else {
		while ( em != NULL ) {
			struct port_in * pnew = AllocPort( NULL ) ;
			if ( pnew == NULL ) {
				break ;
			}
			if ( GOOD(OWServer_Enet_setup( em->name, em->version, pnew )) ) {
				// Add the device, but no need to check for bad match
				Add_InFlight( NULL, pnew ) ;
			}
			em = em->next ;
		}
	}

	enet_list_kill( &elist ) ;

	MONITOR_RUNLOCK ;
}
