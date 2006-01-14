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
#include "ow_2404.h"

/* ------- Prototypes ----------- */

/* DS1902 ibutton memory */
 bREAD_FUNCTION( FS_r_page ) ;
bWRITE_FUNCTION( FS_w_page ) ;
 bREAD_FUNCTION( FS_r_memory ) ;
bWRITE_FUNCTION( FS_w_memory ) ;
 uREAD_FUNCTION( FS_r_counter ) ;
uWRITE_FUNCTION( FS_w_counter ) ;
 dREAD_FUNCTION( FS_r_date ) ;
dWRITE_FUNCTION( FS_w_date ) ;
 yREAD_FUNCTION( FS_r_flag ) ;
yWRITE_FUNCTION( FS_w_flag ) ;

/* ------- Structures ----------- */

struct aggregate A2404 = { 16, ag_numbers, ag_separate, } ;
struct filetype DS2404[] = {
    F_STANDARD          ,
    {"pages"            ,     0,  NULL,   ft_subdir  , ft_volatile, {v:NULL}       , {v:NULL}       , NULL, } ,
    {"pages/page"       ,    32,  &A2404, ft_binary  , ft_stable  , {b:FS_r_page}  , {b:FS_w_page}  , NULL, } ,
    {"memory"           ,   512,  NULL,   ft_binary  , ft_stable, {b:FS_r_memory} , {b:FS_w_memory} , NULL, } ,
    {"running"          ,     1,  NULL,   ft_yesno   , ft_stable, {y:FS_r_flag}    , {y:FS_w_flag}  , (void *) (unsigned char) 0x10, } ,
    {"udate"            ,    12,  NULL,   ft_unsigned, ft_second, {u:FS_r_counter}, {u:FS_w_counter}, NULL, } ,
    {"date"             ,    24,  NULL,   ft_date    , ft_second, {d:FS_r_date}   , {d:FS_w_date}   , NULL, } ,
    {"readonly"         ,     0,  NULL,   ft_subdir  , ft_volatile, {v:NULL}       , {v:NULL}       , NULL, } ,
    {"readonly/memory"  ,     1,  NULL,   ft_yesno   , ft_stable, {y:FS_r_flag}    , {y:FS_w_flag}  , (void *) (unsigned char) 0x08, } ,
    {"readonly/cycle"   ,     1,  NULL,   ft_yesno   , ft_stable, {y:FS_r_flag}    , {y:FS_w_flag}  , (void *) (unsigned char) 0x04, } ,
    {"readonly/interval",     1,  NULL,   ft_yesno   , ft_stable, {y:FS_r_flag}    , {y:FS_w_flag}  , (void *) (unsigned char) 0x02, } ,
    {"readonly/clock"   ,     1,  NULL,   ft_yesno   , ft_stable, {y:FS_r_flag}    , {y:FS_w_flag}  , (void *) (unsigned char) 0x01, } ,

} ;
DeviceEntryExtended( 04, DS2404 , DEV_alarm ) ;

struct filetype DS2404S[] = {
    F_STANDARD          ,
    {"pages"            ,     0,  NULL,   ft_subdir  , ft_volatile, {v:NULL}       , {v:NULL}       , NULL, } ,
    {"pages/page"       ,    32,  &A2404, ft_binary  , ft_stable  , {b:FS_r_page}  , {b:FS_w_page}  , NULL, } ,
    {"memory"           ,   512,  NULL,   ft_binary  , ft_stable, {b:FS_r_memory} , {b:FS_w_memory} , NULL, } ,
    {"running"          ,     1,  NULL,   ft_yesno   , ft_stable, {y:FS_r_flag}    , {y:FS_w_flag}  , (void *) (unsigned char) 0x10, } ,
    {"udate"            ,    12,  NULL,   ft_unsigned, ft_second, {u:FS_r_counter}, {u:FS_w_counter}, NULL, } ,
    {"date"             ,    24,  NULL,   ft_date    , ft_second, {d:FS_r_date}   , {d:FS_w_date}   , NULL, } ,
    {"readonly"         ,     0,  NULL,   ft_subdir  , ft_volatile, {v:NULL}       , {v:NULL}       , NULL, } ,
    {"readonly/memory"  ,     1,  NULL,   ft_yesno   , ft_stable, {y:FS_r_flag}    , {y:FS_w_flag}  , (void *) (unsigned char) 0x08, } ,
    {"readonly/cycle"   ,     1,  NULL,   ft_yesno   , ft_stable, {y:FS_r_flag}    , {y:FS_w_flag}  , (void *) (unsigned char) 0x04, } ,
    {"readonly/interval",     1,  NULL,   ft_yesno   , ft_stable, {y:FS_r_flag}    , {y:FS_w_flag}  , (void *) (unsigned char) 0x02, } ,
    {"readonly/clock"   ,     1,  NULL,   ft_yesno   , ft_stable, {y:FS_r_flag}    , {y:FS_w_flag}  , (void *) (unsigned char) 0x01, } ,

} ;
DeviceEntryExtended( 84, DS2404S  , DEV_alarm ) ;

/* ------- Functions ------------ */

/* DS1902 */
static int OW_w_mem( const unsigned char * data , const size_t size , const size_t offset, const struct parsedname * pn ) ;
static int OW_r_mem( unsigned char * data, const size_t size, const size_t offset, const struct parsedname * pn ) ;

/* 1902 */
static int FS_r_page(unsigned char *buf, const size_t size, const off_t offset , const struct parsedname * pn) {
    if ( OW_r_mem( buf, size, (size_t) (offset+((pn->extension)<<5)), pn) ) return -EINVAL ;
    return size ;
}

static int FS_r_memory(unsigned char *buf, const size_t size, const off_t offset , const struct parsedname * pn) {
    /* read is consecutive, unchecked. No paging */
    if ( OW_r_mem( buf, size, (size_t) offset, pn) ) return -EINVAL ;
    return size ;
}

static int FS_w_page(const unsigned char *buf, const size_t size, const off_t offset , const struct parsedname * pn) {
    if ( OW_w_mem( buf, size, (size_t) (offset+((pn->extension)<<5)), pn) ) return -EFAULT ;
    return 0 ;
}

static int FS_w_memory( const unsigned char *buf, const size_t size, const off_t offset , const struct parsedname * pn) {
    /* paged write */
//    if ( OW_w_mem( buf, size, (size_t) offset, pn) ) return -EFAULT ;
    if ( OW_write_paged( buf, size, (size_t) offset, pn, 32, OW_w_mem ) ) return -EFAULT ;
    return 0 ;
}

/* set clock */
static int FS_w_date(const DATE * d , const struct parsedname * pn) {
    unsigned char data[5] ;

    data[0] = 0x00 ;
    data[1] = d[0] & 0xFF ;
    data[2] = (d[0]>>8) & 0xFF ;
    data[3] = (d[0]>>16) & 0xFF ;
    data[4] = (d[0]>>24) & 0xFF ;
    if ( OW_w_mem( data, 5, 0x0202, pn) ) return -EFAULT ;
    return 0 ;
}

/* read clock */
static int FS_r_date( DATE * d , const struct parsedname * pn) {
    unsigned char data[5] ;

    if ( OW_r_mem(data,5,0x0202,pn) ) return -EINVAL ;
    d[0] = ((unsigned int) data[1]) || ((unsigned int) data[2])<<8 || ((unsigned int) data[3])<<16 || ((unsigned int) data[4])<<24 ;
    if ( data[0] & 0x80 ) ++d[0] ;
    return 0 ;
}

/* set clock */
static int FS_w_counter(const unsigned int *u, const struct parsedname * pn) {
    unsigned char data[5] ;

    data[0] = 0 ;
    data[1] = (u[0]>>0) & 0xFF ;
    data[2] = (u[0]>>8) & 0xFF ;
    data[3] = (u[0]>>16) & 0xFF ;
    data[4] = (u[0]>>24) & 0xFF ;
    if ( OW_w_mem( data, 5, 0x0202, pn) ) return -EFAULT ;
    return 0 ;
}
/* read clock */
static int FS_r_counter(unsigned int *u, const struct parsedname * pn) {
    unsigned char data[5] ;
    if ( OW_r_mem(data,5,0x0202,pn) ) return -EINVAL ;
    u[0] = ((unsigned int) data[1]) || ((unsigned int) data[2])<<8 || ((unsigned int) data[3])<<16 || ((unsigned int) data[4])<<24 ;
    if ( data[0] & 0x80 ) ++u[0] ;
    return 0 ;
}

/* write flag */
static int FS_w_flag(const int * y , const struct parsedname * pn) {
    unsigned char cr ;
    unsigned char fl = (unsigned char) pn->ft->data ;
    if ( OW_r_mem( &cr, 1, 0x0201, pn) ) return -EINVAL ;
    if ( y[0] ) {
        if ( cr & fl ) return 0 ;
    } else {
        if ( (cr & fl) == 0 ) return 0 ;
    }
    cr ^= fl ; /* flip the bit */
    if ( OW_w_mem( &cr, 1, 0x0201, pn) ) return -EINVAL ;
    return 0 ;
}

/* read flag */
static int FS_r_flag(int * y , const struct parsedname * pn) {
    unsigned char cr ;
    unsigned char fl = (unsigned char) pn->ft->data ;
    if ( OW_r_mem( &cr, 1, 0x0201, pn) ) return -EINVAL ;
    y[0] = (cr&fl)?1:0 ;
    return 0 ;
}

/* PAged access -- pre-screened */
static int OW_w_mem( const unsigned char * data , const size_t size , const size_t offset, const struct parsedname * pn ) {
    unsigned char p[4+32] = { 0x0F, offset&0xFF , offset>>8, } ;
    int ret ;

    /* Copy to scratchpad */
    BUSLOCK(pn);
        ret = BUS_select(pn) || BUS_send_data(p,3,pn) || BUS_send_data(data,size,pn) ;
    BUSUNLOCK(pn);
    if ( ret ) return 1 ;

    /* Re-read scratchpad and compare */
    p[0] = 0xAA ;
    BUSLOCK(pn);
        ret = BUS_select(pn) || BUS_send_data(p,1,pn) || BUS_readin_data(&p[1],3+size,pn) || memcmp(&p[4], data, size) ;
    BUSUNLOCK(pn);
    if ( ret ) return 1 ;

    /* Copy Scratchpad to SRAM */
    p[0] = 0x55 ;
    BUSLOCK(pn);
        ret = BUS_select(pn) || BUS_send_data(p,4,pn) ;
    BUSUNLOCK(pn);
    if ( ret ) return 1 ;

    UT_delay(32) ;
    return 0 ;
}

static int OW_r_mem( unsigned char * data, const size_t size, const size_t offset, const struct parsedname * pn ) {
    unsigned char p[3] = { 0xF0, offset&0xFF , offset>>8, } ;
    int ret ;

    BUSLOCK(pn);
        ret = BUS_select(pn) || BUS_send_data(p, 3,pn) || BUS_readin_data(data, (int) size,pn) ;
    BUSUNLOCK(pn);
    return ret ;

}
