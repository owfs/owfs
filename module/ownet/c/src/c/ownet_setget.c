/*
    OWFS -- One-Wire filesystem
    OWHTTPD -- One-Wire Web Server
    Written 2003 Paul H Alfille
    email: paul.alfille@gmail.com
    Released under the GPL
    See the header file: ow.h for full attribution
    1wire/iButton system from Dallas Semiconductor
*/

/* ow_server talks to the server, sending and recieving messages */
/* this is an alternative to direct bus communication */

#include "ownetapi.h"
#include "ow_server.h"

void OWNET_set_temperature_scale(char temperature_scale)
{
	switch (temperature_scale) {
	case 'R':
	case 'r':
		set_semiglobal(&ow_Global.control_flags, TEMPSCALE_MASK, TEMPSCALE_BIT, temp_rankine);
		break;
	case 'K':
	case 'k':
		set_semiglobal(&ow_Global.control_flags, TEMPSCALE_MASK, TEMPSCALE_BIT, temp_kelvin);
		break;
	case 'F':
	case 'f':
		set_semiglobal(&ow_Global.control_flags, TEMPSCALE_MASK, TEMPSCALE_BIT, temp_fahrenheit);
		break;
	case 'C':
	case 'c':
	default:
		set_semiglobal(&ow_Global.control_flags, TEMPSCALE_MASK, TEMPSCALE_BIT, temp_celsius);
		break;
	}
}

char OWNET_get_temperature_scale(void)
{
	switch (TemperatureScale) {
	case temp_rankine:
		return 'R';
	case temp_kelvin:
		return 'K';
	case temp_fahrenheit:
		return 'F';
	case temp_celsius:
	default:
		return 'C';
	}
}

void OWNET_set_device_format(const char *device_format)
{
	if (device_format == NULL) {
		set_semiglobal(&ow_Global.control_flags, DEVFORMAT_MASK, DEVFORMAT_BIT, fdi);
	} else if (0 == strcasecmp(device_format, "f.i")) {
		set_semiglobal(&ow_Global.control_flags, DEVFORMAT_MASK, DEVFORMAT_BIT, fdi);
	} else if (0 == strcasecmp(device_format, "fi")) {
		set_semiglobal(&ow_Global.control_flags, DEVFORMAT_MASK, DEVFORMAT_BIT, fi);
	} else if (0 == strcasecmp(device_format, "f.i.c")) {
		set_semiglobal(&ow_Global.control_flags, DEVFORMAT_MASK, DEVFORMAT_BIT, fdidc);
	} else if (0 == strcasecmp(device_format, "f.ic")) {
		set_semiglobal(&ow_Global.control_flags, DEVFORMAT_MASK, DEVFORMAT_BIT, fdic);
	} else if (0 == strcasecmp(device_format, "fi.c")) {
		set_semiglobal(&ow_Global.control_flags, DEVFORMAT_MASK, DEVFORMAT_BIT, fidc);
	} else if (0 == strcasecmp(device_format, "fic")) {
		set_semiglobal(&ow_Global.control_flags, DEVFORMAT_MASK, DEVFORMAT_BIT, fic);
	} else {
		set_semiglobal(&ow_Global.control_flags, DEVFORMAT_MASK, DEVFORMAT_BIT, fdi);
	}
}

const char *OWNET_get_device_format(void)
{
	switch (DeviceFormat) {
	case fi:
		return "fi";
	case fic:
		return "fic";
	case fdidc:
		return "f.i.c";
	case fdic:
		return "f.ic";
	case fidc:
		return "fi.c";
	case fdi:
	default:
		return "f.i";
	}
}

void OWNET_set_trim( int trim_state )
{
	ow_Global.control_flags &= ~TRIM ; // clear trim
	ow_Global.control_flags |= trim_state ? TRIM : 0 ;
}

int OWNET_get_trim( void )
{
	return (ow_Global.control_flags & TRIM) != 0 ;
}
