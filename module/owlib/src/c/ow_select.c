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

#include "owfs_config.h"
#include "ow.h"

/* ------- Prototypes ----------- */

/* ------- Structures ----------- */

/* ------- Functions ------------ */

// 16 char string holds device address (full 64 bits in hex)
int OW_first( char * str ) {
    unsigned char sn[8] ;
    if ( BUS_first(sn) ) return 1 ;
    bytes2string( str , sn , 8 ) ;
    return CRC8( sn , 8 ) ;
}

int OW_next( char * str ) {
    unsigned char sn[8] ;
    string2bytes( str , sn , 8 ) ;
    if ( BUS_next(sn) ) return 1 ;
    bytes2string( str , sn , 8 ) ;
    return CRC8( sn , 8 ) ;
}

int OW_first_alarm( char * str ) {
    unsigned char sn[8] ;
    if ( BUS_first_alarm(sn) ) return 1 ;
    bytes2string( str , sn , 8 ) ;
    return CRC8( sn , 8 ) ;
}

int OW_next_alarm( char * str ) {
    unsigned char sn[8] ;
    string2bytes( str , sn , 8 ) ;
    if ( BUS_next_alarm(sn) ) return 1 ;
    bytes2string( str , sn , 8 ) ;
    return CRC8( sn , 8 ) ;
}

