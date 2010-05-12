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

/* ow_opt -- owlib specific command line options processing */

#include <config.h>
#include "owfs_config.h"
#include "ow.h"
#include "ow_connection.h"

enum arg_address { arg_addr_device, arg_addr_null, arg_addr_ip, arg_addr_colon, arg_addr_number, arg_addr_other } ;

static enum arg_address ArgType( const char * arg )
{
	if ( arg == NULL ) {
		return arg_addr_null ;
	} else if ( strchr( arg, ':' ) ) {
		return arg_addr_colon ;
	} else if ( strchr( arg, '/' ) ) {
		return arg_addr_device ;
	} else if ( strspn( arg, "0123456789" ) == strlen(arg) ) {
		return arg_addr_number ;
	} else if ( strchr(                         arg,                      '.' )
		&&  strchr( strchr(                 arg,               '.' ), '.' )
		&&  strchr( strchr( strchr(         arg,        '.' ), '.' ), '.' )
		&&  strchr( strchr( strchr( strchr( arg, '.' ), '.' ), '.' ), '.' )
		) {
		return arg_addr_ip ;
	}
	return arg_addr_other ;
}
		
GOOD_OR_BAD ARG_Device(const char *arg)
{
	struct stat sbuf;
	if (stat(arg, &sbuf)) {
		switch( ArgType(arg) ) {
			case arg_addr_number: // port
			case arg_addr_colon:
			case arg_addr_ip:			
			case arg_addr_other:
				return ARG_Xport(arg) ;
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

GOOD_OR_BAD ARG_EtherWeather(const char *arg)
{
	struct connection_in *in = NewIn(NULL);
	if (in == NULL) {
		return gbBAD;
	}
	in->name = (arg!=NULL) ? owstrdup(arg) : NULL;
	in->busmode = bus_etherweather;
	return gbGOOD;
}

GOOD_OR_BAD ARG_Fake(const char *arg)
{
	struct connection_in *in = NewIn(NULL);
	if (in == NULL) {
		return gbBAD;
	}
	in->name = (arg!=NULL) ? owstrdup(arg) : NULL;
	in->busmode = bus_fake;
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
	struct connection_in *in = NewIn(NULL);
	if (in == NULL) {
		return gbBAD;
	}
	in->name = (arg!=NULL) ? owstrdup(arg) : NULL;
	in->busmode = bus_ha5;
	return gbGOOD;
}


GOOD_OR_BAD ARG_HA7(const char *arg)
{
#if OW_HA7
	if (arg != NULL) {
		struct connection_in *in = NewIn(NULL);
		if (in == NULL) {
			return gbBAD;
		}
		in->name = owstrdup(arg);
		in->busmode = bus_ha7net;
		return gbGOOD;
	} else {					// Try multicast discovery
		//printf("Find HA7\n");
		return FS_FindHA7();
	}
#else							/* OW_HA7 */
	LEVEL_DEFAULT("HA7 support (intentionally) not included in compilation. Reconfigure and recompile.");
	return gbBAD;
#endif							/* OW_HA7 */
}

GOOD_OR_BAD ARG_HA7E(const char *arg)
{
	struct connection_in *in = NewIn(NULL);
	if (in == NULL) {
		return gbBAD;
	}
	in->name = (arg!=NULL) ? owstrdup(arg) : NULL;
	in->busmode = bus_ha7e ;
	return gbGOOD;
}

GOOD_OR_BAD ARG_ENET(const char *arg)
{
	struct connection_in *in = NewIn(NULL);
	if (in == NULL) {
		return gbBAD;
	}
	in->name = (arg!=NULL) ? owstrdup(arg) : NULL;
	in->busmode = bus_enet ;
	return gbGOOD;
}

GOOD_OR_BAD ARG_I2C(const char *arg)
{
	#if OW_I2C
	struct connection_in *in = NewIn(NULL);
	if (in == NULL) {
		return gbBAD;
	}
	in->name = (arg!=NULL) ? owstrdup(arg) : owstrdup(":");
	in->busmode = bus_i2c;
	return gbGOOD;
	#else							/* OW_I2C */
	LEVEL_DEFAULT("I2C (smbus DS2482-X00) support (intentionally) not included in compilation. Reconfigure and recompile.");
	return gbBAD;
	#endif							/* OW_I2C */
}

GOOD_OR_BAD ARG_Link(const char *arg)
{
	struct connection_in *in = NewIn(NULL);
	if (in == NULL) {
		return gbBAD;
	}
	in->name = (arg!=NULL) ? owstrdup(arg) : NULL;
	switch( ArgType(arg) ) {
		case arg_addr_null:
			LEVEL_DEFAULT("LINK error. Please include either a serial device or network address in the command line specification");
			return gbBAD ;
		case arg_addr_device:
			in->busmode = bus_link ; // serial
			break ;
		case arg_addr_number: // port
		case arg_addr_colon:
		case arg_addr_ip:
		case arg_addr_other:
			in->busmode = bus_elink ; // network
			break ;	
	}
	return gbGOOD;
}

GOOD_OR_BAD ARG_Mock(const char *arg)
{
	struct connection_in *in = NewIn(NULL);
	if (in == NULL) {
		return gbBAD;
	}
	in->name = (arg!=NULL) ? owstrdup(arg) : NULL;
	in->busmode = bus_mock;
	return gbGOOD;
}

GOOD_OR_BAD ARG_Net(const char *arg)
{
	struct connection_in *in = NewIn(NULL);
	if (in == NULL) {
		return gbBAD;
	}
	in->name = (arg!=NULL) ? owstrdup(arg) : NULL;
	in->busmode = bus_server;
	return gbGOOD;
}

GOOD_OR_BAD ARG_Parallel(const char *arg)
{
	#if OW_PARPORT
	struct connection_in *in = NewIn(NULL);
	if (in == NULL) {
		return gbBAD;
	}
	in->name = (arg!=NULL) ? owstrdup(arg) : NULL;
	in->busmode = bus_parallel;
	return gbGOOD;
	#else							/* OW_PARPORT */
	LEVEL_DEFAULT("Parallel port support (intentionally) not included in compilation. For DS1410E. That's ok, it doesn't work anyways.");
	return gbBAD;
	#endif							/* OW_PARPORT */
}

GOOD_OR_BAD ARG_Passive(char *adapter_type_name, const char *arg)
{
	struct connection_in *in = NewIn(NULL);
	if (in == NULL) {
		return gbBAD;
	}
	in->name = (arg!=NULL) ? owstrdup(arg) : NULL;
	in->busmode = bus_passive;
	// special set name of adapter here
	in->adapter_name = adapter_type_name;
	return gbGOOD;
}

GOOD_OR_BAD ARG_Serial(const char *arg)
{
	struct connection_in *in = NewIn(NULL);
	if (in == NULL) {
		return gbBAD;
	}
	in->name = (arg!=NULL) ? owstrdup(arg) : NULL;
	switch( ArgType(arg) ) {
		case arg_addr_null:
			LEVEL_DEFAULT("LINK error. Please include either a serial device or network address in the command line specification");
			return gbBAD ;
		case arg_addr_device:
			in->busmode = bus_serial;
			break ;
		case arg_addr_number: // port
		case arg_addr_colon:
		case arg_addr_ip:
		case arg_addr_other:
			in->busmode = bus_xport ; // network
			break ;
	}
	return gbGOOD;
}

GOOD_OR_BAD ARG_Server(const char *arg)
{
	struct connection_out *out = NewOut();
	if (out == NULL) {
		return gbBAD;
	}
	out->name = (arg!=NULL) ? owstrdup(arg) : NULL;
	return gbGOOD;
}

GOOD_OR_BAD ARG_Tester(const char *arg)
{
	struct connection_in *in = NewIn(NULL);
	if (in == NULL) {
		return gbBAD;
	}
	in->name = (arg!=NULL) ? owstrdup(arg) : NULL;
	in->busmode = bus_tester;
	return gbGOOD;
}

// USB is a little more involved -- have to handle the "all" case and the specific number case
GOOD_OR_BAD ARG_USB(const char *arg)
{
#if OW_USB
	struct connection_in *in = NewIn(NULL);
	if (in == NULL) {
		return gbBAD;
	}
	in->busmode = bus_usb;
	in->name = (arg!=NULL) ? owstrdup(arg) : NULL;
	return gbGOOD;
#else							/* OW_USB */
	LEVEL_DEFAULT("USB support (intentionally) not included in compilation. Check LIBUSB, then reconfigure and recompile.");
	return gbBAD;
#endif							/* OW_USB */
}

// Xport or telnet -- DS2480B over a remote serial server using telnet protocol.
GOOD_OR_BAD ARG_Xport(const char *arg)
{
	struct connection_in *in = NewIn(NULL);
	if (in == NULL) {
		return gbBAD;
	}
	in->name = (arg!=NULL) ? owstrdup(arg) : NULL;
	in->busmode = bus_xport;
	return gbGOOD;
}
