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
 bREAD_FUNCTION( FS_r_page ) ;
bWRITE_FUNCTION( FS_w_page ) ;
 bREAD_FUNCTION( FS_r_memory ) ;
bWRITE_FUNCTION( FS_w_memory ) ;
 bREAD_FUNCTION( FS_r_param ) ;

/* ------- Structures ----------- */

struct aggregate A2502 = { 4, ag_numbers, ag_separate, } ;
struct filetype DS2502[] = {
    F_STANDARD   ,
    {"memory"    ,   128,  NULL,   ft_binary, ft_stable  , {b:FS_r_memory}, {b:FS_w_memory}, NULL, } ,
    {"pages"     ,     0,  NULL,   ft_subdir, ft_volatile, {v:NULL}       , {v:NULL}       , NULL, } ,
    {"pages/page",    32,  &A2502, ft_binary, ft_stable  , {b:FS_r_page}  , {b:FS_w_page}  , NULL, } ,
} ;
DeviceEntry( 09, DS2502 )

struct filetype DS1982U[] = {
    F_STANDARD   ,
    {"mac_e"     ,     6,  NULL,   ft_binary, ft_stable  , {b:FS_r_param} , {v:NULL}       , (void *) 4, } ,
    {"mac_fw"    ,     8,  NULL,   ft_binary, ft_stable  , {b:FS_r_param} , {v:NULL}       , (void *) 4, } ,
    {"project"   ,     4,  NULL,   ft_binary, ft_stable  , {b:FS_r_param} , {v:NULL}       , (void *) 0, } ,
    {"memory"    ,   128,  NULL,   ft_binary, ft_stable  , {b:FS_r_memory}, {b:FS_w_memory}, NULL, } ,
    {"pages"     ,     0,  NULL,   ft_subdir, ft_volatile, {v:NULL}       , {v:NULL}       , NULL, } ,
    {"pages/page",    32,  &A2502, ft_binary, ft_stable  , {b:FS_r_page}  , {b:FS_w_page}  , NULL, } ,
} ;
DeviceEntry( 89, DS1982U )

/* ------- Functions ------------ */

/* DS2502 */
static int OW_w_mem( const unsigned char * data , const size_t size , const size_t offset, const struct parsedname * pn ) ;
static int OW_r_mem( unsigned char * data , const size_t size , const size_t offset, const struct parsedname * pn ) ;
static int OW_r_data( unsigned char * data , const struct parsedname * pn ) ;

/* 2502 memory */
static int FS_r_memory(unsigned char *buf, const size_t size, const off_t offset , const struct parsedname * pn) {
//    if ( OW_r_mem( buf, size, (size_t) offset, pn) ) return -EINVAL ;
    if ( OW_read_paged( buf, size, (size_t) offset, pn, 32, OW_r_mem) ) return -EINVAL ;
    return size ;
}

static int FS_r_page(unsigned char *buf, const size_t size, const off_t offset , const struct parsedname * pn) {
    if ( OW_r_mem( buf, size, (offset+(pn->extension<<5)), pn) ) return -EINVAL ;
    return size ;
}

static int FS_r_param(unsigned char *buf, const size_t size, const off_t offset , const struct parsedname * pn) {
    unsigned char data[32] ;
    if ( OW_r_data(data,pn) ) return -EINVAL ;
    memcpy( buf, &data[(int)pn->ft->data+offset], size ) ;
    return size ;
}

static int FS_w_memory(const unsigned char *buf, const size_t size, const off_t offset , const struct parsedname * pn) {
    if ( OW_w_mem(buf,size, (size_t) offset,pn) ) return -EINVAL ;
    return 0 ;
}

static int FS_w_page(const unsigned char *buf, const size_t size, const off_t offset , const struct parsedname * pn) {
    if ( OW_w_mem(buf,size, (size_t) (offset+(pn->extension<<5)),pn) ) return -EINVAL ;
    return 0 ;
}

/* Byte-oriented write */
static int OW_w_mem( const unsigned char * data , const size_t size, const size_t offset , const struct parsedname * pn ) {
    unsigned char p[5] = { 0x0F, offset&0xFF , offset>>8, data[0] } ;
    size_t i ;
    int ret ;

    if (size>0) {
        /* First byte */
        BUSLOCK
            ret = BUS_select(pn) || BUS_send_data(p,4) || BUS_readin_data(&p[4],1) || CRC8(p,5) || BUS_ProgramPulse() || BUS_readin_data(&p[3],1) || (p[3]!=data[0]) ;
        BUSUNLOCK
        if ( ret ) return 1 ;

        /* Successive bytes */
        for ( i=1 ; i<size ; ++i ) {
            BUSLOCK
                ret = BUS_send_data(&data[i],1) || BUS_readin_data(p,1) || CRC8seeded(p,1,(offset+i)&0xFF) || BUS_ProgramPulse() || BUS_readin_data(p,1) || (p[0]!=data[i]) ;
            BUSUNLOCK
            if ( ret ) return 1 ;
        }
    }

    return 0 ;
}

/* page-oriented read -- call will not span page boundaries */
static int OW_r_mem( unsigned char * data , const size_t size, const size_t offset , const struct parsedname * pn ) {
    unsigned char p[33] = { 0xC3, offset&0xFF , offset>>8, } ;
    int rest = 32 - (offset & 0x1F) ;
    int ret ;

    BUSLOCK
        ret = BUS_select(pn) || BUS_send_data(p,3) || BUS_readin_data(&p[3],1) || CRC8(p,4) || BUS_readin_data(p,rest+1) || CRC8(p,rest+1) ;
    BUSUNLOCK
    if ( ret ) return 1 ;

    memcpy( data, p, size ) ;

    return 0 ;
}

static int OW_r_data( unsigned char * data , const struct parsedname * pn ) {
    unsigned char p[32] ;

    if ( OW_r_mem(p,32,0,pn) || CRC16(p,3+p[0]) ) return 1 ;
    memcpy( data, &p[1], p[0] ) ;
    return 0 ;
}
