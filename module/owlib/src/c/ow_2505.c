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
#include "ow_2505.h"

/* ------- Prototypes ----------- */

/* DS2423 counter */
 bREAD_FUNCTION( FS_r_page ) ;
bWRITE_FUNCTION( FS_w_page ) ;
 bREAD_FUNCTION( FS_r_memory ) ;
bWRITE_FUNCTION( FS_w_memory ) ;

/* ------- Structures ----------- */

struct aggregate A2505 = { 64, ag_numbers, ag_separate, } ;
struct filetype DS2505[] = {
    F_STANDARD   ,
    {"memory"    ,  2048,  NULL,   ft_binary, ft_stable  , {b:FS_r_memory}  , {b:FS_w_memory}, NULL, } ,
    {"page"      ,    32,  &A2505, ft_binary, ft_stable  , {b:FS_r_page}    , {b:FS_w_page}, NULL, } ,
} ;
DeviceEntry( 0B, DS2505 )

struct filetype DS1985U[] = {
    F_STANDARD   ,
    {"memory"    ,  2048,  NULL,   ft_binary, ft_stable  , {b:FS_r_memory}  , {b:FS_w_memory}, NULL, } ,
    {"page"      ,    32,  &A2505, ft_binary, ft_stable  , {b:FS_r_page}    , {b:FS_w_page}, NULL, } ,
} ;
DeviceEntry( 8B, DS1985U )

struct aggregate A2506 = { 256, ag_numbers, ag_separate, } ;
struct filetype DS2506[] = {
    F_STANDARD   ,
    {"memory"    ,  8192,  &A2506, ft_binary, ft_stable  , {b:FS_r_memory}  , {b:FS_w_memory}, NULL, } ,
    {"page"      ,    32,  NULL,   ft_binary, ft_stable  , {b:FS_r_page}    , {b:FS_w_page}, NULL, } ,
} ;
DeviceEntry( 0F, DS2506 )

struct filetype DS1986U[] = {
    F_STANDARD   ,
    {"memory"    ,  8192,  &A2506, ft_binary, ft_stable  , {b:FS_r_memory}  , {b:FS_w_memory}, NULL, } ,
    {"page"      ,    32,  NULL,   ft_binary, ft_stable  , {b:FS_r_page}    , {b:FS_w_page}, NULL, } ,
} ;
DeviceEntry( 8F, DS1986U )

/* ------- Functions ------------ */

/* DS2505 */
static int OW_w_mem( const unsigned char * data , const size_t location , const size_t length, const struct parsedname * pn ) ;
static int OW_r_mem( unsigned char * data , const size_t location , const size_t length, const struct parsedname * pn ) ;

/* 2505 memory */
int FS_r_memory(unsigned char *buf, const size_t size, const off_t offset , const struct parsedname * pn) {
    int s = size ;
    if ( (int) (size+offset) > pn->ft->suglen ) s = pn->ft->suglen-offset ;
    if ( s<0 ) return -EMSGSIZE ;

    if ( OW_r_mem( buf, (size_t) s, (size_t) offset, pn) ) return -EINVAL ;

    return s ;
}

int FS_r_page(unsigned char *buf, const size_t size, const off_t offset , const struct parsedname * pn) {
    int s = size ;
    if ( (int) (size+offset) > pn->ft->suglen ) s = pn->ft->suglen-offset ;
    if ( s<0 ) return -EMSGSIZE ;

    if ( OW_r_mem( buf, (size_t) s, (size_t) (offset+(pn->extension<<5)), pn) ) return -EINVAL ;

    return s ;
}

int FS_w_memory(const unsigned char *buf, const size_t size, const off_t offset , const struct parsedname * pn) {
    if ( OW_w_mem(buf,size,(size_t) offset,pn) ) return -EINVAL ;
    return 0 ;
}

int FS_w_page(const unsigned char *buf, const size_t size, const off_t offset , const struct parsedname * pn) {
    if ( OW_w_mem(buf,size,(size_t) (offset+(pn->extension<<5)),pn) ) return -EINVAL ;
    return 0 ;
}

static int OW_w_mem( const unsigned char * data , const size_t length, const size_t location , const struct parsedname * pn ) {
    unsigned char p[6] = { 0x0F, location&0xFF , location>>8, data[0] } ;
    size_t i ;
    int ret ;

    if (length>0) {
        BUS_lock() ;
            ret = BUS_select(pn) || BUS_send_data(p,4) || BUS_readin_data(&p[4],2) || CRC16(p,6) || BUS_ProgramPulse() || BUS_readin_data(&p[3],1) || (p[3]!=data[0]);
        BUS_unlock() ;
        if ( ret ) return 1 ;
    }
    for ( i=1 ; i<length ; ++i ) {
        if ( (++p[1])==0) ++p[2] ;
        p[3] = data[i] ;

        BUS_lock() ;
            ret = BUS_send_data(&p[3],1) || BUS_readin_data(&p[4],2) || CRC16(&p[1],5) || BUS_ProgramPulse() || BUS_readin_data(&p[3],1) || (p[3]!=data[i]) ;
        BUS_unlock() ;
        if ( ret ) return 1 ;
    }
    return 0 ;
}

static int OW_r_mem( unsigned char * data , const size_t length, const size_t location , const struct parsedname * pn ) {
    unsigned char p[6] = { 0xA5, location&0xFF , location>>8, } ;
    unsigned char d[34] ;

    int start = location ;
    int rest = 32 - (start & 0x1F) ;
    int ret ;

    BUS_lock() ;
        ret =BUS_select(pn) || BUS_send_data(p,3) || BUS_readin_data(&p[3],3) || CRC16(p,6) || BUS_readin_data(d,rest+2) || CRC16(d,rest+2) ;
    BUS_unlock() ;
    if ( ret ) return 1 ;

    memcpy( data, d, (size_t) rest ) ;

    for ( start+=rest ; (size_t)start<location+length ; start+=rest  ) {
        BUS_lock() ;
            ret = BUS_readin_data(p,3) || CRC16(p,3) || BUS_readin_data(d,34) || CRC16(d,34) ;
        BUS_unlock() ;
        if ( ret ) return 1 ;

        rest =  (location+length-start>32)?32:location+length-start ;
        memcpy( &data[start-location], d, (size_t) rest ) ;
    }

    return 0 ;
}





