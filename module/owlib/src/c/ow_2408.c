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
#include "ow_2408.h"

/* ------- Prototypes ----------- */

/* DS2408 switch */
 yREAD_FUNCTION( FS_r_strobe ) ;
yWRITE_FUNCTION( FS_w_strobe ) ;
 yREAD_FUNCTION( FS_r_pio ) ;
yWRITE_FUNCTION( FS_w_pio ) ;
 yREAD_FUNCTION( FS_sense ) ;
 yREAD_FUNCTION( FS_power ) ;
 yREAD_FUNCTION( FS_latch ) ;

/* ------- Structures ----------- */

struct aggregate A2408 = { 8, ag_numbers, ag_aggregate, } ;
struct filetype DS2408[] = {
    F_STANDARD   ,
    {"power"     ,     1,  NULL,    ft_yesno , ft_volatile, {y:FS_power}    , {v:NULL},        NULL, } ,
    {"PIO"       ,     1,  &A2408,  ft_yesno , ft_stable  , {y:FS_r_pio}    , {y:FS_w_pio},    NULL, } ,
    {"sensed"    ,     1,  &A2408,  ft_yesno , ft_volatile, {y:FS_sense}    , {v:NULL},        NULL, } ,
    {"latch"     ,     1,  &A2408,  ft_yesno , ft_volatile, {y:FS_latch}    , {v:NULL},        NULL, } ,
    {"strobe"    ,     1,  NULL,    ft_yesno , ft_stable  , {y:FS_r_strobe} , {y:FS_w_strobe}, NULL, } ,
} ;
DeviceEntry( 29, DS2408 )

/* ------- Functions ------------ */

/* DS2408 */
static int OW_w_conditional( const unsigned char * data , const struct parsedname * pn ) ;
static int OW_w_control( const unsigned char data , const struct parsedname * pn ) ;
static int OW_r_latch( unsigned char * data , const struct parsedname * pn ) ;
static int OW_w_pio( const unsigned char data,  const struct parsedname * pn ) ;
static int OW_r_reg( unsigned char * data , const struct parsedname * pn ) ;

/* 2408 switch */
/* 2408 switch -- is Vcc powered?*/
static int FS_power(int * y , const struct parsedname * pn) {
    unsigned char data[8] ;
    if ( OW_r_reg(data,pn) ) return -EINVAL ;
    *y = UT_getbit(&data[5],7) ;
    return 0 ;
}

static int FS_r_strobe(int * y , const struct parsedname * pn) {
    unsigned char data[8] ;
    if ( OW_r_reg(data,pn) ) return -EINVAL ;
    *y = UT_getbit(&data[5],2) ;
    return 0 ;
}

static int FS_w_strobe(const int * y, const struct parsedname * pn) {
	unsigned char data[8] ;
    if ( OW_r_reg(data,pn) ) return -EINVAL ;
	UT_setbit( &data[5], 2, y[0] ) ;
	return OW_w_control( data[5] , pn ) ? -EINVAL : 0 ;
}

/* 2408 switch PIO sensed*/
static int FS_sense(int * y , const struct parsedname * pn) {
    unsigned char data[6] ;
    int i ;
    if ( OW_r_reg(data,pn) ) return -EINVAL ;
    for ( i=0 ; i<8 ; ++i ) y[i] = UT_getbit(&data[0],i) ;
    return 0 ;
}

/* 2408 switch PIO set*/
static int FS_r_pio(int * y , const struct parsedname * pn) {
    unsigned char data[6] ;
    int i ;
    if ( OW_r_reg(data,pn) ) return -EINVAL ;
	data[1] ^= 0xFF ; /* reverse bits */
    for ( i=0 ; i<8 ; ++i ) y[i] = UT_getbit(&data[1],i) ;
    return 0 ;
}

/* 2408 switch PIO change*/
static int FS_w_pio(const int * y , const struct parsedname * pn) {
    unsigned char data ;
	UT_setbit(&data,0,y[0]) ;
	UT_setbit(&data,1,y[1]) ;
	UT_setbit(&data,2,y[2]) ;
	UT_setbit(&data,3,y[3]) ;
	UT_setbit(&data,4,y[4]) ;
	UT_setbit(&data,5,y[5]) ;
	UT_setbit(&data,6,y[6]) ;
	UT_setbit(&data,7,y[7]) ;
	data ^= 0xFF ; /* reverse bits */

    if ( OW_w_pio(data,pn) ) return -EINVAL ;
	return 0 ;
}

/* 2408 switch activity latch -- resets*/
static int FS_latch(int * y , const struct parsedname * pn) {
    unsigned char data ;
    int i ;
    if ( OW_r_latch(&data,pn) ) return -EINVAL ;
    for ( i=0 ; i<8 ; ++i ) y[i] = UT_getbit(&data,i) ;
    return 0 ;
}

/* Read 6 bytes --
   0x88 PIO logic State
   0x89 PIO output Latch state
   0x8A PIO Activity Latch
   0x8B Conditional Ch Mask
   0x8C Londitional Ch Polarity
   0x8D Control/Status
*/
static int OW_r_reg( unsigned char * data , const struct parsedname * pn ) {
    unsigned char p[3+8+2] = { 0xF0, 0x88 , 0x00, } ;
    int ret ;

    BUS_lock() ;
        ret = BUS_select(pn) || BUS_send_data( p , 3 ) || BUS_readin_data( &p[3], 8+2 ) || CRC16(p,3+8+2) ;
    BUS_unlock() ;
    if ( ret ) return 1 ;

    memcpy( data , &p[3], 6 ) ;
    return 0 ;
}

static int OW_w_pio( const unsigned char data,  const struct parsedname * pn ) {
    unsigned char p[] = { 0x5A, data , ~data, 0xFF, 0xFF, } ;
    int ret ;
//printf("wPIO data = %2X %2X %2X %2X %2X\n",p[0],p[1],p[2],p[3],p[4]) ;
    BUS_lock() ;
        ret = BUS_select(pn) || BUS_sendback_data(p,p,5) || p[3]!=0xAA ;
    BUS_unlock() ;
//printf("wPIO data = %2X %2X %2X %2X %2X\n",p[0],p[1],p[2],p[3],p[4]) ;
	/* Ignore byte 5 p[4] the PIO status byte */
    return ret ;
}

/* Read and reset teh activity latch */
static int OW_r_latch( unsigned char * data , const struct parsedname * pn ) {
    unsigned char d[6] ; /* register read */
	unsigned char p[] = { 0xC3, 0xFF, } ;
    int ret ;

    /* Read registers (before clearing) */
	if ( OW_r_reg(d,pn) ) return 1 ;

	BUS_lock() ;
        ret = BUS_select(pn) || BUS_sendback_data( p , p , 2 ) || p[1]!=0xAA ;
    BUS_unlock() ;
    if ( ret ) return 1 ;

    *data = d[2] ; /* register 0x8A */
    return 0 ;
}

/* Write control/status */
static int OW_w_control( const unsigned char data , const struct parsedname * pn ) {
    unsigned char d[6] ; /* register read */
	unsigned char p[] = { 0xCC, 0x8D, 0x00, data, } ;
    int ret ;

	BUS_lock() ;
        ret = BUS_select(pn) || BUS_send_data( p , 4 ) ;
    BUS_unlock() ;
    if ( ret ) return -EINVAL ;

    /* Read registers */
	if ( OW_r_reg(d,pn) ) return -EINVAL ;

    return ( data != d[5] ) ;
}

/* Write conditionakl search bytes (2 bytes) */
static int OW_w_conditional( const unsigned char * data , const struct parsedname * pn ) {
    unsigned char d[6] ; /* register read */
	unsigned char p[] = { 0xCC, 0x8B, 0x00, data[0], data[1], } ;
    int ret ;

	BUS_lock() ;
        ret = BUS_select(pn) || BUS_send_data( p , 5 ) ;
    BUS_unlock() ;
    if ( ret ) return 1 ;

    /* Read registers */
	if ( OW_r_reg(d,pn) ) return 1 ;

    return  ( data[0] != d[3] ) || ( data[1] != d[4] ) ;
}
