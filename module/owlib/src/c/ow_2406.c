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
#include "ow_2406.h"

/* ------- Prototypes ----------- */

/* DS2406 switch */
 bREAD_FUNCTION( FS_r_12mem ) ;
bWRITE_FUNCTION( FS_w_12mem ) ;
 bREAD_FUNCTION( FS_r_12page ) ;
bWRITE_FUNCTION( FS_w_12page ) ;
 yREAD_FUNCTION( FS_r_12pio ) ;
yWRITE_FUNCTION( FS_w_12pio ) ;
 yREAD_FUNCTION( FS_12power ) ;
 uREAD_FUNCTION( FS_12channel ) ;
 yREAD_FUNCTION( FS_12sense ) ;

/* ------- Structures ----------- */

struct aggregate A2406 = { 2, ag_letters, ag_aggregate, } ;
struct aggregate A2406p = { 4, ag_numbers, ag_separate, } ;
struct filetype DS2406[] = {
    F_STANDARD   ,
    {"memory"    ,   128,  NULL,    ft_binary  , ft_stable  , {b:FS_r_12mem}    , {b:FS_w_12mem}, NULL, } ,
    {"page"      ,    32,  &A2406p, ft_binary  , ft_stable  , {b:FS_r_12page}    , {b:FS_w_12page}, NULL, } ,
    {"power"     ,     1,  NULL,    ft_yesno   , ft_volatile, {y:FS_12power}    , {v:NULL}, NULL, } ,
    {"channels"  ,     1,  NULL,    ft_unsigned, ft_stable  , {u:FS_12channel}  , {v:NULL}, NULL, } ,
    {"PIO"       ,     1,  &A2406,  ft_yesno   , ft_stable  , {y:FS_r_12pio}    , {y:FS_w_12pio}, NULL, } ,
    {"sensed"    ,     1,  &A2406,  ft_yesno   , ft_volatile, {y:FS_12sense}    , {v:NULL}, NULL, } ,
} ;
DeviceEntry( 12, DS2406 )

/* ------- Functions ------------ */

/* DS2406 */
static int OW_r_12mem( unsigned char * data , const int length, const int location, const struct parsedname * pn ) ;
static int OW_w_12mem( const unsigned char * data , const int length , const int location, const struct parsedname * pn ) ;
// static int OW_r_12control( unsigned char * data , const struct parsedname * pn ) ;
static int OW_w_12control( const unsigned char data , const struct parsedname * pn ) ;
static int OW_w_12pio( const int * pio , const struct parsedname * pn ) ;
static int OW_12access( unsigned char * data , const struct parsedname * pn ) ;

/* 2406 memory read */
int FS_r_12mem(unsigned char *buf, const size_t size, const off_t offset , const struct parsedname * pn) {
    int len = size ;
    if ( size+offset>128) len = 128-offset ;
    if ( OW_r_12mem( buf, len, (int) offset, pn ) ) return -EINVAL ;
    return len ;
}

/* 2406 memory write */
int FS_r_12page(unsigned char *buf, const size_t size, const off_t offset , const struct parsedname * pn) {
    if ( size+offset>32) return -EMSGSIZE;
    if ( OW_r_12mem( buf, size, offset+(pn->extension<<5), pn) ) return -EINVAL ;
    return size ;
}

int FS_w_12page(const unsigned char *buf, const size_t size, const off_t offset , const struct parsedname * pn) {
    if ( size+offset>32) return -EMSGSIZE;
    if ( OW_w_12mem( buf, size, offset+(pn->extension<<5), pn) ) return -EINVAL ;
    return 0 ;
}

/* Note, it's EPROM -- write once */
int FS_w_12mem(const unsigned char *buf, const size_t size, const off_t offset , const struct parsedname * pn) {
    int len = size ;
    if ( size+offset>128) len = 128-offset ;
    if ( OW_w_12mem( buf, len, (int) offset, pn ) ) return -EINVAL ;
    return 0 ;
}

/* 2406 switch */
int FS_r_12pio(int * y , const struct parsedname * pn) {
    unsigned char data ;
    if ( OW_12access(&data,pn) ) return -EINVAL ;
//printf("RPIO data=%2X\n",data) ;
    data ^= 0xFF ; /* reverse bits */
//printf("RPIO data=%2X\n",data) ;
    y[0] = UT_getbit(&data,0) ;
    y[1] = UT_getbit(&data,1) ;
//printf("RPIO pio=%d,%d\n",y[0],y[1]) ;
    return 0 ;
}

/* 2406 switch -- is Vcc powered?*/
int FS_12power(int * y , const struct parsedname * pn) {
    unsigned char data ;
    if ( OW_12access(&data,pn) ) return -EINVAL ;
    *y = UT_getbit(&data,7) ;
    return 0 ;
}

/* 2406 switch -- number of channels (actually, if Vcc powered)*/
int FS_12channel(unsigned int * u , const struct parsedname * pn) {
    unsigned char data ;
    if ( OW_12access(&data,pn) ) return -EINVAL ;
    *u = (data&0x40)?2:1 ;
    return 0 ;
}

/* 2406 switch PIO sensed*/
int FS_12sense(int * y , const struct parsedname * pn) {
    unsigned char data ;
    if ( OW_12access(&data,pn) ) return -EINVAL ;
    y[0] = UT_getbit(&data,2) ;
    y[1] = UT_getbit(&data,3) ;
    return 0 ;
}

/* write 2406 switch -- 2 values*/
int FS_w_12pio(const int * y, const struct parsedname * pn) {
    if ( OW_w_12pio(y,pn) ) return -EINVAL ;
    return 0 ;
}

static int OW_r_12mem( unsigned char * data , const int length , const int location, const struct parsedname * pn ) {
    unsigned char p[3+128+2] = { 0xF0, location&0xFF , location>>8, } ;
    int ret ;

    BUS_lock() ;
        ret = (location+length>128) || BUS_select(pn) || BUS_send_data( p , 3 ) || BUS_readin_data( &p[3], 128+2-location ) || CRC16(p,3+128+2-location) ;
    BUS_unlock() ;
    if ( ret ) return 1 ;

    memcpy( data , &p[3], (size_t) length ) ;
    return 0 ;
}

static int OW_w_12mem( const unsigned char * data , const int length , const int location, const struct parsedname * pn ) {
    unsigned char p[6] = { 0x0F, location&0xFF , location>>8, data[0], } ;
    unsigned char resp ;
    int i ;
    int ret ;

    BUS_lock() ;
        ret = (length==0) || BUS_select(pn) || BUS_send_data(p,4) || BUS_readin_data(&p[4],2) || CRC16(p,6) || BUS_ProgramPulse() || BUS_readin_data(&resp,1) || ((p[3]|resp)!=p[3]) ;
    BUS_unlock() ;
    if ( ret ) return 1 ;

    for ( i=1 ; i<length ; ++i ) {
        p[3] = data[i] ;
        if ( (++p[1])==0x00 ) ++p[2] ;
        BUS_lock() ;
            ret = BUS_send_data(&p[1],3) || BUS_readin_data(&p[4],2) || CRC16(&p[1],5) || BUS_ProgramPulse() || BUS_readin_data(&resp,1) || ((p[3]|resp)!=p[3]) ;
        BUS_unlock() ;
        if ( ret ) return 1 ;
    }
    return 0 ;
}

/*
static int OW_r_12control( unsigned char * data , const struct parsedname * pn ) {
    unsigned char p[3+8+2] = { 0xAA, 0x00 , 0x00, } ;
    int ret ;

    BUS_lock() ;
        ret = BUS_select(pn) || BUS_send_data( p , 3 ) || BUS_readin_data( &p[3], 8+2 ) || CRC16(p,3+8+2) ;
    BUS_unlock() ;
    if ( ret ) return 1 ;

    *data = p[3+7] ;
    return 0 ;
}
*/

static int OW_w_12control( const unsigned char data , const struct parsedname * pn ) {
    unsigned char p[3+1+2] = { 0x55, 0x07 , 0x00, data, } ;
    int ret ;

    BUS_lock() ;
        ret = BUS_select(pn) || BUS_send_data( p , 4 ) || BUS_readin_data( &p[4], 2 ) || CRC16(p,6) ;
    BUS_unlock() ;
    return ret ;
}

/* set PIO state pio: open=1 closed=0 num: A=0 B=1 */
static int OW_w_12pio( const int * pio , const struct parsedname * pn ) {
    unsigned char data = 0x1F;
    UT_setbit( &data , 5 , pio[0]==0 ) ;
    UT_setbit( &data , 6 , pio[1]==0 ) ;
    return OW_w_12control( data , pn ) ;
}

static int OW_12access( unsigned char * data , const struct parsedname * pn ) {
    unsigned char p[3+2+2] = { 0xF5, 0xD5 , 0xFF, } ;
    int ret ;

    BUS_lock() ;
         ret =BUS_select(pn) || BUS_send_data( p , 3 ) || BUS_readin_data( &p[3], 2+2 ) || CRC16(p,3+2+2) ;
    BUS_unlock() ;
    if ( ret ) return 1 ;

    *data = p[3] ;
//printf("ACCESS %.2X %.2X\n",p[3],p[4]);
    return 0 ;
}





