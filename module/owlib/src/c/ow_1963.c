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
#include "ow_1963.h"

/* ------- Prototypes ----------- */

 bREAD_FUNCTION( FS_r_page ) ;
bWRITE_FUNCTION( FS_w_page ) ;
 bREAD_FUNCTION( FS_r_memory ) ;
bWRITE_FUNCTION( FS_w_memory ) ;
bWRITE_FUNCTION( FS_w_password ) ;

/* ------- Structures ----------- */

struct aggregate A1963 = { 3, ag_numbers, ag_separate, } ;
struct filetype DS1963[] = {
    F_STANDARD   ,
    {"pages"     ,     0,  NULL,   ft_subdir, ft_volatile, {v:NULL}       , {v:NULL}       , NULL, } ,
    {"pages/page",    32,  &A1963, ft_binary, ft_stable  , {b:FS_r_page}   , {b:FS_w_page}, NULL, } ,
    {"pages/password", 8,  NULL, ft_binary, ft_stable  ,       {v:NULL} , {b:FS_w_password}, NULL, } ,
    {"memory"    ,   512,  NULL, ft_binary, ft_stable  , {b:FS_r_memory} , {b:FS_w_memory}, NULL, } ,
    {"password"  ,     8,  NULL, ft_binary, ft_stable  ,       {v:NULL} , {b:FS_w_password}, NULL, } ,
} ;
DeviceEntry( 18, DS1963 )

/* ------- Functions ------------ */

static int OW_w_mem( const unsigned char * data , const size_t size , const size_t offset, const struct parsedname * pn ) ;
static int OW_r_mem( unsigned char * data, const size_t size, const size_t offset, const struct parsedname * pn ) ;

static int FS_w_password(const unsigned char *buf, const size_t size, const off_t offset , const struct parsedname * pn) {
  return -EINVAL;
}

static int FS_r_page(unsigned char *buf, const size_t size, const off_t offset , const struct parsedname * pn) {
  return -EINVAL;
}

static int FS_r_memory(unsigned char *buf, const size_t size, const off_t offset , const struct parsedname * pn) {
    /* read is not page-limited */
  return -EINVAL;
}

static int FS_w_page(const unsigned char *buf, const size_t size, const off_t offset , const struct parsedname * pn) {
  return -EINVAL;
}

static int FS_w_memory( const unsigned char *buf, const size_t size, const off_t offset , const struct parsedname * pn) {
  return -EINVAL;
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
