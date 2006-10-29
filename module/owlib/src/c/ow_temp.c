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

const char *tempscale[4] = {
  "Celsius",
  "Fahrenheit",
  "Kelvin",
  "Rankine",
};

const char *TemperatureScaleName(enum temp_type t) {
  return tempscale[t];
}

/* Temperture Conversion routines  */

/* convert internal (Centigrade) to external format */
_FLOAT Temperature( _FLOAT C, const struct parsedname * pn) {
    switch( TemperatureScale(pn) ) {
    case temp_fahrenheit:
        return 1.8*C+32. ;
    case temp_kelvin:
        return C+273.15 ;
    case temp_rankine:
        return 1.8*C+32.+459.67 ;
    default: /* Centigrade */
        return C ;
    }
}

/* convert internal (Centigrade) to external format */
_FLOAT TemperatureGap( _FLOAT C, const struct parsedname * pn) {
    switch( TemperatureScale(pn) ) {
    case temp_fahrenheit:
    case temp_rankine:
        return 1.8*C ;
    default: /* Centigrade, Kelvin */
        return C ;
    }
}

/* convert to internal (Centigrade) from external format */
_FLOAT fromTemperature( _FLOAT T, const struct parsedname * pn) {
    switch( TemperatureScale(pn) ) {
    case temp_fahrenheit:
        return (T-32.)/1.8 ;
    case temp_kelvin:
        return T-273.15 ;
    case temp_rankine:
        return (T-32.-459.67)/1.8 ;
    default: /* Centigrade */
        return T ;
    }
}
/* convert to internal (Centigrade) from external format */
_FLOAT fromTempGap( _FLOAT T, const struct parsedname * pn) {
    switch( TemperatureScale(pn) ) {
    case temp_fahrenheit:
    case temp_rankine:
        return (T)/1.8 ;
    default: /* Centigrade, Kelvin */
        return T ;
    }
}
