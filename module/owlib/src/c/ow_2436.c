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
    {"pages"     ,     0,  NULL,   ft_subdir, ft_volatile, {v:NULL}       , {v:NULL}       , NULL, } ,
    {"pages/page",    32,  &A2436, ft_binary, ft_stable  , {b:FS_r_page}   , {b:FS_w_page}, NULL, } ,
    {"volts"     ,    12,  NULL  , ft_float , ft_volatile, {f:FS_volts}    , {v:NULL},      NULL, } ,
    {"temperature",    12,  NULL , ft_float , ft_volatile, {f:FS_temp}     , {v:NULL},      NULL, } ,
} ;
DeviceEntry( 1B, DS2436 )

/* ------- Functions ------------ */

/* DS2436 */
static int OW_r_page( unsigned char * p , const size_t location , const size_t length, const struct parsedname * pn) ;
static int OW_w_page( const unsigned char * p , const size_t location , const size_t size , const struct parsedname * pn ) ;
static int OW_temp( FLOAT * T , const struct parsedname * pn ) ;
static int OW_volts( FLOAT * V , const struct parsedname * pn ) ;

/* 2436 A/D */
static int FS_r_page(unsigned char *buf, const size_t size, const off_t offset , const struct parsedname * pn) {
    size_t len = size ;
    if ( (offset&0x1F)+size>32 ) len = 32-offset ;
    if ( OW_r_page( buf, offset+((pn->extension)<<5), len, pn) ) return -EINVAL ;

    return len ;
}

static int FS_w_page(const unsigned char *buf, const size_t size, const off_t offset , const struct parsedname * pn) {
    return OW_w_page(buf,size,offset+((pn->extension)<<5),pn) ? -EINVAL : 0 ;
}

static int FS_temp(FLOAT * T , const struct parsedname * pn) {
    if ( OW_temp( T , pn ) ) return -EINVAL ;
    *T = Temperature( *T ) ;
    return 0 ;
}

static int FS_volts(FLOAT * V , const struct parsedname * pn) {
    if ( OW_volts( V , pn ) ) return -EINVAL ;
    return 0 ;
}

/* DS2436 simple battery */
static int OW_r_page( unsigned char * p , const size_t location , const size_t length, const struct parsedname * pn) {
    unsigned char data[33] ;
    static unsigned char copyin[] = {0x71, 0x77, 0x7A, } ;
    unsigned char scratchpad[] = {0x11 , location, } ;
    size_t offset = location&0x1F ;
    size_t size = 33-offset ;
    int ret ;

    // past page 2
    if ( (location>>5) > 2 ) return -EFAULT ;

    // read to scratch, then in
    BUS_lock() ;
        ret = BUS_select(pn) || BUS_send_data( &copyin[location>>5] , 1 ) || BUS_send_data( scratchpad , 2 ) || BUS_readin_data( &data[offset],size ) || CRC8( &data[offset],size) ;
    BUS_unlock() ;
    if ( ret ) return 1 ;

    // copy to buffer
    memcpy( &p[offset] , &data[offset] , (size>length)?length:size-1 ) ;
    return 0 ;
}

static int OW_w_page( const unsigned char * p , const size_t location , const size_t size , const struct parsedname * pn ) {
    int offset = location&0x1F ;
    size_t pagestart = location - offset ;
    size_t rest = 32-offset ;
    unsigned char scratchin[] = {0x11 , location, } ;
    unsigned char scratchout[] = {0x17 , location, } ;
    unsigned char data[33] ;
    static unsigned char copyout[] = {0x22, 0x25, 0x27, } ;
    int ret ;

    // past page 2
    if ( (location>>5) > 2 ) return -ERANGE ; 

    // split across pages
    if ( size > rest ) return OW_w_page( p,location,rest,pn) || OW_w_page(&p[rest],location+rest,size-rest,pn) ;

    // read scratchpad (sets it, too)
    if ( OW_r_page(data,pagestart,32,pn) ) return 1 ;
    memcpy( &data[offset] , p , size ) ;

    // write to scratchpad 
    BUS_lock() ;
        ret = BUS_select(pn) || BUS_send_data( scratchout , 2 ) || BUS_send_data( &data[offset],size ) ;
    BUS_unlock() ;
    if ( ret ) return 1 ;

    // re-read scratchpad and compare
    BUS_lock() ;
        ret = BUS_select(pn) || BUS_send_data( scratchin , 2 ) || BUS_readin_data( &data[offset],rest+1 ) || CRC8( &data[offset],rest+1 ) || memcmp(&data[offset],p,size) ;
    BUS_unlock() ;
    if ( ret ) return 1 ;

    // send scratchpad to perment memory
    BUS_lock() ;
        ret = BUS_send_data(&copyout[location>>5],1) ;
    BUS_unlock() ;
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
    BUS_lock() ;
        ret = BUS_select(pn) || BUS_send_data( &d2 , 1 ) ;
    BUS_unlock() ;
    if ( ret ) return 1 ;

    UT_delay(6) ;

    /* Is it done? */
    BUS_lock() ;
        ret = BUS_send_data( b2, 2 ) || BUS_readin_data(t , 3 ) ;
    BUS_unlock() ;
    if ( ret ) return 1 ;

    /* Is it done, now? */
    if ( t[2] & 0x01 ) {
        UT_delay(5) ;
        BUS_lock() ;
            ret = BUS_send_data( b2, 2 ) || BUS_readin_data(t , 3 ) || (t[2]&0x01) ;
        BUS_unlock() ;
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
    BUS_lock() ;
        ret = BUS_select(pn) || BUS_send_data( &b4 , 1 ) ;
    BUS_unlock() ;
    if ( ret ) return 1 ;

    UT_delay(10) ;

    /* Is it done? */
    BUS_lock() ;
        ret = BUS_send_data( status, 2 ) || BUS_readin_data(&s , 1 ) ;
    BUS_unlock() ;
    if ( ret ) return 1 ;

    /* Is it done, now? */
    if ( s & 0x08 ) {
        UT_delay(10) ;
        BUS_lock() ;
            ret = BUS_send_data( status, 2 ) || BUS_readin_data(&s , 1 ) || (s&0x08) ;
        BUS_unlock() ;
        if ( ret ) return 1 ;
    }

    /* Read it in */
    BUS_lock() ;
        ret = BUS_send_data( volts , 2 ) || BUS_readin_data( v , 2 ) ;
    BUS_unlock() ;
    if ( ret ) return 1 ;

    *V = .01 * (FLOAT)( ( ((int)v[1]) <<8 )|v[0] ) ;
    return 0 ;
}


