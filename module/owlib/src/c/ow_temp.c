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

#include <linux/stddef.h>

#include "owfs_config.h"
#include "ow.h"

/* Temperture Conversion routines  */
FLOAT Temperature( FLOAT C) {
    switch( tempscale ) {
    case temp_fahrenheit:
        return 1.8*C+32. ;
    case temp_kelvin:
        return C+273.15 ;
    case temp_rankine:
        return 1.8*C+32.+459.67 ;
    default:
        return C ;
    }
}

FLOAT TemperatureGap( FLOAT C) {
    switch( tempscale ) {
    case temp_fahrenheit:
    case temp_rankine:
        return 1.8*C ;
    default:
        return C ;
    }
}

FLOAT fromTemperature( FLOAT C) {
    switch( tempscale ) {
    case temp_fahrenheit:
        return (C-32.)/1.8 ;
    case temp_kelvin:
        return C-273.15 ;
    case temp_rankine:
        return 1.8*(C-32.-459.67)/1.8 ;
    default:
        return C ;
    }
}
