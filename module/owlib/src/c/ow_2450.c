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
#include "ow_2450.h"

/* ------- Prototypes ----------- */

/* DS2450 A/D */
 bREAD_FUNCTION( FS_r_20page ) ;
bWRITE_FUNCTION( FS_w_20page ) ;
 bREAD_FUNCTION( FS_r_20mem ) ;
bWRITE_FUNCTION( FS_w_20mem ) ;
 fREAD_FUNCTION( FS_20volts ) ;

/* ------- Structures ----------- */

struct aggregate A2450  = { 4, ag_numbers, ag_separate, } ;
struct aggregate A2450v = { 4, ag_letters, ag_aggregate, } ;
struct filetype DS2450[] = {
    F_STANDARD   ,
    {"page"      ,     8,  &A2450,  ft_binary, ft_stable  , {b:FS_r_20page}   , {b:FS_w_20page}, NULL, } ,
    {"memory"    ,    32,  NULL,    ft_binary, ft_stable  , {b:FS_r_20mem}    , {b:FS_w_20mem}, NULL, } ,
    {"volts"     ,    12,  &A2450v, ft_float , ft_volatile, {f:FS_20volts}    , {v:NULL}, NULL, } ,
} ;
DeviceEntry( 20, DS2450 )

/* ------- Functions ------------ */

/* DS2450 */
static int OW_r_20page( char * p , const int size, const int location , const struct parsedname * pn) ;
static int OW_w_20page( const char * p , const int size , const int location , const struct parsedname * pn) ;
static int OW_20volts( float * f , const struct parsedname * pn ) ;

/* 2450 A/D */
int FS_r_20page(unsigned char *buf, const size_t size, const off_t offset , const struct parsedname * pn) {
    unsigned char p[8] ;
    if ( size+offset>8 ) return -ERANGE ;
    if ( OW_r_20page(p,8,((pn->extension)<<3)+offset,pn) ) return -EINVAL ;
    return FS_read_return(buf,size,offset,p,8) ;
}

/* 2450 A/D */
int FS_w_20page(const unsigned char *buf, const size_t size, const off_t offset , const struct parsedname * pn) {
    if ( size+offset>8 ) return -ERANGE ;
    if ( OW_w_20page(buf,size,((pn->extension)<<3)+offset,pn) ) return -EINVAL ;
    return 0 ;
}

/* 2450 A/D */
int FS_r_20mem(unsigned char *buf, const size_t size, const off_t offset , const struct parsedname * pn) {
    unsigned char p[8] ;

    if ( size+offset>4*8 ) return -ERANGE ;
    if ( OW_r_20page(p,size,offset,pn) ) return -EINVAL ;
    return FS_read_return(buf,size,offset,p,8) ;
}

/* 2450 A/D */
int FS_w_20mem(const unsigned char *buf, const size_t size, const off_t offset , const struct parsedname * pn) {
    if ( size+offset>4*8 ) return -ERANGE ;
    if ( OW_w_20page(buf,size,offset,pn) ) return -EINVAL ;
    return 0 ;
}

/* 2450 A/D */
int FS_20volts(float * V , const struct parsedname * pn) {
    if ( OW_20volts( V , pn ) ) return -EINVAL ;
    return 0 ;
}

/* read page from 2450 */
static int OW_r_20page( char * p , const int size, const int location , const struct parsedname * pn) {
    unsigned char buf[3+8+2] = {0xAA, location&0xFF,location>>8, } ;
    int s = size ;
    int rest = 8-(location&0x07);
    int ret ;

    BUS_lock() ;
        ret = BUS_select(pn) || BUS_send_data(buf,3) || BUS_readin_data(&buf[3],rest+2) || CRC16(buf,rest+5) ;
    BUS_unlock() ;
    if ( ret ) return 1 ;

    memcpy( p , &buf[3] , (size_t) rest ) ;

    for ( s-=rest; s>0; s-=8 ) {
        BUS_lock() ;
            ret = BUS_readin_data(&buf[3],8+2) || CRC16(&buf[3],8+2) ;
        BUS_unlock() ;
        if ( ret ) return 1 ;

        rest = (s>8) ? 8 : s ;
        memcpy( &p[size-s] , &buf[3] , (size_t) rest ) ;
    }
    return 0 ;
}

/* write to 2450 */
static int OW_w_20page( const char * p , const int size , const int location, const struct parsedname * pn) {
    // command, address(2) , data , crc(2), databack
    unsigned char buf[] = {0x55, location&0xFF,location>>8, p[0], 0xFF,0xFF, 0xFF, } ;
    int i ;
    int ret ;

    if ( size == 0 ) return 0 ;

    BUS_lock() ;
        ret = BUS_select(pn) || BUS_sendback_data(buf,buf,7) || CRC16(buf,6) || (buf[6]!=p[0]) ;
    BUS_unlock() ;
    if ( ret ) return 1 ;

    for ( i=2 ; i<size ; ++i ) {
        if ( buf[2] == 0xFF ) {
            buf[2] = 0 ;
            ++buf[1] ;
        } else {
            ++buf[2] ;
        }
        buf[3] = p[i] ;
        buf[4] = buf[5] = buf[6] = 0xFF ;

        BUS_lock() ;
            ret =BUS_sendback_data(&buf[3],&buf[3],4) || CRC16(&buf[1],5) || (buf[6]!=p[i]) ;
        BUS_unlock() ;
        if ( ret ) return 1 ;
    }

    BUS_lock() ;
        BUS_reset() ;
    BUS_unlock() ;
    return 0 ;
}

/* Read A/D from 2450 */
/* Note: Sets 16 bits resolution and all 4 channels */
static int OW_20volts( float * f , const struct parsedname * pn ) {
    unsigned char control[8] ;
    unsigned char data[8] ;
    unsigned char convert[] = { 0x3C , 0x0F , 0x00, 0xFF, 0xFF, } ;
    int ret ;

    // Get control registers and set to A2D 16 bits
    if ( OW_r_20page( control , 8, 1<<3, pn ) ) return 1 ;
    control[0] = 0x00 ; // 16bit, A/D
    control[1] &= 0x7F ; // Reset -- leave input range alone    control[0] = 0x00 ; // 16bit, A/D
    control[2] = 0x00 ; // 16bit, A/D
    control[3] &= 0x7F ; // Reset -- leave input range alone    control[0] = 0x00 ; // 16bit, A/D
    control[4] = 0x00 ; // 16bit, A/D
    control[5] &= 0x7F ; // Reset -- leave input range alone    control[0] = 0x00 ; // 16bit, A/D
    control[6] = 0x00 ; // 16bit, A/D
    control[7] &= 0x7F ; // Reset -- leave input range alone    control[0] = 0x00 ; // 16bit, A/D

    // Set control registers
    if ( OW_w_20page( control, 8, 1<<3, pn) ) return 1 ;

    // Start conversion
	// 6 msec for 16bytex4channel (5.2)
    BUS_lock() ;
        ret = BUS_sendback_data( convert , data , 5 ) || memcmp( convert , data , 3 ) || CRC16(data,6) || BUS_PowerByte(0x04, 6) ;
    BUS_unlock() ;
    if ( ret ) return 1 ;

    // read data
    BUS_lock() ;
        ret = BUS_readin_data(data,1) || (data[0]!=0xFF) ;
    BUS_unlock() ;
    if ( ret ) return 1 ;

    if ( OW_r_20page( data, 8, 0<<3, pn) ) return 1 ;

    // data conversions
    f[0] = ((control[1]&0x01)?7.8126192E-5:3.90630961E-5)*((((unsigned int)data[1])<<8)|data[0]) ;
    f[1] = ((control[3]&0x01)?7.8126192E-5:3.90630961E-5)*((((unsigned int)data[3])<<8)|data[2]) ;
    f[2] = ((control[5]&0x01)?7.8126192E-5:3.90630961E-5)*((((unsigned int)data[5])<<8)|data[4]) ;
    f[3] = ((control[7]&0x01)?7.8126192E-5:3.90630961E-5)*((((unsigned int)data[7])<<8)|data[6]) ;
    return 0 ;
}




