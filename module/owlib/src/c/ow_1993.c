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
#include "ow_1993.h"

/* ------- Prototypes ----------- */

/* DS1902 ibutton memory */
 bREAD_FUNCTION( FS_r_page ) ;
bWRITE_FUNCTION( FS_w_page ) ;
 bREAD_FUNCTION( FS_r_memory ) ;
bWRITE_FUNCTION( FS_w_memory ) ;

/* ------- Structures ----------- */

struct aggregate A1992 = { 4, ag_numbers, ag_separate, } ;
struct filetype DS1992[] = {
    F_STANDARD   ,
    {"pages"     ,     0,  NULL,   ft_subdir, ft_volatile, {v:NULL}       , {v:NULL}       , NULL, } ,
    {"pages/page",    32,  &A1992, ft_binary, ft_stable  , {b:FS_r_page}   , {b:FS_w_page}, NULL, } ,
    {"memory"    ,   128,  NULL, ft_binary, ft_stable  , {b:FS_r_memory} , {b:FS_w_memory}, NULL, } ,
} ;
DeviceEntry( 08, DS1992 )

struct aggregate A1993 = { 16, ag_numbers, ag_separate, } ;
struct filetype DS1993[] = {
    F_STANDARD   ,
    {"pages"     ,     0,  NULL,   ft_subdir, ft_volatile, {v:NULL}       , {v:NULL}       , NULL, } ,
    {"pages/page",    32,  &A1993, ft_binary, ft_stable  , {b:FS_r_page}   , {b:FS_w_page}, NULL, } ,
    {"memory"    ,   512,  NULL, ft_binary, ft_stable  , {b:FS_r_memory} , {b:FS_w_memory}, NULL, } ,
} ;
DeviceEntry( 06, DS1993 )

struct aggregate A1995 = { 64, ag_numbers, ag_separate, } ;
struct filetype DS1995[] = {
    F_STANDARD   ,
    {"pages"     ,     0,  NULL,   ft_subdir, ft_volatile, {v:NULL}       , {v:NULL}       , NULL, } ,
    {"pages/page",    32,  &A1995, ft_binary, ft_stable  , {b:FS_r_page}   , {b:FS_w_page}, NULL, } ,
    {"memory"    ,  2048,  NULL, ft_binary, ft_stable  , {b:FS_r_memory} , {b:FS_w_memory}, NULL, } ,
} ;
DeviceEntry( 0A, DS1995 )

struct aggregate A1996 = { 256, ag_numbers, ag_separate, } ;
struct filetype DS1996[] = {
    F_STANDARD   ,
    {"pages"     ,     0,  NULL,   ft_subdir, ft_volatile, {v:NULL}       , {v:NULL}       , NULL, } ,
    {"pages/page",    32,  &A1996, ft_binary, ft_stable  , {b:FS_r_page}   , {b:FS_w_page}, NULL, } ,
    {"memory"    ,  8192,  NULL, ft_binary, ft_stable  , {b:FS_r_memory} , {b:FS_w_memory}, NULL, } ,
} ;
DeviceEntry( 0C, DS1996 )

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
    /* read is not page-limited */
    if ( OW_r_mem( buf, size, (size_t) offset, pn) ) return -EINVAL ;
    return size ;
}

static int FS_w_page(const unsigned char *buf, const size_t size, const off_t offset , const struct parsedname * pn) {
    if ( OW_w_mem( buf, size, (size_t) (offset+((pn->extension)<<5)), pn) ) return -EFAULT ;
    return 0 ;
}

static int FS_w_memory( const unsigned char *buf, const size_t size, const off_t offset , const struct parsedname * pn) {
    /* paged access */
//    if ( OW_w_mem( buf, size, (size_t) offset, pn) ) return -EFAULT ;
    if ( OW_write_paged( buf, size, offset, pn, 32, OW_w_mem) ) return -EFAULT ;
    return 0 ;
}

/* paged, and pre-screened */
static int OW_w_mem( const unsigned char * data , const size_t size , const size_t offset, const struct parsedname * pn ) {
    unsigned char p[4+32] = { 0x0F, offset&0xFF , offset>>8, } ;
    int ret ;

    /* Copy to scratchpad */
    BUSLOCK(pn)
        ret = BUS_select(pn) || BUS_send_data( p,3,pn) || BUS_send_data(data,size,pn) ;
    BUSUNLOCK(pn)
    if ( ret ) return 1 ;

    /* Re-read scratchpad and compare */
    p[0] = 0xAA ;
    BUSLOCK(pn)
        ret = BUS_select(pn) || BUS_send_data( p,1,pn) || BUS_readin_data(&p[1],3+size,pn) || memcmp(&p[4], data, (size_t) size) ;
    BUSUNLOCK(pn)
    if ( ret ) return 1 ;

    /* Copy Scratchpad to SRAM */
    p[0] = 0x55 ;
    BUSLOCK(pn)
        ret = BUS_select(pn) || BUS_send_data(p,4,pn) ;
    BUSUNLOCK(pn)
    if ( ret ) return 1 ;

    UT_delay(32) ;
    return 0 ;
}

static int OW_r_mem( unsigned char * data, const size_t size, const size_t offset, const struct parsedname * pn ) {
    unsigned char p[3] = { 0xF0, offset&0xFF , offset>>8, } ;
    int ret ;

    BUSLOCK(pn)
        ret = BUS_select(pn) || BUS_send_data( p, 3,pn) || BUS_readin_data( data, (int) size,pn) ;
    BUSUNLOCK(pn)
    return ret ;
    
}
