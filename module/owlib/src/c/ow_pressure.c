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

struct pressure_data {
	char * name ;
	_FLOAT in_mbars ;
} PressureData[] = {
	{ "mbar", 1.0, },
	{ "atm", 0.00098692316931, },
	{ "mmHg", 0.7500615613, },
	{ "inHg", 0.029529983071, },
	{ "psi", 0.014503773801, },
	{ "Pa", 100.0, },
	{ "", 1.0, },
	{ "", 1.0, },
} ;

const char *PressureScaleName(enum pressure_type p)
{
	return PressureData[p].name;
}

/* Pressure Conversion routines  */

/* convert internal (mbar) to external format */
_FLOAT Pressure(_FLOAT P, const struct parsedname * pn)
{
	return P * PressureData[PressureScale(pn)].in_mbars ;
}

/* convert to internal (mbar) from external format */
_FLOAT fromPressure(_FLOAT P, const struct parsedname * pn)
{
	return P / PressureData[PressureScale(pn)].in_mbars ;
}

