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

unsigned char char2num( const char * s ) {
    if ( s == NULL ) return 0 ;
    switch (*s) {
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
        return (*s) - '0' ;
    case 'a':
    case 'b':
    case 'c':
    case 'd':
    case 'e':
    case 'f':
        return 10 + (*s) - 'a' ;
    case 'A':
    case 'B':
    case 'C':
    case 'D':
    case 'E':
    case 'F':
        return 10 + (*s) - 'A' ;
    }
    return 0 ;
}

unsigned char string2num( const char * s ) {
    if ( s == NULL ) return 0 ;
    return char2num(&s[0])<<4 | char2num(&s[1]) ;
}

char num2char( const unsigned char n ) {
    switch(n) {
    case  0: return '0' ;
    case  1: return '1' ;
    case  2: return '2' ;
    case  3: return '3' ;
    case  4: return '4' ;
    case  5: return '5' ;
    case  6: return '6' ;
    case  7: return '7' ;
    case  8: return '8' ;
    case  9: return '9' ;
    case 10: return 'A' ;
    case 11: return 'B' ;
    case 12: return 'C' ;
    case 13: return 'D' ;
    case 14: return 'E' ;
    case 15: return 'F' ;
    default: return ' ' ;
    }
}

void num2string( char * s , const unsigned char n ) {
    s[0] = num2char((unsigned char)(n>>4)) ;
    s[1] = num2char((unsigned char)(n&0xF)) ;
}

void string2bytes( const char * str , unsigned char * b , const int bytes ) {
    int i ;
    for ( i=0 ; i<bytes ; ++i ) b[i]=string2num(&str[i<<1]) ;
}

void bytes2string( char * str , const unsigned char * b , const int bytes ) {
    int i ;
    for ( i=0 ; i<bytes ; ++i ) num2string(&str[i<<1],b[i]) ;
}

// #define UT_getbit(buf, loc)	( ( (buf)[(loc)>>3]>>((loc)&0x7) ) &0x01 )
int UT_getbit(const unsigned char * buf, const int loc) {
    return ( ( (buf[loc>>3]) >> (loc&0x7) ) & 0x01 ) ;
}
int UT_get2bit(const unsigned char * buf, const int loc) {
    return ( ( (buf[loc>>2]) >> ((loc&0x3)<<1) ) & 0x03 ) ;
}
void UT_setbit( unsigned char * buf, const int loc , const int bit ) {
    if (bit) {
        buf[loc>>3] |= 0x01 << (loc&0x7) ;
    } else {
        buf[loc>>3] &= ~( 0x01 << (loc&0x7) ) ;
    }
}
void UT_set2bit( unsigned char * buf, const int loc , const int bits ) {
    unsigned char * p = &buf[loc>>2] ;
    switch (loc&3) {
    case 0 :
        *p = (*p & 0xFC) | bits ;
        return ;
    case 1 :
        *p = (*p & 0xF3) | (bits<<2) ;
        return ;
    case 2 :
        *p = (*p & 0xCF) | (bits<<4) ;
        return ;
    case 3 :
        *p = (*p & 0x3F) | (bits<<6) ;
        return ;
    }
}
