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

struct aggregate A2436 = { 3, ag_numbers, ag_separate,} ;
struct filetype DS2436[] = {
    F_STANDARD   ,
    {"pages"     ,     0,  NULL,   ft_subdir     , ft_volatile, {v:NULL}       , {v:NULL}       , {v:NULL}, } ,
    {"pages/page",    32,  &A2436, ft_binary     , ft_stable  , {b:FS_r_page}   , {b:FS_w_page}, {v:NULL}, } ,
    {"volts"     ,    12,  NULL  , ft_float      , ft_volatile, {f:FS_volts}    , {v:NULL},      {v:NULL}, } ,
    {"temperature",    12,  NULL , ft_temperature, ft_volatile, {f:FS_temp}     , {v:NULL},      {v:NULL}, } ,
} ;
DeviceEntry( 1B, DS2436 ) ;

/* ------- Functions ------------ */

/* DS2436 */
static int OW_r_page( unsigned char * p , const size_t size, const size_t offset , const struct parsedname * pn) ;
static int OW_w_page( const unsigned char * p , const size_t size , const size_t offset , const struct parsedname * pn ) ;
static int OW_temp( FLOAT * T , const struct parsedname * pn ) ;
static int OW_volts( FLOAT * V , const struct parsedname * pn ) ;

static size_t Asize[] = { 24, 8, 8, } ;

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
    unsigned char data[32] ;
    int page = offset>>5 ;
    size_t s ;
    unsigned char scratchin[] = {0x11 , offset, } ;
    static unsigned char copyin[] = {0x71, 0x77, 0x7A, } ;
    int ret ;

    s = Asize[page] - (offset & 0x1F) ;
    if ( s > size ) s = size ;

    memset( p, 0xFF, size ) ;

    // send perment memory to scratchpad
    BUSLOCK(pn);
        ret = BUS_select(pn) || BUS_send_data( &copyin[page],1,pn) ;
    BUSUNLOCK(pn);
    if ( ret ) return 1 ;

    UT_delay(10) ;
    
    // re-read scratchpad and compare
    BUSLOCK(pn);
        ret = BUS_select(pn) || BUS_send_data( scratchin, 2,pn ) || BUS_readin_data( data,s,pn ) ;
    BUSUNLOCK(pn);
    if ( ret ) return 1 ;

    // copy to buffer
    memcpy( p , data , size ) ;
    
    return 0 ;
}

/* only called for a single page, and that page is 0,1,2 only*/
static int OW_w_page( const unsigned char * p , const size_t size , const size_t offset , const struct parsedname * pn ) {
    int page = offset >> 5 ;
    size_t s ;
    unsigned char scratchin[] = {0x11 , offset, } ;
    unsigned char scratchout[] = {0x17 , offset, } ;
    unsigned char data[32] ;
    static unsigned char copyout[] = {0x22, 0x25, 0x27, } ;
    int ret ;

    s = Asize[page] - (offset & 0x1F) ;
    if ( s > size ) s = size ;

    // write to scratchpad
    BUSLOCK(pn);
        ret = BUS_select(pn) || BUS_send_data( scratchout, 2,pn ) || BUS_send_data( data,s,pn ) ;
    BUSUNLOCK(pn);
    if ( ret ) return 1 ;

    // re-read scratchpad and compare
    BUSLOCK(pn);
        ret = BUS_select(pn) || BUS_send_data( scratchin, 2,pn ) || BUS_readin_data( data,s,pn ) || memcpy( data,p,s ) ;
    BUSUNLOCK(pn);
    if ( ret ) return 1 ;

    // send scratchpad to perment memory
    BUSLOCK(pn);
        ret = BUS_send_data( &copyout[page],1,pn) ;
    BUSUNLOCK(pn);
    if ( ret ) return 1 ;

    UT_delay(10) ;
    
    return 0 ;
}

static int OW_temp( FLOAT * T , const struct parsedname * pn ) {
    unsigned char d2 = 0xD2 ;
    unsigned char b2[] = { 0xB2 , 0x60, } ;
    unsigned char t[3] ;
    int timewait = 2 ;
    int ret ;

    // initiate conversion
    BUSLOCK(pn);
        ret = BUS_select(pn) || BUS_send_data( &d2, 1,pn ) ;
    BUSUNLOCK(pn);
    if ( ret ) return 1 ;

    UT_delay(timewait) ; // 3 msec minimum conversion delay

    /* Is it done? */
    while (timewait<10) {
        UT_delay(2) ;
        timewait += 2 ;
        BUSLOCK(pn);
        ret = BUS_select(pn) || BUS_send_data( b2, 2,pn ) || BUS_readin_data( t, 3,pn ) ;
        BUSUNLOCK(pn);
        if ( ret ) return 1 ;

        if ( (t[2] & 0x01)==0 ) {
            // success
            T[0] = ((int)((int8_t)t[1])) + .00390625*t[0] ;
            return 0 ;
        }
    }
    return -ETIMEDOUT ;
}

static int OW_volts( FLOAT * V , const struct parsedname * pn ) {
    unsigned char b4 = 0xB4 ;
    unsigned char status[] = { 0xB2 , 0x62, } ;
    unsigned char volts[] = { 0xB2 , 0x77, } ;
    unsigned char s,v[2] ;
    int timewait = 6 ;
    int ret ;

    // initiate conversion
    BUSLOCK(pn);
        ret = BUS_select(pn) || BUS_send_data( &b4 , 1,pn ) ;
    BUSUNLOCK(pn);
    if ( ret ) return 1 ;

    UT_delay(timewait) ;

    /* Is it done? */
    while (timewait<10) {
        UT_delay(2) ;
        timewait += 2 ;
        BUSLOCK(pn);
        ret = BUS_select(pn) || BUS_send_data( status, 2,pn ) || BUS_readin_data( &s , 1,pn ) ;
        BUSUNLOCK(pn);

        if ( (s&0x08)==0 ) {
            // success
            /* Read it in */
            BUSLOCK(pn);
                ret = BUS_select(pn) || BUS_send_data( volts , 2,pn ) || BUS_readin_data( v , 2,pn ) ;
            BUSUNLOCK(pn);
            if ( ret ) return 1 ;

            *V = .01 * (FLOAT)( ( ((uint32_t)v[1]) <<8 )|v[0] ) ;
            return 0 ;
        }
    }
    return -ETIMEDOUT ;
}
