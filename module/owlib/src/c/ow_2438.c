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
#include "ow_2438.h"

/* ------- Prototypes ----------- */

/* DS2438 Battery */
 bREAD_FUNCTION( FS_r_page ) ;
bWRITE_FUNCTION( FS_w_page ) ;
 fREAD_FUNCTION( FS_temp ) ;
 fREAD_FUNCTION( FS_volts ) ;
 fREAD_FUNCTION( FS_Humid ) ;
 iREAD_FUNCTION( FS_Current ) ;
 uREAD_FUNCTION( FS_r_Ienable ) ;
uWRITE_FUNCTION( FS_w_Ienable ) ;
 iREAD_FUNCTION( FS_r_Offset ) ;
iWRITE_FUNCTION( FS_w_Offset ) ;

/* ------- Structures ----------- */

struct aggregate A2438 = { 8, ag_numbers, ag_separate, } ;
struct filetype DS2438[] = {
    F_STANDARD   ,
    {"pages"     ,     0,  NULL,   ft_subdir, ft_volatile, {v:NULL}       , {v:NULL}       , NULL, } ,
    {"pages/page",     8,  &A2438, ft_binary , ft_stable  , {b:FS_r_page}   , {b:FS_w_page}   , NULL      , } ,
    {"VDD"       ,    12,  NULL  , ft_float  , ft_volatile, {f:FS_volts}    , {v:NULL}        , (void *) 1, } ,
    {"VAD"       ,    12,  NULL  , ft_float  , ft_volatile, {f:FS_volts}    , {v:NULL}        , (void *) 0, } ,
    {"temperature",   12,  NULL  , ft_float  , ft_volatile, {f:FS_temp}     , {v:NULL}        , NULL      , } ,
    {"humidity"  ,    12,  NULL  , ft_float  , ft_volatile, {f:FS_Humid}    , {v:NULL}        , NULL      , } ,
    {"current"   ,    12,  NULL  , ft_integer, ft_volatile, {i:FS_Current}  , {v:NULL}        , NULL      , } ,
    {"Ienable"   ,    12,  NULL  , ft_unsigned,ft_stable  , {u:FS_r_Ienable}, {u:FS_w_Ienable}, NULL      , } ,
    {"offset"    ,    12,  NULL  , ft_unsigned,ft_stable  , {i:FS_r_Offset} , {i:FS_w_Offset} , NULL      , } ,
} ;
DeviceEntryExtended( 26, DS2438, DEV_temp )

/* ------- Functions ------------ */

/* DS2438 */
static int OW_r_page( unsigned char * const p , const int page , const struct parsedname * const pn) ;
static int OW_w_page( const unsigned char * const p , const int page , const struct parsedname * const pn ) ;
static int OW_temp( FLOAT * const T , const struct parsedname * const pn ) ;
static int OW_volts( FLOAT * const V , const int src, const struct parsedname * const pn ) ;
static int OW_current( int * const I , const struct parsedname * const pn ) ;
static int OW_r_Ienable( unsigned * const u , const struct parsedname * const pn ) ;
static int OW_w_Ienable( const unsigned u , const struct parsedname * const pn ) ;
static int OW_r_int( int * const I , const unsigned int address, const struct parsedname * const pn ) ;
static int OW_w_int( const int I , const unsigned int address, const struct parsedname * const pn ) ;
static int OW_w_offset( const int I , const struct parsedname * const pn ) ;

/* 2438 A/D */
static int FS_r_page(unsigned char *buf, const size_t size, const off_t offset , const struct parsedname * pn) {
    unsigned char data[8] ;
    if ( OW_r_page( data, pn->extension, pn ) ) return -EINVAL ;
    memcpy(buf,&data[offset],size) ;
    return size ;
}

static int FS_w_page(const unsigned char *buf, const size_t size, const off_t offset , const struct parsedname * pn ) {
    if (size != 8 ) { /* partial page */
        unsigned char data[8] ;
        if ( OW_r_page( data, pn->extension, pn ) ) return -EINVAL ;
        memcpy(&data[offset],buf,size) ;
        if ( OW_w_page(data,pn->extension,pn) ) return -EFAULT ;
    } else { /* complete page */
        if ( OW_w_page(buf,pn->extension,pn) ) return -EFAULT ;
    }
    return 0 ;
}

static int FS_temp(FLOAT * T , const struct parsedname * pn) {
    if ( OW_temp( T , pn ) ) return -EINVAL ;
    *T = Temperature( *T ) ;
    return 0 ;
}

static int FS_volts(FLOAT * V , const struct parsedname * pn) {
    /* data=1 VDD data=0 VAD */
    if ( OW_volts( V , (int) pn->ft->data , pn ) ) return -EINVAL ;
    return 0 ;
}

static int FS_Humid(FLOAT * H , const struct parsedname * pn) {
    FLOAT T,VAD,VDD ;
    if ( OW_volts( &VDD , 1 , pn ) || OW_volts( &VAD , 0 , pn ) || OW_temp( &T , pn ) ) return -EINVAL ;
    *H = (VAD/VDD-.16)/(.0062*(1.0546-.00216*T)) ;
    return 0 ;
}

static int FS_Current(int * I , const struct parsedname * pn) {
    if ( OW_current( I , pn ) ) return -EINVAL ;
    return 0 ;
}

static int FS_r_Ienable(unsigned * u , const struct parsedname * pn) {
    if ( OW_r_Ienable( u , pn ) ) return -EINVAL ;
    return 0 ;
}

static int FS_w_Ienable(const unsigned * u , const struct parsedname * pn) {
    if ( *u > 3 ) return -EINVAL ;
    if ( OW_w_Ienable( *u , pn ) ) return -EINVAL ;
    return 0 ;
}

static int FS_r_Offset(int * i , const struct parsedname * pn) {
    if ( OW_r_int( i , 0x0D, pn ) ) return -EINVAL ; /* page1 byte 5 */
    *i >>= 3 ;
    return 0 ;
}

static int FS_w_Offset(const int * i , const struct parsedname * pn) {
    int I = *i ;
    if ( I > 255 || I < -256 ) return -EINVAL ;
    if ( OW_w_offset( I<<3 , pn ) ) return -EINVAL ;
    return 0 ;
}

/* DS2438 fancy battery */
static int OW_r_page( unsigned char * const p , const int page , const struct parsedname * const pn) {
    unsigned char data[9] ;
    unsigned char recall[] = {0xB8, page, } ;
    unsigned char r[] = {0xBE , page, } ;
    int ret ;

//printf("page: %d %.2X %.2X %.2X %.2X \n",page,cmd[0],cmd[1],cmd[2],cmd[3]) ;
//(printf("Data: %.2X %.2X %.2X %.2X %.2X %.2X %.2X %.2X %.2X\n",data[0],data[1],data[2],data[3],data[4],data[5],data[6],data[7],data[8])<0)||
//printf("Data: %.2X %.2X %.2X %.2X %.2X %.2X %.2X %.2X %.2X\n",p[0],p[1],p[2],p[3],p[4],p[5],p[6],p[7],p[8]) ;
//printf("Data: %.2X %.2X %.2X %.2X %.2X %.2X %.2X %.2X %.2X\n",data[0],data[1],data[2],data[3],data[4],data[5],data[6],data[7],data[8]) ;
    // read to scratch, then in

    BUSLOCK
        ret = BUS_select(pn) || BUS_send_data(recall,2) ;
    BUSUNLOCK
    if ( ret ) return 1 ;

    BUSLOCK
        ret = BUS_select(pn) || BUS_send_data( r, 2 ) || BUS_readin_data( data,9 ) || CRC8( data,9 ) ;
    BUSUNLOCK
    if ( ret ) return 1 ;

    // copy to buffer
    memcpy( p , data , 8 ) ;
    return 0 ;
}

/* write 8 bytes */
static int OW_w_page( const unsigned char * const p , const int page , const struct parsedname * const pn ) {
    unsigned char data[9] ;
    unsigned char w[] = {0x4E, page, } ;
    unsigned char r[] = {0xBE, page, } ;
    unsigned char eeprom[] = {0x48, page, } ;
    int i ;
    int ret ;

    // write then read to scratch, then into EEPROM if scratch matches
    BUSLOCK
        ret = BUS_select(pn) || BUS_send_data( w, 2 ) || BUS_send_data( p, 8 ) ;
    BUSUNLOCK
    if ( ret ) return 1 ;

    BUSLOCK
        ret = BUS_select(pn) || BUS_send_data( r, 2 ) || BUS_readin_data( data,9 ) || CRC8( data,9 ) ;
    BUSUNLOCK
    if ( ret ) return 1 ;
    if ( page && memcmp( p, data, 8 ) ) return 1 ; /* page 0 has readonly fields that won't compare */

    BUSLOCK
    ret = BUS_select(pn) || BUS_send_data(eeprom,2) ;
    BUSUNLOCK
    if ( ret ) return 1 ;

    // Loop waiting for completion
    for ( i=0 ; i<10 ; ++i ) {
        UT_delay(1) ;
        BUSLOCK
            ret = BUS_readin_data(data,1) ;
        BUSUNLOCK
        if ( ret ) return 1 ;
        if ( data[0] ) return 0 ;
    }
    return 1 ; // timeout
}

static int OW_temp( FLOAT * const T , const struct parsedname * const pn ) {
    unsigned char data[9] ;
    static unsigned char t = 0x44 ;
    int i ;
    int ret ;

    // write conversion command
    if ( ! Simul_Test( DEV_temp, 10, pn ) ){
        BUSLOCK
            ret = BUS_select(pn) || BUS_send_data( &t, 1 ) ;
        BUSUNLOCK
        if ( ret ) return 1 ;
        // Loop waiting for completion
        for ( i=0 ; i<10 ; ++i ) {
            UT_delay(1) ;
            BUSLOCK
                ret = BUS_readin_data(data,1) ;
            BUSUNLOCK
            if ( ret ) return 1 ;
            if ( data[0] ) break ;
        }
    }

    // read back registers
    if ( OW_r_page( data , 0 , pn ) ) return 1 ;
    *T = ((int)((signed char)data[2])) + .00390625*data[1] ;
    return 0 ;
}

static int OW_volts( FLOAT * const V , const int src, const struct parsedname * const pn ) {
    // src deserves some explanation:
    //   1 -- VDD (battery) measured
    //   0 -- VAD (other) measured
    unsigned char data[9] ;
    static unsigned char v = 0xB4 ;
    static unsigned char w[] = {0x4E, 0x00, } ;
    int i ;
    int ret ;

    // set voltage source command
    if ( OW_r_page( data , 0 , pn ) ) return 1 ;
    UT_setbit( data , 3 , src ) ; // AD bit in status register

    BUSLOCK
        ret = BUS_select(pn) || BUS_send_data( w , 2 ) || BUS_send_data( data , 8 ) ;
    BUSUNLOCK
    if ( ret ) return 1 ;

    // write conversion command
    BUSLOCK
        ret = BUS_select(pn) || BUS_send_data( &v, 1 ) ;
    BUSUNLOCK
    if ( ret ) return 1 ;

    // Loop waiting for completion
    for ( i=0 ; i<10 ; ++i ) {
        UT_delay(1) ;
        BUSLOCK
            ret = BUS_readin_data(data,1) ;
        BUSUNLOCK
        if ( ret ) return 1 ;
        if ( data[0] ) break ;
    }

    // read back registers
    if ( OW_r_page( data , 0 , pn ) ) return 1 ;
    *V = .01 * (FLOAT)( ( ((int)data[4]) <<8 )|data[3] ) ;
    return 0 ;
}

static int OW_current( int * const I , const struct parsedname * const pn ) {
    unsigned char data[8] ;
    int enabled ;

    // set current readings on source command
    if ( OW_r_page( data , 0 , pn ) ) return 1 ;
    enabled = UT_getbit( data , 0 ) ;
    if ( !enabled ) {
        UT_setbit( data , 0 , 1 ) ; // AD bit in status register
        if ( OW_w_page( data, 0, pn ) ) return 1 ;
    }

    // read back registers
    if ( OW_r_int( I, 0x05, pn ) ) return 1 ;

    if ( !enabled ) {
        // if ( OW_r_page( data , 0 , pn ) ) return 1 ; /* Assume no change to these fields */
        UT_setbit( data , 0 , 0 ) ; // AD bit in status register
        if ( OW_w_page( data, 0, pn ) ) return 1 ;
    }
    return 0 ;
}

static int OW_w_offset( const int I , const struct parsedname * const pn ) {
    unsigned char data[8] ;
    int enabled ;

    // set current readings off source command
    if ( OW_r_page( data , 0 , pn ) ) return 1 ;
    enabled = UT_getbit( data , 0 ) ;
    if ( enabled ) {
        UT_setbit( data , 0 , 0 ) ; // AD bit in status register
        if ( OW_w_page( data, 0, pn ) ) return 1 ;
    }

    // read back registers
    if ( OW_w_int( I, 0x0D, pn ) ) return 1 ; /* page1 byte5 */

    if ( enabled ) {
        // if ( OW_r_page( data , 0 , pn ) ) return 1 ; /* Assume no change to these fields */
        UT_setbit( data , 0 , 1 ) ; // AD bit in status register
        if ( OW_w_page( data, 0, pn ) ) return 1 ;
    }
    return 0 ;
}

static int OW_r_Ienable( unsigned * const u , const struct parsedname * const pn ) {
    unsigned char data[8] ;

    if ( OW_r_page( data , 0 , pn ) ) return 1 ;
    if ( UT_getbit( data,0 ) ) {
        if ( UT_getbit( data , 1 ) ) {
            if ( UT_getbit( data , 2 ) ) {
                *u = 3 ;
            } else {
                *u = 2 ;
            }
        } else {
            *u = 1 ;
        }
    } else {
        *u = 0 ;
    }
    return 0 ;
}

static int OW_w_Ienable( const unsigned u , const struct parsedname * const pn ) {
    unsigned char data[8] ;
    static unsigned char iad[] = { 0x00, 0x01, 0x3, 0x7, } ;

    if ( OW_r_page( data , 0 , pn ) ) return 1 ;
    if ( (data[0] & 0x07) != iad[u] ) { /* only change if needed */
        data[0] = (data[0] & 0xF8) | iad[u] ;
        if ( OW_w_page( data , 0 , pn ) ) return 1 ;
    }
    return 0 ;
}


static int OW_r_int( int * const I , const unsigned int address, const struct parsedname * const pn ) {
    unsigned char data[8] ;

    // read back registers
    if ( OW_r_page( data , address>>3 , pn ) ) return 1 ;
    *I = ((int)((signed char)data[(address&0x07)+1]))<<8 | data[address&0x07] ;
    return 0 ;
}

static int OW_w_int( const int I , const unsigned int address, const struct parsedname * const pn ) {
    unsigned char data[8] ;

    // read back registers
    if ( OW_r_page( data , address>>3 , pn ) ) return 1 ;
    data[address&0x07] = I & 0xFF ;
    data[(address&0x07)+1] = (I>>8) & 0xFF ;
    return OW_w_page( data , address>>3 , pn ) ;
}
