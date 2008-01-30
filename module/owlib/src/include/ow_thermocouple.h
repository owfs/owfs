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

#ifndef OW_THERMOCOUPLE_H
#define OW_THERMOCOUPLE_H

#include "ow_localtypes.h"		// for _FLOAT

enum e_thermocouple_type {
	e_type_b,
	e_type_e,
	e_type_j,
	e_type_k,
	e_type_n,
	e_type_r,
	e_type_s,
	e_type_t,
	e_type_last,
};

_FLOAT ThermocoupleTemperature(_FLOAT mV_reading, _FLOAT temperature_coldjunction, enum e_thermocouple_type etype);
_FLOAT Thermocouple_range_low(enum e_thermocouple_type etype);
_FLOAT Thermocouple_range_high(enum e_thermocouple_type etype);

#endif							/* OW_THERMOCOUPLE_H */
