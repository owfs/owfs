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
 bREAD_FUNCTION( FS_r_26page ) ;
bWRITE_FUNCTION( FS_w_26page ) ;
 fREAD_FUNCTION( FS_26temp ) ;
 fREAD_FUNCTION( FS_26volts ) ;
 fREAD_FUNCTION( FS_26Humid ) ;
 iREAD_FUNCTION( FS_26Current ) ;
 uREAD_FUNCTION( FS_r_26Ienable ) ;
uWRITE_FUNCTION( FS_w_26Ienable ) ;
 iREAD_FUNCTION( FS_r_26Offset ) ;
iWRITE_FUNCTION( FS_w_26Offset ) ;

/* ------- Structures ----------- */

struct aggregate A2438 = { 8, ag_numbers, ag_separate, } ;
struct filetype DS2438[] = {
    F_STANDARD   ,
    {"page"      ,     8,  &A2438, ft_binary , ft_stable  , {b:FS_r_26page}   , {b:FS_w_26page}, NULL, } ,
    {"VDD"       ,    12,  NULL  , ft_float  , ft_volatile, {f:FS_26volts}    , {v:NULL}, (void *) 1 , } ,
    {"VAD"       ,    12,  NULL  , ft_float  , ft_volatile, {f:FS_26volts}    , {v:NULL}, (void *) 0, } ,
    {"temperature",   12,  NULL  , ft_float  , ft_volatile, {f:FS_26temp}     , {v:NULL}, NULL, } ,
    {"humidity"  ,    12,  NULL  , ft_float  , ft_volatile, {f:FS_26Humid}    , {v:NULL} , NULL,} ,
    {"current"   ,    12,  NULL  , ft_integer, ft_volatile, {i:FS_26Current}  , {v:NULL} , NULL,} ,
    {"Ienable"   ,    12,  NULL  , ft_unsigned,ft_stable  , {u:FS_r_26Ienable}, {u:FS_w_26Ienable} , NULL,} ,
    {"offset"    ,    12,  NULL  , ft_unsigned,ft_stable  , {i:FS_r_26Offset} , {i:FS_w_26Offset}  , NULL,} ,
} ;
DeviceEntry( 26, DS2438 )

/* ------- Functions ------------ */

/* DS2438 */
static int OW_r_26page( unsigned char * const p , const int page , const struct parsedname * const pn) ;
static int OW_w_26page( const unsigned char * const p , const int page , const struct parsedname * const pn ) ;
static int OW_26temp( float * const T , const struct parsedname * const pn ) ;
static int OW_26volts( float * const V , const int src, const struct parsedname * const pn ) ;
static int OW_26current( int * const I , const struct parsedname * const pn ) ;
static int OW_r_26Ienable( unsigned * const u , const struct parsedname * const pn ) ;
static int OW_w_26Ienable( const unsigned u , const struct parsedname * const pn ) ;
static int OW_r_26int( int * const I , const unsigned int address, const struct parsedname * const pn ) ;
static int OW_w_26int( const int I , const unsigned int address, const struct parsedname * const pn ) ;
static int OW_w_26offset( const int I , const struct parsedname * const pn ) ;

/* 2438 A/D */
int FS_r_26page(unsigned char *buf, const size_t size, const off_t offset , const struct parsedname * pn) {
    unsigned char data[8] ;
    if ( OW_r_26page( data, pn->extension, pn ) ) return -EINVAL ;
    return FS_read_return(buf,size,offset,data,8) ;
}

int FS_w_26page(const unsigned char *buf, const size_t size, const off_t offset , const struct parsedname * pn ) {
    if (size != 8 ) return -EMSGSIZE ;
    if (offset != 8 ) return -EINVAL ;
    if ( OW_w_26page(buf,pn->extension,pn) ) return -EFAULT ;
    return 0 ;
}

int FS_26temp(float * T , const struct parsedname * pn) {
    if ( OW_26temp( T , pn ) ) return -EINVAL ;
    *T = Temperature( *T ) ;
    return 0 ;
}

int FS_26volts(float * V , const struct parsedname * pn) {
    /* data=1 VDD data=0 VAD */
    if ( OW_26volts( V , (int) pn->ft->data , pn ) ) return -EINVAL ;
    return 0 ;
}

int FS_26Humid(float * H , const struct parsedname * pn) {
    float T,VAD,VDD ;
    if ( OW_26volts( &VDD , 1 , pn ) || OW_26volts( &VAD , 0 , pn ) || OW_26temp( &T , pn ) ) return -EINVAL ;
    *H = (VAD/VDD-.16)/(.0062*(1.0546-.00216*T)) ;
    return 0 ;
}

int FS_26Current(int * I , const struct parsedname * pn) {
    if ( OW_26current( I , pn ) ) return -EINVAL ;
    return 0 ;
}

int FS_r_26Ienable(unsigned * u , const struct parsedname * pn) {
    if ( OW_r_26Ienable( u , pn ) ) return -EINVAL ;
    return 0 ;
}

int FS_w_26Ienable(const unsigned * u , const struct parsedname * pn) {
    if ( *u > 3 ) return -EINVAL ;
    if ( OW_w_26Ienable( *u , pn ) ) return -EINVAL ;
    return 0 ;
}

int FS_r_26Offset(int * i , const struct parsedname * pn) {
    if ( OW_r_26int( i , 0x0D, pn ) ) return -EINVAL ; /* page1 byte 5 */
    *i >>= 3 ;
    return 0 ;
}

int FS_w_26Offset(const int * i , const struct parsedname * pn) {
    int I = *i ;
    if ( I > 255 || I < -256 ) return -EINVAL ;
    if ( OW_w_26offset( I<<3 , pn ) ) return -EINVAL ;
    return 0 ;
}

/* DS2438 fancy battery */
static int OW_r_26page( unsigned char * const p , const int page , const struct parsedname * const pn) {
    unsigned char data[9] ;
    unsigned char recall[] = {0xB8, page, } ;
    unsigned char r[] = {0xBE , page, } ;
    int ret ;

//printf("page: %d %.2X %.2X %.2X %.2X \n",page,cmd[0],cmd[1],cmd[2],cmd[3]) ;
//(printf("Data: %.2X %.2X %.2X %.2X %.2X %.2X %.2X %.2X %.2X\n",data[0],data[1],data[2],data[3],data[4],data[5],data[6],data[7],data[8])<0)||
//printf("Data: %.2X %.2X %.2X %.2X %.2X %.2X %.2X %.2X %.2X\n",p[0],p[1],p[2],p[3],p[4],p[5],p[6],p[7],p[8]) ;
//printf("Data: %.2X %.2X %.2X %.2X %.2X %.2X %.2X %.2X %.2X\n",data[0],data[1],data[2],data[3],data[4],data[5],data[6],data[7],data[8]) ;
    // read to scratch, then in

    BUS_lock() ;
        ret = BUS_select(pn) || BUS_send_data(recall,2) ;
    BUS_unlock() ;
    if ( ret ) return 1 ;

    BUS_lock() ;
        ret = BUS_select(pn) || BUS_send_data( r, 2 ) || BUS_readin_data( data,9 ) || CRC8( data,9 ) ;
    BUS_unlock() ;
    if ( ret ) return 1 ;

    // copy to buffer
    memcpy( p , data , 8 ) ;
    return 0 ;
}

/* write 8 bytes */
static int OW_w_26page( const unsigned char * const p , const int page , const struct parsedname * const pn ) {
    unsigned char data[9] ;
    unsigned char w[] = {0x4E, page, } ;
    unsigned char r[] = {0xBE, page, } ;
    unsigned char eeprom[] = {0x48, page, } ;
    int i ;
    int ret ;

    // write then read to scratch, then into EEPROM if scratch matches
    BUS_lock() ;
        ret = BUS_select(pn) || BUS_send_data( w, 2 ) || BUS_send_data( p, 8 ) ;
    BUS_unlock() ;
    if ( ret ) return 1 ;

    BUS_lock() ;
        ret = BUS_select(pn) || BUS_send_data( r, 2 ) || BUS_readin_data( data,9 ) || CRC8( data,9 ) ;
    BUS_unlock() ;
    if ( ret ) return 1 ;
    if ( page && memcpy( p, data, 8 ) ) return 1 ; /* page 0 has readonly fields that won't compare */

    BUS_lock() ;
    ret = BUS_select(pn) || BUS_send_data(eeprom,2) ;
    BUS_unlock() ;
    if ( ret ) return 1 ;

    // Loop waiting for completion
    for ( i=0 ; i<10 ; ++i ) {
        UT_delay(1) ;
        BUS_lock() ;
            ret = BUS_readin_data(data,1) ;
        BUS_unlock() ;
        if ( ret ) return 1 ;
        if ( data[0] ) return 0 ;
    }
    return 1 ; // timeout
}

static int OW_26temp( float * const T , const struct parsedname * const pn ) {
    unsigned char data[9] ;
    static unsigned char t = 0x44 ;
    int i ;
    int ret ;

    // write conversion command
    BUS_lock() ;
        ret = BUS_select(pn) || BUS_send_data( &t, 1 ) ;
    BUS_unlock() ;
    if ( ret ) return 1 ;

    // Loop waiting for completion
    for ( i=0 ; i<10 ; ++i ) {
        UT_delay(1) ;
        BUS_lock() ;
            ret = BUS_readin_data(data,1) ;
        BUS_unlock() ;
        if ( ret ) return 1 ;
        if ( data[0] ) break ;
    }

    // read back registers
    if ( OW_r_26page( data , 0 , pn ) ) return 1 ;
    *T = ((int)((signed char)data[2])) + .00390625*data[1] ;
    return 0 ;
}

static int OW_26volts( float * const V , const int src, const struct parsedname * const pn ) {
    // src deserves some explanation:
    //   1 -- VDD (battery) measured
    //   0 -- VAD (other) measured
    unsigned char data[9] ;
    static unsigned char v = 0xB4 ;
    static unsigned char w[] = {0x4E, 0x00, } ;
    int i ;
    int ret ;

    // set voltage source command
    if ( OW_r_26page( data , 0 , pn ) ) return 1 ;
    UT_setbit( data , 3 , src ) ; // AD bit in status register

    BUS_lock() ;
        ret = BUS_select(pn) || BUS_send_data( w , 2 ) || BUS_send_data( data , 8 ) ;
    BUS_unlock() ;
    if ( ret ) return 1 ;

    // write conversion command
    BUS_lock() ;
        ret = BUS_select(pn) || BUS_send_data( &v, 1 ) ;
    BUS_unlock() ;
    if ( ret ) return 1 ;

    // Loop waiting for completion
    for ( i=0 ; i<10 ; ++i ) {
        UT_delay(1) ;
        BUS_lock() ;
            ret = BUS_readin_data(data,1) ;
        BUS_unlock() ;
        if ( ret ) return 1 ;
        if ( data[0] ) break ;
    }

    // read back registers
    if ( OW_r_26page( data , 0 , pn ) ) return 1 ;
    *V = .01 * (float)( ( ((int)data[4]) <<8 )|data[3] ) ;
    return 0 ;
}

static int OW_26current( int * const I , const struct parsedname * const pn ) {
    unsigned char data[8] ;
    int enabled ;

    // set current readings on source command
    if ( OW_r_26page( data , 0 , pn ) ) return 1 ;
    enabled = UT_getbit( data , 0 ) ;
    if ( !enabled ) {
        UT_setbit( data , 0 , 1 ) ; // AD bit in status register
        if ( OW_w_26page( data, 0, pn ) ) return 1 ;
    }

    // read back registers
    if ( OW_r_26int( I, 0x05, pn ) ) return 1 ;

    if ( !enabled ) {
        // if ( OW_r_26page( data , 0 , pn ) ) return 1 ; /* Assume no change to these fields */
        UT_setbit( data , 0 , 0 ) ; // AD bit in status register
        if ( OW_w_26page( data, 0, pn ) ) return 1 ;
    }
    return 0 ;
}

static int OW_w_26offset( const int I , const struct parsedname * const pn ) {
    unsigned char data[8] ;
    int enabled ;

    // set current readings off source command
    if ( OW_r_26page( data , 0 , pn ) ) return 1 ;
    enabled = UT_getbit( data , 0 ) ;
    if ( enabled ) {
        UT_setbit( data , 0 , 0 ) ; // AD bit in status register
        if ( OW_w_26page( data, 0, pn ) ) return 1 ;
    }

    // read back registers
    if ( OW_w_26int( I, 0x0D, pn ) ) return 1 ; /* page1 byte5 */

    if ( enabled ) {
        // if ( OW_r_26page( data , 0 , pn ) ) return 1 ; /* Assume no change to these fields */
        UT_setbit( data , 0 , 1 ) ; // AD bit in status register
        if ( OW_w_26page( data, 0, pn ) ) return 1 ;
    }
    return 0 ;
}

static int OW_r_26Ienable( unsigned * const u , const struct parsedname * const pn ) {
    unsigned char data[8] ;

    if ( OW_r_26page( data , 0 , pn ) ) return 1 ;
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

static int OW_w_26Ienable( const unsigned u , const struct parsedname * const pn ) {
    unsigned char data[8] ;
    static unsigned char iad[] = { 0x00, 0x01, 0x3, 0x7, } ;

    if ( OW_r_26page( data , 0 , pn ) ) return 1 ;
    if ( (data[0] & 0x07) != iad[u] ) { /* only change if needed */
        data[0] = (data[0] & 0xF8) | iad[u] ;
        if ( OW_w_26page( data , 0 , pn ) ) return 1 ;
    }
    return 0 ;
}


static int OW_r_26int( int * const I , const unsigned int address, const struct parsedname * const pn ) {
    unsigned char data[8] ;

    // read back registers
    if ( OW_r_26page( data , address>>3 , pn ) ) return 1 ;
    *I = ((int)((signed char)data[(address&0x07)+1]))<<8 | data[address&0x07] ;
    return 0 ;
}

static int OW_w_26int( const int I , const unsigned int address, const struct parsedname * const pn ) {
    unsigned char data[8] ;

    // read back registers
    if ( OW_r_26page( data , address>>3 , pn ) ) return 1 ;
    data[address&0x07] = I & 0xFF ;
    data[(address&0x07)+1] = (I>>8) & 0xFF ;
    return OW_w_26page( data , address>>3 , pn ) ;
}
