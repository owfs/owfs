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
 uREAD_FUNCTION( FS_r_pio ) ;
uWRITE_FUNCTION( FS_w_pio ) ;
 uREAD_FUNCTION( FS_sense ) ;
 yREAD_FUNCTION( FS_power ) ;
 uREAD_FUNCTION( FS_r_latch ) ;
uWRITE_FUNCTION( FS_w_latch ) ;
 uREAD_FUNCTION( FS_r_s_alarm ) ;
uWRITE_FUNCTION( FS_w_s_alarm ) ;
 yREAD_FUNCTION( FS_r_por ) ;
yWRITE_FUNCTION( FS_w_por ) ;

/* ------- Structures ----------- */

struct aggregate A2408 = { 8, ag_numbers, ag_aggregate, } ;
struct filetype DS2408[] = {
    F_STANDARD   ,
    {"power"     ,     1,  NULL,    ft_yesno   , ft_volatile, {y:FS_power}    , {v:NULL},        NULL, } ,
    {"PIO"       ,     1,  &A2408,  ft_bitfield, ft_stable  , {u:FS_r_pio}    , {u:FS_w_pio},    NULL, } ,
    {"sensed"    ,     1,  &A2408,  ft_bitfield, ft_volatile, {u:FS_sense}    , {v:NULL},        NULL, } ,
    {"latch"     ,     1,  &A2408,  ft_bitfield, ft_volatile, {u:FS_r_latch}  , {u:FS_w_latch},  NULL, } ,
    {"strobe"    ,     1,  NULL,    ft_yesno   , ft_stable  , {y:FS_r_strobe} , {y:FS_w_strobe}, NULL, } ,
    {"set_alarm" ,     9,  NULL,    ft_unsigned, ft_stable  , {u:FS_r_s_alarm}, {u:FS_w_s_alarm},NULL, } ,
    {"por"       ,     1,  NULL,    ft_yesno   , ft_stable  , {y:FS_r_por}    , {y:FS_w_por},    NULL, } ,
} ;
DeviceEntryExtended( 29, DS2408, DEV_alarm )

/* ------- Functions ------------ */

/* DS2408 */
static int OW_w_control( const unsigned char data , const struct parsedname * pn ) ;
static int OW_c_latch( const struct parsedname * pn ) ;
static int OW_w_pio( const unsigned char data,  const struct parsedname * pn ) ;
static int OW_r_reg( unsigned char * data , const struct parsedname * pn ) ;
static int OW_w_s_alarm( const unsigned char *data , const struct parsedname * pn ) ;

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
/* From register 0x88 */
static int FS_sense(unsigned int * u, const struct parsedname * pn) {
    unsigned char data[6] ;
    if ( OW_r_reg(data,pn) ) return -EINVAL ;
    u[0] = data[0] ;
    return 0 ;
}

/* 2408 switch PIO set*/
/* From register 0x89 */
static int FS_r_pio(unsigned int * u , const struct parsedname * pn) {
    unsigned char data[6] ;
    if ( OW_r_reg(data,pn) ) return -EINVAL ;
    u[0] = data[1] ^ 0xFF ; /* reverse bits */
    return 0 ;
}

/* 2408 switch PIO change*/
static int FS_w_pio(const unsigned int * u , const struct parsedname * pn) {
    /* reverse bits */
    if ( OW_w_pio((u[0]&0xFF)^0xFF,pn) ) return -EINVAL ;
    return 0 ;
}

/* 2408 read activity latch */
/* From register 0x8A */
static int FS_r_latch(unsigned int * u , const struct parsedname * pn) {
    unsigned char data[6] ;
    if ( OW_r_reg(data,pn) ) return -EINVAL ;
    u[0] = data[2] ;
    return 0 ;
}

/* 2408 write activity latch */
/* Actually resets them all */
static int FS_w_latch(const unsigned int * u, const struct parsedname * pn) {
    (void) u ;
    if ( OW_c_latch(pn) ) return -EINVAL ;
    return 0 ;
}

/* 2408 alarm settings*/
/* From registers 0x8B-0x8D */
static int FS_r_s_alarm(unsigned int * u , const struct parsedname * pn) {
    unsigned char d[6] ;
    int i, p ;
    if ( OW_r_reg(d,pn) ) return -EINVAL ;
    /* register 0x8D */
    u[0] = ( d[5] & 0x03 ) * 100000000 ;
    /* registers 0x8B and 0x8C */
    for ( i=0, p=1 ; i<8 ; ++i, p*=10 )
        u[0] += UT_getbit(&d[3],i) | ( UT_getbit(&d[4],i) << 1 ) * p ;
    return 0 ;
}

/* 2408 alarm settings*/
static int FS_w_s_alarm(const unsigned int * u , const struct parsedname * pn) {
    unsigned char data[3];
    int i, p ;
    for ( i=0, p=1 ; i<8 ; ++i, p*=10 ) {
        UT_setbit(&data[0],i,(int)(u[0] / p % 10) & 0x01) ;
        UT_setbit(&data[1],i,((int)(u[0] / p % 10) & 0x02) >> 1) ;
    }
    data[2] = (u[0] / 100000000 % 10) & 0x03 ;
    if ( OW_w_s_alarm(data,pn) ) return -EINVAL ;
    return 0 ;
}

static int FS_r_por(int * y , const struct parsedname * pn) {
    unsigned char data[8] ;
    if ( OW_r_reg(data,pn) ) return -EINVAL ;
    *y = UT_getbit(&data[5],3) ;
    return 0 ;
}

static int FS_w_por(const int * y, const struct parsedname * pn) {
    unsigned char data[8] ;
    if ( OW_r_reg(data,pn) ) return -EINVAL ;
    UT_setbit( &data[5], 3, y[0] ) ;
    return OW_w_control( data[5] , pn ) ? -EINVAL : 0 ;
}

/* Read 6 bytes --
   0x88 PIO logic State
   0x89 PIO output Latch state
   0x8A PIO Activity Latch
   0x8B Conditional Ch Mask
   0x8C Londitional Ch Polarity
   0x8D Control/Status
   plus 2 more bytes to get to the end of the page and qualify for a CRC16 checksum
*/
static int OW_r_reg( unsigned char * data , const struct parsedname * pn ) {
    unsigned char p[3+8+2] = { 0xF0, 0x88 , 0x00, } ;
    int ret ;

    BUSLOCK(pn);
        ret = BUS_select(pn) || BUS_send_data( p , 3,pn ) || BUS_readin_data( &p[3], 8+2,pn ) || CRC16(p,3+8+2) ;
    BUSUNLOCK(pn);
    if ( ret ) return 1 ;

    memcpy( data , &p[3], 6 ) ;
    return 0 ;
}

static int OW_w_pio( const unsigned char data,  const struct parsedname * pn ) {
    unsigned char p[] = { 0x5A, data , ~data, 0xFF, 0xFF, } ;
    int ret ;
//printf("wPIO data = %2X %2X %2X %2X %2X\n",p[0],p[1],p[2],p[3],p[4]) ;
    BUSLOCK(pn);
        ret = BUS_select(pn) || BUS_sendback_data(p,p,5,pn) || p[3]!=0xAA ;
    BUSUNLOCK(pn);
//printf("wPIO data = %2X %2X %2X %2X %2X\n",p[0],p[1],p[2],p[3],p[4]) ;
    /* Ignore byte 5 p[4] the PIO status byte */
    return ret ;
}

/* Reset activity latch */
static int OW_c_latch(const struct parsedname * pn) {
    unsigned char p[] = { 0xC3, 0xFF, } ;
    int ret ;
    BUSLOCK(pn);
    ret = BUS_select(pn) || BUS_sendback_data( p , p , 2,pn ) || p[1]!=0xAA ;
    BUSUNLOCK(pn);
    return ret ;
}

/* Write control/status */
static int OW_w_control( const unsigned char data , const struct parsedname * pn ) {
    unsigned char d[6] ; /* register read */
    unsigned char p[] = { 0xCC, 0x8D, 0x00, data, } ;
    int ret ;

    BUSLOCK(pn);
        ret = BUS_select(pn) || BUS_send_data( p , 4,pn ) ;
    BUSUNLOCK(pn);
    if ( ret ) return -EINVAL ;

    /* Read registers */
    if ( OW_r_reg(d,pn) ) return -EINVAL ;

    return ( (data & 0x0F) != (d[5] & 0x0F) ) ;
}

/* write alarm settings */
static int OW_w_s_alarm( const unsigned char *data , const struct parsedname * pn ) {
    unsigned char d[6], cr ;
    unsigned char c[] = { 0xCC, 0x8D, 0x00, 0x00, } ;
    unsigned char s[] = { 0xCC, 0x8B, 0x00, data[0], data[1], } ;
    int ret ;

    if ( OW_r_reg(d,pn) ) return -EINVAL ;
    c[3] = cr = (data[2] & 0x03) | (d[5] & 0x0C) ;
    BUSLOCK(pn);
        ret = BUS_select(pn) || BUS_send_data( s , 5,pn ) || BUS_send_data( c , 4,pn ) ;
    BUSUNLOCK(pn);
    if ( ret ) return 1 ;

    /* Read registers */
    if ( OW_r_reg(d,pn) ) return 1 ;

    return  ( data[0] != d[3] ) || ( data[1] != d[4] ) || ( cr != (d[5] & 0x0F) ) ;
}

