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
#include "ow_2502.h"

/* ------- Prototypes ----------- */

/* DS2423 counter */
 bREAD_FUNCTION( FS_r_09page ) ;
bWRITE_FUNCTION( FS_w_09page ) ;
 bREAD_FUNCTION( FS_r_09memory ) ;
bWRITE_FUNCTION( FS_w_09memory ) ;
 bREAD_FUNCTION( FS_r_89param ) ;

/* ------- Structures ----------- */

struct aggregate A2502 = { 4, ag_numbers, ag_separate, } ;
struct filetype DS2502[] = {
    F_STANDARD   ,
    {"memory"    ,   128,  NULL,   ft_binary, ft_stable  , {b:FS_r_09memory}  , {b:FS_w_09memory}, NULL, } ,
    {"page"      ,    32,  &A2502, ft_binary, ft_stable  , {b:FS_r_09page}    , {b:FS_w_09page}, NULL, } ,
} ;
DeviceEntry( 09, DS2502 )

struct filetype DS1982U[] = {
    F_STANDARD   ,
    {"mac_e"     ,     6,  NULL,   ft_binary, ft_stable  , {b:FS_r_89param}   , {v:NULL} , (void *) 4 } ,
    {"mac_fw"    ,     8,  NULL,   ft_binary, ft_stable  , {b:FS_r_89param}   , {v:NULL} , (void *) 4 } ,
    {"project"   ,     4,  NULL,   ft_binary, ft_stable  , {b:FS_r_89param}   , {v:NULL} , (void *) 0 } ,
    {"memory"    ,   128,  NULL,   ft_binary, ft_stable  , {b:FS_r_09memory}  , {b:FS_w_09memory}, NULL, } ,
    {"page"      ,    32,  &A2502, ft_binary, ft_stable  , {b:FS_r_09page}    , {b:FS_w_09page}, NULL, } ,
} ;
DeviceEntry( 89, DS1982U )

/* ------- Functions ------------ */

/* DS2502 */
static int OW_w_09mem( const unsigned char * data , const size_t location , const size_t length, const struct parsedname * pn ) ;
static int OW_r_09mem( unsigned char * data , const size_t location , const size_t length, const struct parsedname * pn ) ;
static int OW_r_89data( unsigned char * data , const struct parsedname * pn ) ;

/* 2502 memory */
int FS_r_09memory(unsigned char *buf, const size_t size, const off_t offset , const struct parsedname * pn) {
    int s = size ;
    if ( (int) (size+offset) > pn->ft->suglen ) s = pn->ft->suglen-offset ;
    if ( s<0 ) return -EMSGSIZE ;

    if ( OW_r_09mem( buf, (size_t) s, (size_t) offset, pn) ) return -EINVAL ;

    return s ;
}

int FS_r_09page(unsigned char *buf, const size_t size, const off_t offset , const struct parsedname * pn) {
    int s = size ;
    if ( (int) (size+offset) > pn->ft->suglen ) s = pn->ft->suglen-offset ;
    if ( s<0 ) return -EMSGSIZE ;

    if ( OW_r_09mem( buf,  (size_t) s,  (size_t) (offset+(pn->extension<<5)), pn) ) return -EINVAL ;

    return s ;
}

int FS_r_89param(unsigned char *buf, const size_t size, const off_t offset , const struct parsedname * pn) {
    unsigned char data[32] ;
    if (offset != 0 ) return -EFAULT ;
    if (size<pn->ft->suglen) return -EFAULT ;

    if ( OW_r_89data(data,pn) ) return -EINVAL ;
    memcpy( buf, &data[(int)pn->ft->data], pn->ft->suglen ) ;
    return pn->ft->suglen ;
}

int FS_w_09memory(const unsigned char *buf, const size_t size, const off_t offset , const struct parsedname * pn) {
    if ( OW_w_09mem(buf,size, (size_t) offset,pn) ) return -EINVAL ;
    return 0 ;
}

int FS_w_09page(const unsigned char *buf, const size_t size, const off_t offset , const struct parsedname * pn) {
    if ( OW_w_09mem(buf,size, (size_t) (offset+(pn->extension<<5)),pn) ) return -EINVAL ;
    return 0 ;
}

static int OW_w_09mem( const unsigned char * data , const size_t length, const size_t location , const struct parsedname * pn ) {
    unsigned char p[5] = { 0x0F, location&0xFF , location>>8, data[0] } ;
    size_t i ;
    int ret ;

    if (length>0) {
        BUS_lock() ;
            ret = BUS_select(pn) || BUS_send_data(p,4) || BUS_readin_data(&p[4],1) || CRC8(p,5) || BUS_ProgramPulse() || BUS_readin_data(&p[3],1) || (p[3]!=data[0]) ;
        BUS_unlock() ;
        if ( ret ) return 1 ;
    }
    p[0]=p[1] ;
    for ( i=1 ; i<length ; ++i ) {
        ++p[0] ;
        p[1] = data[i] ;

        BUS_lock() ;
            ret = BUS_send_data(&p[1],1) || BUS_readin_data(&p[2],1) || CRC8(p,3) || BUS_ProgramPulse() || BUS_readin_data(&p[1],1) || (p[1]!=data[i]) ;
        BUS_unlock() ;
        if ( ret ) return 1 ;
    }
    return 0 ;
}

static int OW_r_09mem( unsigned char * data , const size_t length, const size_t location , const struct parsedname * pn ) {
    unsigned char p[33] = { 0xC3, location&0xFF , location>>8, } ;

    int start = location ;
    int rest = 32 - (start & 0x1F) ;
    int ret ;

    BUS_lock() ;
        ret = BUS_select(pn) || BUS_send_data(p,3) || BUS_readin_data(&p[3],1) || CRC8(p,4) || BUS_readin_data(p,rest+1) || CRC8(p,rest+1) ;
    BUS_unlock() ;
    if ( ret ) return 1 ;

    memcpy( data, p, (size_t) rest ) ;

    for ( start+=rest ; (size_t)start<location+length ; start+=rest  ) {
        rest =  (location+length-start>32)?32:location+length-start ;

        BUS_lock() ;
            ret = BUS_readin_data(p,rest+1) || CRC8(p,rest+1) ;
        BUS_unlock() ;
        if ( ret ) return 1 ;

        memcpy( &data[start-location], p, (size_t) rest ) ;
    }
    return 0 ;
}

static int OW_r_89data( unsigned char * data , const struct parsedname * pn ) {
    unsigned char p[32] ;

    if ( OW_r_09mem(p,32,0,pn) || CRC16(p,3+p[0]) ) return 1 ;
    memcpy( data, &p[1], p[0] ) ;
    return 0 ;
}
