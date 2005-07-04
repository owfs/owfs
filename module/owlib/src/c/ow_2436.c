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

/* General Device File format:
    This device file corresponds to a specific 1wire/iButton chip type
    ( or a closely related family of chips )

    The connection to the larger program is through the "device" data structure,
      which must be declared in the acompanying header file.

    The device structure holds the
      family code,
      name,
      device type (chip, interface or pseudo)
      number of properties,
      list of property structures, called "filetype".

    Each filetype structure holds the
      name,
      estimated length (in bytes),
      aggregate structure pointer,
      data format,
      read function,
      write funtion,
      generic data pointer

    The aggregate structure, is present for properties that several members
    (e.g. pages of memory or entries in a temperature log. It holds:
      number of elements
      whether the members are lettered or numbered
      whether the elements are stored together and split, or separately and joined
*/

#include "owfs_config.h"
#include "ow_2436.h"

/* ------- Prototypes ----------- */

/* DS2436 Battery */
 bREAD_FUNCTION( FS_r_page ) ;
bWRITE_FUNCTION( FS_w_page ) ;
 fREAD_FUNCTION( FS_temp ) ;
 fREAD_FUNCTION( FS_volts ) ;

/* ------- Structures ----------- */

struct aggregate A2436 = { 5, ag_numbers, ag_separate,} ;
struct filetype DS2436[] = {
    F_STANDARD   ,
    {"pages"     ,     0,  NULL,   ft_subdir     , ft_volatile, {v:NULL}       , {v:NULL}       , NULL, } ,
    {"pages/page",    32,  &A2436, ft_binary     , ft_stable  , {b:FS_r_page}   , {b:FS_w_page}, NULL, } ,
    {"volts"     ,    12,  NULL  , ft_float      , ft_volatile, {f:FS_volts}    , {v:NULL},      NULL, } ,
    {"temperature",    12,  NULL , ft_temperature, ft_volatile, {f:FS_temp}     , {v:NULL},      NULL, } ,
} ;
DeviceEntry( 1B, DS2436 ) ;

/* ------- Functions ------------ */

/* DS2436 */
static int OW_r_page( unsigned char * p , const size_t size, const size_t offset , const struct parsedname * pn) ;
static int OW_w_page( const unsigned char * p , const size_t size , const size_t offset , const struct parsedname * pn ) ;
static int OW_temp( FLOAT * T , const struct parsedname * pn ) ;
static int OW_volts( FLOAT * V , const struct parsedname * pn ) ;

/* 2436 A/D */
static int FS_r_page(unsigned char *buf, const size_t size, const off_t offset , const struct parsedname * pn) {
    if ( pn->extension > 2 ) return -ERANGE ;
    if ( OW_r_page( buf, size, offset+((pn->extension)<<5), pn) ) return -EINVAL ;
    return size ;
}

static int FS_w_page(const unsigned char *buf, const size_t size, const off_t offset , const struct parsedname * pn) {
    if ( pn->extension > 2 ) return -ERANGE ;
    if ( OW_w_page(buf,size,offset+((pn->extension)<<5),pn) ) return -EINVAL ;
    return 0 ;
}

static int FS_temp(FLOAT * T , const struct parsedname * pn) {
    if ( OW_temp( T , pn ) ) return -EINVAL ;
    return 0 ;
}

static int FS_volts(FLOAT * V , const struct parsedname * pn) {
    if ( OW_volts( V , pn ) ) return -EINVAL ;
    return 0 ;
}

/* DS2436 simple battery */
/* only called for a single page, and that page is 0,1,2 only*/
static int OW_r_page( unsigned char * p , const size_t size , const size_t offset, const struct parsedname * pn) {
    unsigned char data[33] ;
    static unsigned char copyin[] = {0x71, 0x77, 0x7A, } ;
    unsigned char scratchpad[] = { copyin[offset>>5], 0x11 , offset, } ;
    size_t rest = 32-(offset&0x1F) ;
    int ret ;

    // read to scratch, then in
    BUSLOCK(pn);
        ret = BUS_select(pn) || BUS_send_data( scratchpad, 3,pn ) || BUS_readin_data( data,rest+1,pn ) || CRC8( data,rest+1) ;
    BUSUNLOCK(pn);
    if ( ret ) return 1 ;

    // copy to buffer
    memcpy( p , data , size ) ;
    return 0 ;
}

/* only called for a single page, and that page is 0,1,2 only*/
static int OW_w_page( const unsigned char * p , const size_t size , const size_t offset , const struct parsedname * pn ) {
    size_t rest = 32-(offset&0x1F) ;
    unsigned char scratchin[] = {0x11 , offset, } ;
    unsigned char scratchout[] = {0x17 , offset, } ;
    unsigned char data[33] ;
    static unsigned char copyin[] = {0x71, 0x77, 0x7A, } ;
    static unsigned char copyout[] = {0x22, 0x25, 0x27, } ;
    int ret ;

    // read scratchpad (sets it, too)
    if ( size != rest ) { /* partial page write (doesn't go to end of page) */
        if ( OW_r_page(&data[size],offset+size,rest-size,pn) ) return 1 ;
    } else { /* just copy in NVram to make sure scratchpad is set */
        BUSLOCK(pn);
            ret = BUS_select(pn) || BUS_send_data( &copyin[offset>>5], 1,pn ) ;
        BUSUNLOCK(pn);
        if ( ret ) return 1 ;
    }

    memcpy( data , p , size ) ;

    // write to scratchpad
    BUSLOCK(pn);
        ret = BUS_select(pn) || BUS_send_data( scratchout, 2,pn ) || BUS_send_data( data,rest,pn ) ;
    BUSUNLOCK(pn);
    if ( ret ) return 1 ;

    // re-read scratchpad and compare
    BUSLOCK(pn);
        ret = BUS_select(pn) || BUS_send_data( scratchin, 2,pn ) || BUS_readin_data( data,rest+1,pn ) || CRC8( data,rest+1 ) || memcmp(data,p,size) ;
    BUSUNLOCK(pn);
    if ( ret ) return 1 ;

    // send scratchpad to perment memory
    BUSLOCK(pn);
        ret = BUS_send_data( &copyout[offset>>5],1,pn) ;
    BUSUNLOCK(pn);
    if ( ret ) return 1 ;

    // Could recheck with a read, but no point
    return 0 ;
}

static int OW_temp( FLOAT * T , const struct parsedname * pn ) {
    unsigned char d2 = 0xD2 ;
    unsigned char b2[] = { 0xB2 , 0x60, } ;
    unsigned char t[3] ;
    int ret ;

    // initiate conversion
    BUSLOCK(pn);
        ret = BUS_select(pn) || BUS_send_data( &d2, 1,pn ) ;
    BUSUNLOCK(pn);
    if ( ret ) return 1 ;

    UT_delay(6) ;

    /* Is it done? */
    BUSLOCK(pn);
        ret = BUS_send_data( b2, 2,pn ) || BUS_readin_data( t, 3,pn ) ;
    BUSUNLOCK(pn);
    if ( ret ) return 1 ;

    /* Is it done, now? */
    if ( t[2] & 0x01 ) {
        UT_delay(5) ;
        BUSLOCK(pn);
            ret = BUS_send_data( b2, 2,pn ) || BUS_readin_data( t , 3,pn ) || (t[2]&0x01) ;
        BUSUNLOCK(pn);
        if ( ret ) return 1 ;
    }
    *T = ((int)((signed char)t[1])) + .00390625*t[0] ;
    return 0 ;
}

static int OW_volts( FLOAT * V , const struct parsedname * pn ) {
    unsigned char b4 = 0xB4 ;
    unsigned char status[] = { 0xB2 , 0x62, } ;
    unsigned char volts[] = { 0xB2 , 0x77, } ;
    unsigned char s,v[2] ;
    int ret ;

    // initiate conversion
    BUSLOCK(pn);
        ret = BUS_select(pn) || BUS_send_data( &b4 , 1,pn ) ;
    BUSUNLOCK(pn);
    if ( ret ) return 1 ;

    UT_delay(10) ;

    /* Is it done? */
    BUSLOCK(pn);
        ret = BUS_send_data( status, 2,pn ) || BUS_readin_data( &s , 1,pn ) ;
    BUSUNLOCK(pn);
    if ( ret ) return 1 ;

    /* Is it done, now? */
    if ( s & 0x08 ) {
        UT_delay(10) ;
        BUSLOCK(pn);
            ret = BUS_send_data( status, 2,pn ) || BUS_readin_data( &s , 1,pn ) || (s&0x08) ;
        BUSUNLOCK(pn);
        if ( ret ) return 1 ;
    }

    /* Read it in */
    BUSLOCK(pn);
        ret = BUS_send_data( volts , 2,pn ) || BUS_readin_data( v , 2,pn ) ;
    BUSUNLOCK(pn);
    if ( ret ) return 1 ;

    *V = .01 * (FLOAT)( ( ((int)v[1]) <<8 )|v[0] ) ;
    return 0 ;
}
