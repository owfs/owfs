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

int ARG_Net(const char *arg)
{
	struct connection_in *in = NewIn(NULL);
	if (in == NULL) {
		return 1;
	}
	in->name = owstrdup(arg);
	in->busmode = bus_server;
	return 0;
}

int ARG_HA5( const char *arg)
{
	struct connection_in *in = NewIn(NULL);
	if (in == NULL) {
		return 1;
	}
	in->name = owstrdup(arg);
	in->busmode = bus_ha5;
	return 0;
}


int ARG_HA7(const char *arg)
{
#if OW_HA7
	if (arg) {
		struct connection_in *in = NewIn(NULL);
		if (in == NULL) {
			return 1;
		}
		in->name = arg ? owstrdup(arg) : NULL;
		in->busmode = bus_ha7net;
		return 0;
	} else {					// Try multicast discovery
		//printf("Find HA7\n");
		return FS_FindHA7();
	}
#else							/* OW_HA7 */
	LEVEL_DEFAULT("HA7 support (intentionally) not included in compilation. Reconfigure and recompile.\n");
	return 1;
#endif							/* OW_HA7 */
}

int ARG_EtherWeather(const char *arg)
{
	struct connection_in *in = NewIn(NULL);
	if (in == NULL) {
		return 1;
	}
	in->name = arg ? owstrdup(arg) : NULL;
	in->busmode = bus_etherweather;
	return 0;
}

int ARG_Xport(const char *arg)
{
	struct connection_in *in = NewIn(NULL);
	if (in == NULL) {
		return 1;
	}
	in->name = arg ? owstrdup(arg) : NULL;
	in->busmode = bus_xport;
	return 0;
}

int ARG_Fake(const char *arg)
{
	struct connection_in *in = NewIn(NULL);
	if (in == NULL) {
		return 1;
	}
	in->name = owstrdup(arg);
	in->busmode = bus_fake;
	return 0;
}

int ARG_Mock(const char *arg)
{
	struct connection_in *in = NewIn(NULL);
	if (in == NULL) {
		return 1;
	}
	in->name = owstrdup(arg);
	in->busmode = bus_mock;
	return 0;
}

int ARG_Tester(const char *arg)
{
	struct connection_in *in = NewIn(NULL);
	if (in == NULL) {
		return 1;
	}
	in->name = owstrdup(arg);
	in->busmode = bus_tester;
	return 0;
}

int ARG_Server(const char *arg)
{
	struct connection_out *out = NewOut();
	if (out == NULL) {
		return 1;
	}
	out->name = (arg != NULL) ? owstrdup(arg) : NULL;
	return 0;
}

int ARG_Side(const char *arg)
{
	struct connection_side *side = NewSide();
	if (side == NULL) {
		return 1;
	}
	side->name = arg ? owstrdup(arg) : NULL;
	return 0;
}

int ARG_Device(const char *arg)
{
	struct stat sbuf;
	if (stat(arg, &sbuf)) {
		LEVEL_DEFAULT("Cannot access device %s\n", arg);
		return 1;
	}
	if (!S_ISCHR(sbuf.st_mode)) {
		LEVEL_DEFAULT("Not a \"character\" device %s (st_mode=%x)\n", arg, sbuf.st_mode);
		return 1;
	}
	if (major(sbuf.st_rdev) == 99) {
		return ARG_Parallel(arg);
	}
	if (major(sbuf.st_rdev) == 89) {
		return ARG_I2C(arg);
	}
	return ARG_Serial(arg);
}

int ARG_Serial(const char *arg)
{
	struct connection_in *in = NewIn(NULL);
	if (in == NULL) {
		return 1;
	}
	in->name = owstrdup(arg);
	in->busmode = bus_serial;
	return 0;
}

int ARG_Passive(char *adapter_type_name, const char *arg)
{
	struct connection_in *in = NewIn(NULL);
	if (in == NULL) {
		return 1;
	}
	in->name = owstrdup(arg);
	in->busmode = bus_passive;
	// special set name of adapter here
	in->adapter_name = adapter_type_name;
	return 0;
}

int ARG_Link(const char *arg)
{
    struct connection_in *in = NewIn(NULL);
    if (in == NULL) {
        return 1;
    }
    in->name = owstrdup(arg);
    in->busmode = (arg[0] == '/') ? bus_link : bus_elink;
    return 0;
}

int ARG_HA7E(const char *arg)
{
    struct connection_in *in = NewIn(NULL);
    if (in == NULL) {
        return 1;
    }
    in->name = owstrdup(arg);
    in->busmode = bus_ha7e ;
    return 0;
}

int ARG_Parallel(const char *arg)
{
#if OW_PARPORT
	struct connection_in *in = NewIn(NULL);
	if (in == NULL) {
		return 1;
	}
	in->name = owstrdup(arg);
	in->busmode = bus_parallel;
	return 0;
#else							/* OW_PARPORT */
	LEVEL_DEFAULT("Parallel port support (intentionally) not included in compilation. For DS1410E. That's ok, it doesn't work anyways.\n");
	return 1;
#endif							/* OW_PARPORT */
}

int ARG_I2C(const char *arg)
{
#if OW_I2C
	struct connection_in *in = NewIn(NULL);
	if (in == NULL) {
		return 1;
	}
	if ( arg==NULL) {
		in->name = owstrdup(":") ;
	} else {
		in->name = owstrdup(arg);
    	}
	in->busmode = bus_i2c;
	return 0;
#else							/* OW_I2C */
	LEVEL_DEFAULT("I2C (smbus DS2482-X00) support (intentionally) not included in compilation. Reconfigure and recompile.\n");
	return 1;
#endif							/* OW_I2C */
}

int ARG_USB(const char *arg)
{
#if OW_USB
	struct connection_in *in = NewIn(NULL);
	if (in == NULL) {
		return 1;
	}
	in->busmode = bus_usb;
	if (arg == NULL) {
		in->connin.usb.usb_nr = 1;
	} else if (strcasecmp(arg, "all") == 0) {
		int number_of_usb_adapters;
		number_of_usb_adapters = DS9490_enumerate();
		LEVEL_CONNECT("All USB adapters requested, %d found.\n", number_of_usb_adapters);
		// first one
		in->connin.usb.usb_nr = 1;
		// cycle through rest
		if (number_of_usb_adapters > 1) {
			int usb_adapter_index;
			for (usb_adapter_index = 2; usb_adapter_index <= number_of_usb_adapters; ++usb_adapter_index) {
				struct connection_in *in2 = NewIn(NULL);
				if (in2 == NULL) {
					return 1;
				}
				in2->busmode = bus_usb;
				in2->connin.usb.usb_nr = usb_adapter_index;
			}
		}
	} else {
		in->connin.usb.usb_nr = atoi(arg);
		//printf("ArgUSB file_descriptor=%d\n",in->file_descriptor);
		if (in->connin.usb.usb_nr < 1) {
			LEVEL_CONNECT("USB option %s implies no USB detection.\n", arg);
			in->connin.usb.usb_nr = 0;
		} else if (in->connin.usb.usb_nr > 1) {
			LEVEL_CONNECT("USB adapter %d requested.\n", in->connin.usb.usb_nr);
		}
	}
	return 0;
#else							/* OW_USB */
	LEVEL_DEFAULT("USB support (intentionally) not included in compilation. Check LIBUSB, then reconfigure and recompile.\n");
	return 1;
#endif							/* OW_USB */
}

int ARG_Generic(const char *arg)
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
	return 1;
}
