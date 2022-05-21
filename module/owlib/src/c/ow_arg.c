/*
    OWFS -- One-Wire filesystem
    OWHTTPD -- One-Wire Web Server
    Written 2003 Paul H Alfille
    email: paul.alfille@gmail.com
    Released under the GPL
    See the header file: ow.h for full attribution
    1wire/iButton system from Dallas Semiconductor
*/

// regex

/* ow_opt -- owlib specific command line options processing */

#include <config.h>
#include "owfs_config.h"
#include "ow.h"
#include "ow_connection.h"
#include "ow_usb_msg.h" // for DS9490_port_setup

enum arg_address { arg_addr_device, arg_addr_null,
	arg_addr_ip, arg_addr_colon,
	arg_addr_number, arg_addr_ftdi,
	arg_addr_other, arg_addr_error, } ;

static regex_t rx_dev;
static regex_t rx_num;
static regex_t rx_ip;
static regex_t rx_ftdi;
static regex_t rx_col;

static void regex_fini(void)
{
	regfree(&rx_dev);
	regfree(&rx_num);
	regfree(&rx_ip);
	regfree(&rx_ftdi);
	regfree(&rx_col);
}

static pthread_once_t regex_init_once = PTHREAD_ONCE_INIT;

static void regex_init(void)
{
	ow_regcomp(&rx_dev, "/", REG_NOSUB);
	ow_regcomp(&rx_num, "^[:digit:]+$", REG_NOSUB);
	ow_regcomp(&rx_ip, "[:digit:]{1,3}\\.[:digit:]{1,3}\\.[:digit:]{1,3}\\.[:digit:]{1,3}", REG_NOSUB);
	ow_regcomp(&rx_ftdi, "^ftdi:", REG_NOSUB);
	ow_regcomp(&rx_col, ":", REG_NOSUB);

	atexit(regex_fini);
}

static enum arg_address ArgType( const char * arg )
{
	pthread_once(&regex_init_once, regex_init);

	if ( arg == NULL ) {
		return arg_addr_null ;
	} else if ( ow_regexec( &rx_ip, arg, NULL ) == 0 ) {
		return arg_addr_ip ;
	} else if ( ow_regexec( &rx_ftdi, arg, NULL ) == 0 ) {
		return arg_addr_ftdi;
	} else if ( ow_regexec( &rx_col, arg, NULL ) == 0 ) {
		return arg_addr_colon ;
	} else if ( ow_regexec( &rx_dev, arg, NULL ) == 0 ) {
		return arg_addr_device ;
	} else if ( ow_regexec( &rx_num, arg, NULL ) == 0 ) {
		return arg_addr_number ;
	}
	return arg_addr_other ;
}

// Put initial port configuration in init_data and connection_in devicename
static void arg_data( const char * arg, struct port_in * pin )
{
	if ( arg == NULL ) {
		DEVICENAME(pin->first) = NULL ;
		pin->init_data = NULL ;
	} else {
		DEVICENAME(pin->first) = owstrdup(arg) ;
		pin->init_data = owstrdup(arg) ;
	}
}

// Test whether address is a serial port, or a serial over telnet (ser2net)
static GOOD_OR_BAD Serial_or_telnet( const char * arg, struct connection_in * in )
{
	switch( ArgType(arg) ) {
		case arg_addr_null:
		case arg_addr_error:
			LEVEL_DEFAULT("Error with device. Specify a serial port, or a serial-over-telnet network address");
			return gbBAD ;
		case arg_addr_device:
			in->pown->type = ct_serial ; // serial port
			break ;
		case arg_addr_ftdi:
#if OW_FTDI
			in->pown->type = ct_ftdi;
			break;
#else
			LEVEL_DEFAULT("FTDI support not included in compilation. Use generic serial device.");
			return gbBAD;
#endif
		case arg_addr_number: // port
		case arg_addr_colon:
		case arg_addr_ip:
		case arg_addr_other:
			in->pown->type = ct_telnet ; // network
			break ;	
	}
	return gbGOOD;
}
		
GOOD_OR_BAD ARG_Device(const char *arg)
{
	struct stat sbuf;
	if (stat(arg, &sbuf)) {
		switch( ArgType(arg) ) {
			case arg_addr_number: // port
			case arg_addr_colon:
			case arg_addr_ip:
			case arg_addr_ftdi:
			case arg_addr_other:
				return ARG_Serial(arg) ;
			default:
				LEVEL_DEFAULT("Cannot access device %s", arg);
				return gbBAD;
		}
	}
	if (!S_ISCHR(sbuf.st_mode)) {
		LEVEL_DEFAULT("Not a \"character\" device %s (st_mode=%x)", arg, sbuf.st_mode);
		return gbBAD;
	}
	if (major(sbuf.st_rdev) == 99) {
		return ARG_Parallel(arg);
	}
	if (major(sbuf.st_rdev) == 89) {
		return ARG_I2C(arg);
	}
	return ARG_Serial(arg);
}

GOOD_OR_BAD ARG_PBM(const char *arg)
{
	struct port_in * pin = NewPort( NULL ) ;
	struct connection_in * in ;
	if ( pin == NULL ) {
		return gbBAD;
	}
	in = pin->first ;
	if (in == NO_CONNECTION) {
		return gbBAD;
	}
	arg_data(arg,pin) ;
	pin->busmode = bus_pbm ; // elabnet
	return Serial_or_telnet( arg, in ) ;
}

GOOD_OR_BAD ARG_EtherWeather(const char *arg)
{
	struct port_in * pin = NewPort( NULL ) ;
	struct connection_in * in ;
	if ( pin == NULL ) {
		return gbBAD;
	}
	in = pin->first ;
	if (in == NO_CONNECTION) {
		return gbBAD;
	}
	arg_data( arg, pin ) ;
	pin->busmode = bus_etherweather;
	return gbGOOD;
}

GOOD_OR_BAD ARG_External(const char *arg)
{
	(void) arg ;
	if ( Inbound_Control.external == NULL ) {
		struct port_in * pin = NewPort( NULL ) ;
		struct connection_in * in ;
		if ( pin == NULL ) {
			return gbBAD;
		}
		in = pin->first ;
		if (in == NO_CONNECTION) {
			return gbBAD;
		}
		arg_data("external",pin) ;
		pin->busmode = bus_external;
		Inbound_Control.external = in ;
	}
	return gbGOOD;
}

GOOD_OR_BAD ARG_Fake(const char *arg)
{
	struct port_in * pin = NewPort( NULL ) ;
	struct connection_in * in ;
	if ( pin == NULL ) {
		return gbBAD;
	}
	in = pin->first ;
	if (in == NO_CONNECTION) {
		return gbBAD;
	}
	arg_data(arg,pin) ;
	pin->busmode = bus_fake;
	return gbGOOD;
}

GOOD_OR_BAD ARG_Generic(const char *arg)
{
	if (arg && arg[0]) {
		switch (arg[0]) {
			case '/':
				return ARG_Device(arg);
			case 'u':
			case 'U':
				return ARG_USB(&arg[1]);
			default:
				return ARG_Net(arg);
		}
	}
	return gbBAD;
}

GOOD_OR_BAD ARG_HA5( const char *arg)
{
	struct port_in * pin = NewPort( NULL ) ;
	struct connection_in * in ;
	if ( pin == NULL ) {
		return gbBAD;
	}
	in = pin->first ;
	if (in == NO_CONNECTION) {
		return gbBAD;
	}
	if ( arg == NULL ) {
		return gbBAD ;
	}
	arg_data(arg,pin) ;
	pin->busmode = bus_ha5;
	return Serial_or_telnet( arg, in ) ;
}

GOOD_OR_BAD ARG_HA7(const char *arg)
{
	if (arg != NULL) {
		struct port_in * pin = NewPort( NULL ) ;
		struct connection_in * in ;
		if ( pin == NULL ) {
			return gbBAD;
		}
		in = pin->first ;
		if (in == NO_CONNECTION) {
			return gbBAD;
		}
		arg_data(arg,pin) ;
		pin->busmode = bus_ha7net;
		return gbGOOD;
	} else {					// Try multicast discovery
		//printf("Find HA7\n");
		return FS_FindHA7();
	}
}

GOOD_OR_BAD ARG_HA7E(const char *arg)
{
	struct port_in * pin = NewPort( NULL ) ;
	struct connection_in * in ;
	if ( pin == NULL ) {
		return gbBAD;
	}
	in = pin->first ;
	if (in == NO_CONNECTION) {
		return gbBAD;
	}
	arg_data(arg,pin) ;
	pin->busmode = bus_ha7e ;
	return Serial_or_telnet( arg, in ) ;
}

GOOD_OR_BAD ARG_DS1WM(const char *arg)
{
	struct port_in * pin = NewPort( NULL ) ;
	struct connection_in * in ;
	if ( pin == NULL ) {
		return gbBAD;
	}
	in = pin->first ;
	if (in == NO_CONNECTION) {
		return gbBAD;
	}
	arg_data(arg,pin) ;
	pin->busmode = bus_ds1wm ;
	return gbGOOD ;
}

GOOD_OR_BAD ARG_K1WM(const char *arg)
{
	struct port_in * pin = NewPort( NULL ) ;
	struct connection_in * in ;
	if ( pin == NULL ) {
		return gbBAD;
	}
	in = pin->first ;
	if (in == NO_CONNECTION) {
		return gbBAD;
	}
	arg_data(arg,pin) ;
	pin->busmode = bus_k1wm ;
	return gbGOOD ;
}

GOOD_OR_BAD ARG_ENET(const char *arg)
{
	struct port_in * pin = NewPort( NULL ) ;
	struct connection_in * in ;
	if ( pin == NULL ) {
		return gbBAD;
	}
	in = pin->first ;
	if (in == NO_CONNECTION) {
		return gbBAD;
	}
	arg_data(arg,pin) ;
	pin->busmode = bus_enet ;
	return gbGOOD;
}

GOOD_OR_BAD ARG_I2C(const char *arg)
{
#if OW_I2C
	struct port_in * pin = NewPort( NULL ) ;
	struct connection_in * in ;
	if ( pin == NULL ) {
		return gbBAD;
	}
	in = pin->first ;
	if (in == NO_CONNECTION) {
		return gbBAD;
	}
	arg_data( (arg!=NULL) ? arg : ":" , pin ) ;
	pin->busmode = bus_i2c;
	return gbGOOD;
#else							/* OW_I2C */
	LEVEL_DEFAULT("I2C (smbus DS2482-X00) support (intentionally) not included in compilation. Reconfigure and recompile.");
	return gbBAD;
#endif							/* OW_I2C */
}

GOOD_OR_BAD ARG_Link(const char *arg)
{
	struct port_in * pin = NewPort( NULL ) ;
	struct connection_in * in ;
	if ( pin == NULL ) {
		return gbBAD;
	}
	in = pin->first ;
	if (in == NO_CONNECTION) {
		return gbBAD;
	}
	arg_data(arg,pin) ;
	pin->busmode = bus_link ; // link
	return Serial_or_telnet( arg, in ) ;
}

GOOD_OR_BAD ARG_MasterHub(const char *arg)
{
	struct port_in * pin = NewPort( NULL ) ;
	struct connection_in * in ;
	if ( pin == NULL ) {
		return gbBAD;
	}
	in = pin->first ;
	if (in == NO_CONNECTION) {
		return gbBAD;
	}
	arg_data(arg,pin) ;
	pin->busmode = bus_masterhub ; // link
	return Serial_or_telnet( arg, in ) ;
}

GOOD_OR_BAD ARG_Mock(const char *arg)
{
	struct port_in * pin = NewPort( NULL ) ;
	struct connection_in * in ;
	if ( pin == NULL ) {
		return gbBAD;
	}
	in = pin->first ;
	if (in == NO_CONNECTION) {
		return gbBAD;
	}
	arg_data(arg,pin) ;
	pin->busmode = bus_mock;
	return gbGOOD;
}

GOOD_OR_BAD ARG_W1_monitor(void)
{
	struct port_in * pin = NewPort( NULL ) ;
	struct connection_in * in ;
	if ( pin == NULL ) {
		return gbBAD;
	}
	in = pin->first ;
	if (in == NO_CONNECTION) {
		return gbBAD;
	}
	arg_data("W1 bus monitor",pin) ;
	pin->busmode = bus_w1_monitor;
	return gbGOOD;
}

GOOD_OR_BAD ARG_Browse(void)
{
	struct port_in * pin = NewPort( NULL ) ;
	struct connection_in * in ;
	if ( pin == NULL ) {
		return gbBAD;
	}
	in = pin->first ;
	if (in == NO_CONNECTION) {
		return gbBAD;
	}
	arg_data("ZeroConf monitor",pin) ;
	pin->busmode = bus_browse;
	return gbGOOD;
}

// This is to connect to owserver as a (remote) bus
GOOD_OR_BAD ARG_Net(const char *arg)
{
	struct port_in * pin = NewPort( NULL ) ;
	struct connection_in * in ;
	if ( pin == NULL ) {
		return gbBAD;
	}
	in = pin->first ;
	if (in == NO_CONNECTION) {
		return gbBAD;
	}
	arg_data(arg,pin) ;
	pin->busmode = bus_server;
	return gbGOOD;
}

GOOD_OR_BAD ARG_Parallel(const char *arg)
{
	(void) arg ;
#if OW_PARPORT
	struct port_in * pin = NewPort( NULL ) ;
	struct connection_in * in ;
	if ( pin == NULL ) {
		return gbBAD;
	}
	in = pin->first ;
	if (in == NO_CONNECTION) {
		return gbBAD;
	}
	arg_data(arg,pin) ;
	pin->busmode = bus_parallel;
	return gbGOOD;
#else							/* OW_PARPORT */
	LEVEL_DEFAULT("Parallel port support (intentionally) not included in compilation. For DS1410E. That's ok, it doesn't work anyways.");
	return gbBAD;
#endif							/* OW_PARPORT */
}

GOOD_OR_BAD ARG_Passive(char *adapter_type_name, const char *arg)
{
	struct port_in * pin = NewPort( NULL ) ;
	struct connection_in * in ;
	if ( pin == NULL ) {
		return gbBAD;
	}
	in = pin->first ;
	if (in == NO_CONNECTION) {
		return gbBAD;
	}
	arg_data(arg,pin) ;
	// special set name of adapter here
	in->adapter_name = adapter_type_name;
	pin->busmode = bus_passive ; // DS9097
	return Serial_or_telnet( arg, in ) ;
}

GOOD_OR_BAD ARG_Serial(const char *arg)
{
	struct port_in * pin = NewPort( NULL ) ;
	struct connection_in * in ;
	if ( pin == NULL ) {
		return gbBAD;
	}
	in = pin->first ;
	if (in == NO_CONNECTION) {
		return gbBAD;
	}
	arg_data(arg,pin) ;
	pin->busmode = bus_serial ; // DS2480B
	return Serial_or_telnet( arg, in ) ;
}

// This is owserver's listening port
// and owfs's mountpoint
GOOD_OR_BAD ARG_Server(const char *arg)
{
	switch (Globals.daemon_status) {
		case e_daemon_sd:
		case e_daemon_sd_done:
			LEVEL_DEBUG("Systemd mode: Ignore %s",arg);
			break ;
		default:
		{
			struct connection_out *out = NewOut();
			if (out == NULL) {
				return gbBAD;
			}
			out->name = (arg!=NULL) ? owstrdup(arg) : NULL;
		}
			break ;
	}		
	return gbGOOD;
}

GOOD_OR_BAD ARG_Tester(const char *arg)
{
	struct port_in * pin = NewPort( NULL ) ;
	struct connection_in * in ;
	if ( pin == NULL ) {
		return gbBAD;
	}
	in = pin->first ;
	if (in == NO_CONNECTION) {
		return gbBAD;
	}
	arg_data(arg,pin) ;
	pin->busmode = bus_tester;
	return gbGOOD;
}

// USB is a little more involved -- have to handle the "all" case and the specific number case
GOOD_OR_BAD ARG_USB(const char *arg)
{
#if OW_USB
	if ( Globals.luc != NULL ) {
		struct port_in * pin = NewPort( NULL ) ;
		struct connection_in * in ;
		if ( pin == NULL ) {
			return gbBAD;
		}
		in = pin->first ;
		if (in == NO_CONNECTION) {
			return gbBAD;
		}
		pin->busmode = bus_usb;
		DS9490_port_setup( NULL, pin ) ; // flag as not yet set up
		arg_data(arg,pin) ;
		return gbGOOD;
	} else {
		LEVEL_DEFAULT( "USB library initialization had problems -- can't proceed") ;
		return gbBAD ;
	}
#else							/* OW_USB */
	(void) arg ;
	LEVEL_DEFAULT("USB support (intentionally) not included in compilation. Check LIBUSB, then reconfigure and recompile.");
	return gbBAD;
#endif							/* OW_USB */
}

// Xport or telnet -- DS2480B over a remote serial server using telnet protocol.
GOOD_OR_BAD ARG_Xport(const char *arg)
{
	struct port_in * pin = NewPort( NULL ) ;
	struct connection_in * in ;
	if ( pin == NULL ) {
		return gbBAD;
	}
	in = pin->first ;
	if (in == NO_CONNECTION) {
		return gbBAD;
	}
	arg_data(arg,pin) ;
	pin->busmode = bus_xport;
	pin->type = ct_telnet ; // network
	return gbGOOD;
}

