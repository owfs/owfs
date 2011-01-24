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
#include "ow_devices.h"
#include "ow_pid.h"

static void IgnoreSignals(void);
static void SetupTemperatureLimits( void );
static void SetupInboundConnections(void);
static GOOD_OR_BAD SetupSingleInboundConnection( struct connection_in * in ) ;

/* Start the owlib process -- already in background */
GOOD_OR_BAD LibStart(void)
{
	Globals.zero = zero_none ;
#if OW_ZERO
#if OW_MT
	// Avahi only implementd with multithreading
	if ( GOOD( OW_Load_avahi_library()) ) {
		Globals.zero = zero_avahi ;
		//OW_Load_dnssd_library() ; // until avahi browse implemented
	} else
#endif
	if ( OW_Load_dnssd_library() == 0 ) {
		Globals.zero = zero_bonjour ;
	}
#endif

	/* Initialize random number generator, make sure fake devices get the same
	 * id each time */
	srand(1);

	SetupTemperatureLimits() ;
	
	MONITOR_WLOCK ;
	SetupInboundConnections();
	MONITOR_WUNLOCK ;

	// Signal handlers
	IgnoreSignals();
	
	if ( Inbound_Control.head == NO_CONNECTION ) {
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
	LEVEL_DEBUG("Globals temp limits %gC %gC (for simulated adapters)",Globals.templow,Globals.temphigh);
}

static void SetupInboundConnections(void)
{
	struct connection_in *in = Inbound_Control.head;

	// cycle through connections analyzing them
	while (in != NO_CONNECTION) {
		struct connection_in * next = in->next ;
		if ( BAD( SetupSingleInboundConnection(in) ) ) {				
			/* flag that that the adapter initiation was unsuccessful */
			STAT_ADD1_BUS(e_bus_detect_errors, in);
			RemoveIn( in ) ;
		}
		in = next ;
	}
}
	
static GOOD_OR_BAD SetupSingleInboundConnection( struct connection_in * in )
{
	switch (get_busmode(in)) {

	case bus_zero:
		if ( BAD( Zero_detect(in) )) {
			LEVEL_CONNECT("Cannot open server at %s", SOC(in)->devicename);
			return gbBAD ;
		}
		break;

	case bus_server:
		if (BAD( Server_detect(in)) ) {
			LEVEL_CONNECT("Cannot open server at %s", SOC(in)->devicename);
			return gbBAD ;
		}
		break;

	case bus_serial:
		/* Set up DS2480/LINK interface */
		if ( BAD( DS2480_detect(in) )) {
			LEVEL_CONNECT("Cannot detect DS2480 or LINK interface on %s.", SOC(in)->devicename);
		} else {
			return gbGOOD ;
		}
		// Fall Through
		in->adapter_name = "DS9097";	// need to set adapter name for this approach to passive adapter

	case bus_passive:
		if ( BAD( DS9097_detect(in) )) {
			LEVEL_DEFAULT("Cannot detect DS9097 (passive) interface on %s.", SOC(in)->devicename);
			return gbBAD ;
		}
		break;
		
	case bus_xport:
		if ( BAD( DS2480_detect(in) )) {
			LEVEL_DEFAULT("Cannot detect DS2480B via telnet interface on %s.", SOC(in)->devicename);
			return gbBAD ;
		}
		break;
		
	case bus_i2c:
#if OW_I2C
		if ( BAD( DS2482_detect(in) )) {
			LEVEL_CONNECT("Cannot detect an i2c DS2482-x00 on %s", SOC(in)->devicename);
			return gbBAD ;
		}
#endif							/* OW_I2C */
		break;

	case bus_ha7net:
#if OW_HA7
		if ( BAD( HA7_detect(in) )) {
			LEVEL_CONNECT("Cannot detect an HA7net server on %s", SOC(in)->devicename);
			return gbBAD ;
		}
#endif                          /* OW_HA7 */
	break;

	case bus_enet:
#if OW_HA7
		if ( BAD( OWServer_Enet_detect(in) )) {
			LEVEL_CONNECT("Cannot detect an OWServer_Enet on %s", SOC(in)->devicename);
			return gbBAD ;
		}
#endif                          /* OW_HA7 */
	break;

	case bus_ha5:
		if ( BAD( HA5_detect(in) )) {
			LEVEL_CONNECT("Cannot detect an HA5 on %s", SOC(in)->devicename);
			return gbBAD ;
		}
		break;

	case bus_ha7e:
		if ( BAD( HA7E_detect(in) )) {
			LEVEL_CONNECT("Cannot detect an HA7E/HA7S on %s", SOC(in)->devicename);
			return gbBAD ;
		}
		break;

	case bus_parallel:
#if OW_PARPORT
		if ( BAD( DS1410_detect(in) )) {
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
		if ( BAD( DS9490_detect(in) )) {
			LEVEL_DEFAULT("Cannot open USB bus master");
			return gbBAD ;
		}
#endif							/* OW_USB */
		break;

	case bus_link:
		if ( BAD( LINK_detect(in) )) {
			LEVEL_CONNECT("Cannot open LINK bus master at %s", SOC(in)->devicename);
			return gbBAD ;
		}
		break;

	case bus_etherweather:
		if ( BAD( EtherWeather_detect(in) )) {
			LEVEL_CONNECT("Cannot detect an EtherWeather server on %s", SOC(in)->devicename);
			return gbBAD ;
		}
		break;

	case bus_browse:
		RETURN_BAD_IF_BAD( Browse_detect(in) ) ;
		break;

	case bus_fake:
		Fake_detect(in);	// never fails
		break;

	case bus_tester:
		Tester_detect(in);	// never fails
		break;

	case bus_mock:
		Mock_detect(in);	// never fails
		break;

	case bus_w1_monitor:
		RETURN_BAD_IF_BAD( W1_monitor_detect(in) ) ;
		break;

	case bus_usb_monitor:
		RETURN_BAD_IF_BAD( USB_monitor_detect(in) ) ;
		break;

	case bus_w1:
		/* w1 is different: it is a dynamic list of adapters */
		/* the scanning starts with "W1_Browse" in LibStart and continues in it's own thread */
		/* No connection_in entries should have been created for w1 yet */
		break ;
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

