/*
    OWFS -- One-Wire filesystem
    OWHTTPD -- One-Wire Web Server
    Written 2003 Paul H Alfille
    email: paul.alfille@gmail.com
    Released under the GPL
    See the header file: ow.h for full attribution
    1wire/iButton system from Dallas Semiconductor
*/

#include <config.h>
#include "owfs_config.h"
#include "ow.h"
#include "ow_devices.h"
#include "ow_pid.h"

static void IgnoreSignals(void);
static void SetupTemperatureLimits( void );
static void SetupInboundConnections(void);
static GOOD_OR_BAD SetupSingleInboundConnection( struct port_in * pin ) ;

/* Start the owlib process -- already in background */
GOOD_OR_BAD LibStart(void* v)
{
	/* Start configuration monitoring */
	Config_Monitor_Watch(v) ;
	
	/* Build device and filetype arrays (including externals) */
	DeviceSort();

	Globals.zero = zero_none ;
#if OW_ZERO
	if ( OW_Load_dnssd_library() == 0 ) {
		Globals.zero = zero_bonjour ;
	}
#endif /* OW_ZERO */

	/* Initialize random number generator, make sure fake devices get the same
	 * id each time */
	srand(1);

	SetupTemperatureLimits() ;
	
	MONITOR_WLOCK ;
	SetupInboundConnections();
	MONITOR_WUNLOCK ;

	// Signal handlers
	IgnoreSignals();
	
	if ( Inbound_Control.head_port == NULL ) {
		LEVEL_DEFAULT("No valid 1-wire buses found");
		return gbBAD ;
	}
	return gbGOOD ;
}

// only changes FAKE and MOCK temp limits
// do it here after options are parsed to allow correction forr temperature scale
static void SetupTemperatureLimits( void )
{
	struct parsedname pn;
	
	FS_ParsedName_Placeholder(&pn);	// minimal parsename -- no destroy needed

	if ( Globals.templow < GLOBAL_UNTOUCHED_TEMP_LIMIT + 1 ) {
		Globals.templow = 0. ; // freezing point
	} else {
		Globals.templow = fromTemperature(Globals.templow,&pn) ; // internal scale
	}
	
	if ( Globals.temphigh < GLOBAL_UNTOUCHED_TEMP_LIMIT + 1 ) {
		Globals.temphigh = 100. ; // boiling point
	} else {
		Globals.temphigh = fromTemperature(Globals.temphigh,&pn) ; // internal scale
	}
	LEVEL_DEBUG("Global temp limit %gC to %gC (for fake and mock adapters)",Globals.templow,Globals.temphigh);
}

static void SetupInboundConnections(void)
{
	struct port_in *pin = Inbound_Control.head_port;

	// cycle through connections analyzing them
	while (pin != NULL) {
		struct port_in * next = pin->next ; // read before potential delete
		if ( BAD( SetupSingleInboundConnection(pin) ) ) {				
			RemovePort( pin ) ;
		}
		pin = next ;
	}
}
	
static GOOD_OR_BAD SetupSingleInboundConnection( struct port_in * pin )
{
	struct connection_in * in = pin->first ;
	switch (pin->busmode) {

	case bus_zero:
		if ( BAD( Zero_detect(pin) )) {
			LEVEL_CONNECT("Cannot open server at %s", DEVICENAME(in));
			return gbBAD ;
		}
		break;

	case bus_server:
		if (BAD( Server_detect(pin)) ) {
			LEVEL_CONNECT("Cannot open server at %s -- first attempt.", DEVICENAME(in));
			sleep(5); // delay to allow owserver to open it's listen socket
			if ( GOOD( Server_detect(pin)) ) {
				break ;
			}
			LEVEL_CONNECT("Cannot open server at %s -- second (and final) attempt.", DEVICENAME(in));
			return gbBAD ;
		}
		break;

	case bus_serial:
		/* Set up DS2480/LINK interface */
		if ( BAD( DS2480_detect(pin) )) {
			LEVEL_CONNECT("Cannot detect DS2480 or LINK interface on %s.", DEVICENAME(in));
		} else {
			return gbGOOD ;
		}
		// Fall Through
		in->adapter_name = "DS9097";	// need to set adapter name for this approach to passive adapter
		__attribute__ ((fallthrough));
	case bus_passive:
		if ( BAD( DS9097_detect(pin) )) {
			LEVEL_DEFAULT("Cannot detect DS9097 (passive) interface on %s.", DEVICENAME(in));
			return gbBAD ;
		}
		break;
		
	case bus_xport:
		if ( BAD( DS2480_detect(pin) )) {
			LEVEL_DEFAULT("Cannot detect DS2480B via telnet interface on %s.", DEVICENAME(in));
			return gbBAD ;
		}
		break;
		
	case bus_i2c:
#if OW_I2C
		if ( BAD( DS2482_detect(pin) )) {
			LEVEL_CONNECT("Cannot detect an i2c DS2482-x00 on %s", DEVICENAME(in));
			return gbBAD ;
		}
#endif							/* OW_I2C */
		break;

	case bus_ha7net:
		if ( BAD( HA7_detect(pin) )) {
			LEVEL_CONNECT("Cannot detect an HA7net server on %s", DEVICENAME(in));
			return gbBAD ;
		}
	break;

	case bus_enet:
		if ( BAD( OWServer_Enet_detect(pin) )) {
			LEVEL_CONNECT("Cannot detect an OWServer_Enet on %s", DEVICENAME(in));
			return gbBAD ;
		}
	break;

	case bus_ha5:
		if ( BAD( HA5_detect(pin) )) {
			LEVEL_CONNECT("Cannot detect an HA5 on %s", DEVICENAME(in));
			return gbBAD ;
		}
		break;

	case bus_ha7e:
		if ( BAD( HA7E_detect(pin) )) {
			LEVEL_CONNECT("Cannot detect an HA7E/HA7S on %s", DEVICENAME(in));
			return gbBAD ;
		}
		break;

	case bus_ds1wm:
		if ( BAD( DS1WM_detect(pin) )) {
			LEVEL_CONNECT("Cannot detect an DS1WM synthesized bus master at %s", DEVICENAME(in));
			return gbBAD ;
		}
		break;

	case bus_k1wm:
		if ( BAD( K1WM_detect(pin) )) {
			LEVEL_CONNECT("Cannot detect an K1WM synthesized bus master at %s", DEVICENAME(in));
			return gbBAD ;
		}
		break;

	case bus_parallel:
#if OW_PARPORT
		if ( BAD( DS1410_detect(pin) )) {
			LEVEL_DEFAULT("Cannot detect the DS1410E parallel bus master");
			return gbBAD ;
		}
#endif							/* OW_PARPORT */
		break;

	case bus_usb:
#if OW_USB
		/* in->master.usb.ds1420_address should be set to identify the
		 * adapter just in case it's disconnected. It's done in the
		 * DS9490_next_both() if not set. */
		if ( BAD( DS9490_detect(pin) )) {
			LEVEL_DEFAULT("Cannot open USB bus master");
			return gbBAD ;
		}
#endif							/* OW_USB */
		break;

	case bus_link:
		if ( BAD( LINK_detect(pin) )) {
			LEVEL_CONNECT("Cannot open LINK bus master at %s", DEVICENAME(in));
			return gbBAD ;
		}
		break;

	case bus_masterhub:
		if ( BAD( MasterHub_detect(pin) )) {
			LEVEL_CONNECT("Cannot open Hobby Boards MasterHub bus master at %s", DEVICENAME(in));
			return gbBAD ;
		}
		break;

	case bus_pbm:
		if ( BAD( PBM_detect(pin) )) {
			LEVEL_CONNECT("Cannot open PBM bus master at %s", DEVICENAME(in));
			return gbBAD ;
		}
		break;

	case bus_etherweather:
		if ( BAD( EtherWeather_detect(pin) )) {
			LEVEL_CONNECT("Cannot detect an EtherWeather server on %s", DEVICENAME(in));
			return gbBAD ;
		}
		break;

	case bus_browse:
		RETURN_BAD_IF_BAD( Browse_detect(pin) ) ;
		break;

	case bus_fake:
		Fake_detect(pin);	// never fails
		break;

	case bus_tester:
		Tester_detect(pin);	// never fails
		break;

	case bus_mock:
		Mock_detect(pin);	// never fails
		break;

	case bus_w1_monitor:
		RETURN_BAD_IF_BAD( W1_monitor_detect(pin) ) ;
		break;

	case bus_usb_monitor:
#if OW_USB
		RETURN_BAD_IF_BAD( USB_monitor_detect(pin) ) ;
#endif /* OW_USB */
		break;

	case bus_w1:
		/* w1 is different: it is a dynamic list of adapters */
		/* the scanning starts with "W1_Browse" in LibStart and continues in it's own thread */
		/* No connection_in entries should have been created for w1 yet */
		break ;

	case bus_external:
		RETURN_BAD_IF_BAD( External_detect(pin) ) ;
		break;

	case bus_bad:
	default:
		return gbBAD ;
		break;
	}
	return gbGOOD ;
}

static void IgnoreSignals(void)
{
	struct sigaction sa;

	memset(&sa, 0, sizeof(struct sigaction));
	sigemptyset(&(sa.sa_mask));

	sa.sa_flags = 0;
	sa.sa_handler = SIG_IGN;
	if (sigaction(SIGPIPE, &sa, NULL) == -1) {
		LEVEL_DEFAULT("Cannot ignore SIGPIPE");
		exit(0);
	}
}

