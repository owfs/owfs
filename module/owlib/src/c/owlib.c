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

/* Start the owlib process -- already in background */
int LibStart(void)
{
	/* Initialize random number generator, make sure fake devices get the same
	 * id each time */
	srand(1);

	SetupTemperatureLimits() ;
	
	SetupInboundConnections();
	if ( Globals.w1 ) {
#if OW_W1
		W1_Browse() ;
#else /* OW_W1 */
		LEVEL_DEFAULT("W1 (the linux kernel 1-wire system) is not supported");
#endif /* OW_W1 */
	}

	// zeroconf/Bonjour look for new services
	if (Globals.autoserver) {
#if OW_ZERO
		if (Globals.zero == zero_none ) {
			fprintf(stderr, "Zeroconf/Bonjour is disabled since Bonjour or Avahi library wasn't found.\n");
			exit(0);
		} else {
			OW_Browse();
		}
#else
		fprintf(stderr, "OWFS is compiled without Zeroconf/Bonjour support.\n");
		exit(0);
#endif
	}

	// Signal handlers
	IgnoreSignals();

	return 0;
}

// only changes FAKE and MOCK temp limits
// do it here after options are parsed to allow correction forr temperature scale
static void SetupTemperatureLimits( void )
{
	struct parsedname pn;
	
	FS_ParsedName(NULL, &pn);	// minimal parsename -- no destroy needed

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
	struct connection_in *in;

	for (in = Inbound_Control.head; in != NULL; in = in->next) {
		int ret = -ENOPROTOOPT;;
		BadAdapter_detect(in);	/* default "NOTSUP" calls */
		switch (get_busmode(in)) {

		case bus_zero:
			if ((ret = Zero_detect(in))) {
				LEVEL_CONNECT("Cannot open server at %s", in->name);
			}
			break;

		case bus_server:
			if ((ret = Server_detect(in))) {
				LEVEL_CONNECT("Cannot open server at %s", in->name);
			}
			break;

		case bus_serial:
			/* Set up DS2480/LINK interface */
			if ((ret = DS2480_detect(in))) {
				LEVEL_CONNECT("Cannot detect DS2480 or LINK interface on %s.", in->name);
			} else {
				break ;
			}
			// Fall Through
			BUS_close(in);
			BadAdapter_detect(in);	/* reset the methods */
			in->adapter_name = "DS9097";	// need to set adapter name for this approach to passive adapter

		case bus_passive:
			if ((ret = DS9097_detect(in))) {
				LEVEL_DEFAULT("Cannot detect DS9097 (passive) interface on %s.", in->name);
			}
			break;
			
		case bus_xport:
			if ((ret = DS2480_detect(in))) {
				LEVEL_DEFAULT("Cannot detect DS2480B via telnet interface on %s.", in->name);
			}
			break;
			
		case bus_i2c:
#if OW_I2C
			if ((ret = DS2482_detect(in))) {
				LEVEL_CONNECT("Cannot detect an i2c DS2482-x00 on %s", in->name);
			}
#endif							/* OW_I2C */
			break;

		case bus_ha7net:
#if OW_HA7
			if ((ret = HA7_detect(in))) {
			LEVEL_CONNECT("Cannot detect an HA7net server on %s", in->name);
			}
#endif                          /* OW_HA7 */
                break;

		case bus_enet:
#if OW_HA7
			if ((ret = OWServer_Enet_detect(in))) {
			LEVEL_CONNECT("Cannot detect an OWServer_Enet on %s", in->name);
			}
#endif                          /* OW_HA7 */
                break;

		case bus_ha5:
			if ((ret = HA5_detect(in))) {
				LEVEL_CONNECT("Cannot detect an HA5 on %s", in->name);
			}
			break;

		case bus_ha7e:
			if ((ret = HA7E_detect(in))) {
			LEVEL_CONNECT("Cannot detect an HA7E/HA7S on %s", in->name);
			}
			break;

		case bus_parallel:
#if OW_PARPORT
			if ((ret = DS1410_detect(in))) {
				LEVEL_DEFAULT("Cannot detect the DS1410E parallel bus master");
			}
#endif							/* OW_PARPORT */
			break;

		case bus_usb:
#if OW_USB
			/* in->connin.usb.ds1420_address should be set to identify the
			 * adapter just in case it's disconnected. It's done in the
			 * DS9490_next_both() if not set. */
			if ((ret = DS9490_detect(in))) {
				LEVEL_DEFAULT("Cannot open USB bus master");
			}
#endif							/* OW_USB */
			break;

		case bus_link:
		case bus_elink:
			if ((ret = LINK_detect(in))) {
				LEVEL_CONNECT("Cannot open LINK bus master at %s", in->name);
			}
			break;

		case bus_etherweather:
			if ((ret = EtherWeather_detect(in))) {
				LEVEL_CONNECT("Cannot detect an EtherWeather server on %s", in->name);
			}
			break;

		case bus_fake:
			Fake_detect(in);	// never fails
			ret = 0 ;
			break;

		case bus_tester:
			Tester_detect(in);	// never fails
			ret = 0 ;
			break;

		case bus_mock:
			Mock_detect(in);	// never fails
			ret = 0 ;
			break;

		case bus_w1:
			/* w1 is different: it is a dynamic list of adapters */
			/* the scanning starts with "W1_Browse" in LibStart and continues in it's own thread */
			/* No connection_in entries should have been created for w1 yet */
			ret = 0 ;
			break ;
		case bus_bad:
		default:
			break;

		}

		if (ret) {				/* flag that that the adapter initiation was unsuccessful */
			STAT_ADD1_BUS(e_bus_detect_errors, in);
			BUS_close(in);		/* can use adapter's close */
			BadAdapter_detect(in);	/* Set to default null assignments */
			in->busmode = bus_bad ;
		}
	}
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

