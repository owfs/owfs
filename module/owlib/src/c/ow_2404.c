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
 aREAD_FUNCTION( FS_r_date ) ;
aWRITE_FUNCTION( FS_w_date ) ;
 yREAD_FUNCTION( FS_r_run ) ;
yWRITE_FUNCTION( FS_w_run ) ;

/* ------- Structures ----------- */

struct aggregate A2404 = { 16, ag_numbers, ag_separate, } ;
struct filetype DS2404[] = {
    F_STANDARD   ,
    {"page"      ,    32,  &A2404, ft_binary  , ft_stable, {b:FS_r_page}   , {b:FS_w_page}   , NULL, } ,
    {"memory"    ,   512,  NULL,   ft_binary  , ft_stable, {b:FS_r_memory} , {b:FS_w_memory} , NULL, } ,
    {"running"   ,     1,  NULL,   ft_yesno   , ft_stable, {y:FS_r_run}    , {y:FS_w_run}    , NULL, } ,
    {"counter"   ,    12,  NULL,   ft_unsigned, ft_second, {u:FS_r_counter}, {u:FS_w_counter}, NULL, } ,
    {"date"      ,    26,  NULL,   ft_ascii   , ft_second, {a:FS_r_date}   , {a:FS_w_date}   , NULL, } ,
} ;
DeviceEntry( 04, DS2404 )

struct filetype DS2404S[] = {
    F_STANDARD   ,
    {"page"      ,    32,  &A2404, ft_binary  , ft_stable, {b:FS_r_page}   , {b:FS_w_page}   , NULL, } ,
    {"memory"    ,   512,  NULL,   ft_binary  , ft_stable, {b:FS_r_memory} , {b:FS_w_memory} , NULL, } ,
    {"running"   ,     1,  NULL,   ft_yesno   , ft_stable, {y:FS_r_run}    , {y:FS_w_run}    , NULL, } ,
    {"counter"   ,    12,  NULL,   ft_unsigned, ft_second, {u:FS_r_counter}, {u:FS_w_counter}, NULL, } ,
    {"date"      ,    26,  NULL,   ft_ascii   , ft_second, {a:FS_r_date}   , {a:FS_w_date}   , NULL, } ,
} ;
DeviceEntry( 84, DS2404S )

/* ------- Functions ------------ */

/* DS1902 */
static int OW_w_mem( const unsigned char * data , const size_t length , const size_t location, const struct parsedname * pn ) ;
static int OW_r_mem( unsigned char * data, const size_t length, const size_t location, const struct parsedname * pn ) ;

/* 1902 */
int FS_r_page(unsigned char *buf, const size_t size, const off_t offset , const struct parsedname * pn) {
    size_t len = size ;
    if ( (offset&0x1F)+size>32 ) len = 32-offset ;
    if ( OW_r_mem( buf, len, (size_t) (offset+((pn->extension)<<5)), pn) ) return -EINVAL ;

    return len ;
}

int FS_r_memory(unsigned char *buf, const size_t size, const off_t offset , const struct parsedname * pn) {
    int len = pn->ft->suglen - offset ;
    if ( len < 0 ) return -ERANGE ;
    if ( (size_t)len > size ) len = size ;
    if ( OW_r_mem( buf, (size_t) len, (size_t) offset, pn) ) return -EINVAL ;
    return len ;
}

int FS_w_page(const unsigned char *buf, const size_t size, const off_t offset , const struct parsedname * pn) {
    if ( OW_w_mem( buf, size, (size_t) (offset+((pn->extension)<<5)), pn) ) return -EFAULT ;
    return 0 ;
}

int FS_w_memory( const unsigned char *buf, const size_t size, const off_t offset , const struct parsedname * pn) {
    int len = pn->ft->suglen - offset ;
    if ( len < 0 ) return -ERANGE ;
    if ( (size_t)len > size ) len = size ;
    if ( OW_w_mem( buf, (size_t) len, (size_t) offset, pn) ) return -EFAULT ;
    return 0 ;
}

/* set clock */
int FS_w_date(const char *buf, const size_t size, const off_t offset , const struct parsedname * pn) {
    struct tm tm ;
    unsigned int t ;
    unsigned char data[5] ;

    /* Not sure if this is valid, but won't allow offset != 0 at first */
    /* otherwise need a buffer */
    if ( offset != 0 ) return -EFAULT ;
    if ( size==0 || buf[0]=='\0' ) {
        t = (unsigned int) time(NULL) ;
    } else if ( strptime(buf,"%a %b %d %T %Y",&tm) || strptime(buf,"%b %d %T %Y",&tm) || strptime(buf,"%c",&tm) || strptime(buf,"%D %T",&tm) ) {
        t = (unsigned int) mktime(&tm) ;
    } else {
        return -EINVAL ;
    }
    data[0] = 0x00 ;
    data[1] = t & 0xFF ;
    data[2] = (t>>8) & 0xFF ;
    data[3] = (t>>16) & 0xFF ;
    data[4] = (t>>24) & 0xFF ;
    if ( OW_w_mem( data, 5, 0x0202, pn) ) return -EFAULT ;
    return 0 ;
}
/* read clock */
int FS_r_date(char *buf, const size_t size, const off_t offset , const struct parsedname * pn) {
    unsigned int t ; /* must me 32bit (or more) */
    unsigned char data[5] ;
    if ( offset ) return -EFAULT ;
    if ( size<26 ) return -EMSGSIZE ;

    if ( OW_r_mem(data,5,0x0202,pn) ) return -EINVAL ;
    t = ((unsigned int) data[1]) || ((unsigned int) data[2])<<8 || ((unsigned int) data[3])<<16 || ((unsigned int) data[4])<<24 ;
    if ( data[0] & 0x80 ) ++t ;
    if ( ctime_r((time_t *) &t,buf) ) return 24 ;
    return -EINVAL ;
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

/* write running */
static int FS_w_run(const int * y , const struct parsedname * pn) {
    unsigned char cr ;
    if ( OW_r_mem( &cr, 1, 0x0201, pn) ) return -EINVAL ;
    if ( y[0] ) {
        if ( cr & 0x10 ) return 0 ;
        cr &= 0x10 ;
    } else {
        if ( (cr & 0x10) == 0 ) return 0 ;
        cr |= 0xEF ;
    }
    if ( OW_w_mem( &cr, 1, 0x0201, pn) ) return -EINVAL ;
    return 0 ;
}

/* read running */
static int FS_r_run(int * y , const struct parsedname * pn) {
    unsigned char cr ;
    if ( OW_r_mem( &cr, 1, 0x0201, pn) ) return -EINVAL ;
    y[0] = (cr&0x10)?1:0 ;
    return 0 ;
}

static int OW_w_mem( const unsigned char * data , const size_t length , const size_t location, const struct parsedname * pn ) {
    unsigned char p[4+32] = { 0x0F, location&0xFF , location>>8, } ;
    int offset = location & 0x1F ;
    int rest = 32-offset ;
    int ret ;

    if ( offset+length > 32 ) return OW_w_mem( data, (size_t) rest, location, pn) || OW_w_mem( &data[rest], length-rest, location+rest, pn) ;
    if ( (size_t)rest>length ) rest = length ;

    /* Copy to scratchpad */
    BUS_lock() ;
        ret = BUS_select(pn) || BUS_send_data(p,3) || BUS_send_data(data,rest) ;
    BUS_unlock() ;
    if ( ret ) return 1 ;

    /* Re-read scratchpad and compare */
    p[0] = 0xAA ;
    BUS_lock() ;
        ret = BUS_select(pn) || BUS_send_data(p,1) || BUS_readin_data(&p[1],3+rest) || memcmp(&p[4], data, (size_t) rest) ;
    BUS_unlock() ;
    if ( ret ) return 1 ;

    /* Copy Scratchpad to SRAM */
    p[0] = 0x55 ;
    BUS_lock() ;
        ret = BUS_select(pn) || BUS_send_data(p,4) ;
    BUS_unlock() ;
    if ( ret ) return 1 ;

    UT_delay(32) ;
    return 0 ;
}

static int OW_r_mem( unsigned char * data, const size_t length, const size_t location, const struct parsedname * pn ) {
    unsigned char p[3] = { 0xF0, location&0xFF , location>>8, } ;
    int ret ;

    BUS_lock() ;
        ret = BUS_select(pn) || BUS_send_data( p, 3) || BUS_readin_data( data, (int) length) ;
    BUS_unlock() ;
    return ret ;

}
