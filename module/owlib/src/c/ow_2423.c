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
#include "ow_2423.h"

/* ------- Prototypes ----------- */

/* DS2423 counter */
 bREAD_FUNCTION( FS_r_1Dpage ) ;
bWRITE_FUNCTION( FS_w_1Dpage ) ;
 uREAD_FUNCTION( FS_1Dcounter ) ;
 uREAD_FUNCTION( FS_1Dpagecount ) ;

/* ------- Structures ----------- */

struct aggregate A2423 = { 16, ag_numbers, ag_separate,} ;
struct aggregate A2423c = { 2, ag_letters, ag_separate,} ;
struct filetype DS2423[] = {
    F_STANDARD   ,
    {"page"      ,    32,  &A2423,  ft_binary , ft_stable  , {b:FS_r_1Dpage}   , {b:FS_w_1Dpage}, NULL, } ,
    {"counters"  ,    32,  &A2423c, ft_unsigned,ft_volatile, {u:FS_1Dcounter}  , {v:NULL}, NULL, } ,
    {"pagecount" ,    32,  &A2423,  ft_unsigned,ft_volatile, {u:FS_1Dpagecount}  , {v:NULL}, NULL, } ,
} ;
DeviceEntry( 1D, DS2423 )

/* ------- Functions ------------ */

/* DS2423 */
static int OW_w_1Dmem( const unsigned char * data , const int length , const int location, const struct parsedname * pn ) ;
static int OW_r_1Dmem( unsigned char * data , const int length , const int location, const struct parsedname * pn ) ;
static int OW_1Dcounter( unsigned int * counter , const int page, const struct parsedname * pn ) ;

/* 2423A/D Counter */
int FS_r_1Dpage(unsigned char *buf, const size_t size, const off_t offset , const struct parsedname * pn) {
    int len = size ;
    if ( (offset&0x1F)+size>32 ) len = 32-offset ;
    if ( OW_r_1Dmem( buf, len, offset+((pn->extension)<<5), pn) ) return -EINVAL ;

    return len ;
}

int FS_w_1Dpage(const unsigned char *buf, const size_t size, const off_t offset , const struct parsedname * pn) {
    return OW_w_1Dmem( buf, (int)size, offset+((pn->extension)<<5), pn) ;
}

int FS_1Dcounter(unsigned int * u , const struct parsedname * pn) {
    if ( OW_1Dcounter( u , (pn->extension)+14,  pn ) ) return -EINVAL ;
    return 0 ;
}

int FS_1Dpagecount(unsigned int * u , const struct parsedname * pn) {
    if ( OW_1Dcounter( u , pn->extension,  pn ) ) return -EINVAL ;
    return 0 ;
}

static int OW_w_1Dmem( const unsigned char * data , const int length , const int location, const struct parsedname * pn ) {
    unsigned char p[1+2+32+2] = { 0x0F, location&0xFF , location>>8, } ;
    int offset = location & 0x1F ;
    int rest = 32-offset ;
    int ret ;

    /* die if too big, split across pages */
    if ( location + length > 0x1FF ) return 1;
    if ( offset+length > 32 ) return OW_w_1Dmem( data, rest, location, pn ) || OW_w_1Dmem( &data[rest], length-rest, location+rest, pn ) ;
    /* Copy to scratchpad */
    memcpy( &p[3], data, (size_t) length ) ;

    BUS_lock() ;
        ret = (length==0) || BUS_select(pn) || BUS_send_data(p,length+3) || BUS_readin_data(&p[length+3],2) || CRC16(p,1+2+length+2) ;
    BUS_unlock() ;
    if ( ret ) return 1 ;

    /* Re-read scratchpad and compare */
    p[0] = 0xAA ;
    BUS_lock() ;
        ret = BUS_select(pn) || BUS_send_data(p,1) || BUS_readin_data(&p[1],3+length) || memcmp( &p[4], data, (size_t) length) ;
    BUS_unlock() ;
    if ( ret ) return 1 ;

    /* Copy Scratchpad to SRAM */
    p[0] = 0x5A ;
    BUS_lock() ;
        ret = BUS_select(pn) || BUS_send_data(p,4) ;
    BUS_unlock() ;
    if ( ret ) return 1 ;

    UT_delay(32) ;
    return 0 ;
}

static int OW_r_1Dmem( unsigned char * data , const int length , const int location, const struct parsedname * pn ) {
    unsigned char p[3] = { 0xF0, location&0xFF , location>>8, } ;
    int ret ;

    BUS_lock() ;
        ret = (location+length>0x1FF) || BUS_select(pn) || BUS_send_data( p , 3 ) || BUS_readin_data( data, length ) ;
    BUS_unlock() ;
    return ret ;
}

static int OW_1Dcounter( unsigned int * counter , const int page, const struct parsedname * pn ) {
    ssize_t location = (page<<5)+31;
    unsigned char p[1+2+1+10] = { 0xA5, location&0xFF , location>>8, } ;
    int ret ;

    BUS_lock() ;
        ret = BUS_select(pn) || BUS_send_data(p,3) || BUS_readin_data(&p[3],1+10) || CRC16(p,1+2+1+10) ;
    BUS_unlock() ;
    if ( ret ) return 1 ;

    *counter = (((((((unsigned int) p[7])<<8)|p[6])<<8)|p[5])<<8)|p[4] ;
    return 0 ;
}




