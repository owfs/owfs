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
#include "ow_2433.h"

/* ------- Prototypes ----------- */

/* DS2433 EEPROM */
 bREAD_FUNCTION( FS_r_page ) ;
bWRITE_FUNCTION( FS_w_page ) ;
bWRITE_FUNCTION( FS_w_page2D ) ;
 bREAD_FUNCTION( FS_r_memory ) ;
 bWRITE_FUNCTION( FS_w_memory ) ;
 bWRITE_FUNCTION( FS_w_memory2D ) ;

 /* ------- Structures ----------- */

struct aggregate A2431 = { 4, ag_numbers, ag_separate,} ;
struct filetype DS2431[] = {
    F_STANDARD   ,
    {"pages"     ,    0,  NULL,   ft_subdir, ft_volatile, {v:NULL}        , {v:NULL}       , NULL, } ,
    {"pages/page",   32,  &A2431, ft_binary, ft_stable  , {b:FS_r_page}   , {b:FS_w_page2D}, NULL, } ,
    {"memory"    ,  512,  NULL,   ft_binary, ft_stable  , {b:FS_r_memory} , {b:FS_w_memory2D}, NULL, } ,
} ;
DeviceEntryExtended( 2D, DS2431 , DEV_ovdr | DEV_resume ) ;

struct aggregate A2433 = { 16, ag_numbers, ag_separate,} ;
struct filetype DS2433[] = {
    F_STANDARD   ,
    {"pages"     ,    0,  NULL,   ft_subdir, ft_volatile, {v:NULL}        , {v:NULL}       , NULL, } ,
    {"pages/page",   32,  &A2433, ft_binary, ft_stable  , {b:FS_r_page}   , {b:FS_w_page}  , NULL, } ,
    {"memory"    ,  512,  NULL,   ft_binary, ft_stable  , {b:FS_r_memory} , {b:FS_w_memory}, NULL, } ,
} ;
DeviceEntryExtended( 23, DS2433 , DEV_ovdr ) ;

/* ------- Functions ------------ */

/* DS2433 */

static int OW_w_mem( const unsigned char * data , const size_t size , const size_t offset, const struct parsedname * pn ) ;
static int OW_w_mem2D( const unsigned char * data , const size_t size , const size_t offset, const struct parsedname * pn ) ;
static int OW_r_mem( unsigned char * data, const size_t size, const size_t offset, const struct parsedname * pn ) ;


static int FS_r_memory(unsigned char *buf, const size_t size, const off_t offset , const struct parsedname * pn) {
    /* read is not page-limited */
    if ( OW_r_mem( buf, size, (size_t) offset, pn) ) return -EINVAL ;
    return size ;
}

static int FS_w_memory( const unsigned char *buf, const size_t size, const off_t offset , const struct parsedname * pn) {
    /* paged access */
    if ( OW_write_paged( buf, size, offset, pn, 32, OW_w_mem) ) return -EFAULT ;
    return 0 ;
}

/* Although externally it's 32 byte pages, internally it acts as 8 byte pages */
static int FS_w_memory2D( const unsigned char *buf, const size_t size, const off_t offset , const struct parsedname * pn) {
    /* paged access */
    if ( OW_write_paged( buf, size, offset, pn, 8, OW_w_mem2D) ) return -EFAULT ;
    return 0 ;
}

static int FS_r_page(unsigned char *buf, const size_t size, const off_t offset , const struct parsedname * pn) {
//    if ( pn->extension > 15 ) return -ERANGE ;
    if ( OW_r_mem( buf, size, offset+((pn->extension)<<5), pn) ) return -EINVAL ;
    return size ;
}

static int FS_w_page(const unsigned char *buf, const size_t size, const off_t offset , const struct parsedname * pn) {
//    if ( pn->extension > 15 ) return -ERANGE ;
    //printf("FS_w_page: size=%d offset=%d pn->extension=%d (%d)\n", size, offset, pn->extension, (pn->extension)<<5);
    if ( OW_w_mem(buf,size,offset+((pn->extension)<<5),pn) ) return -EINVAL ;
    return 0 ;
}

static int FS_w_page2D(const unsigned char *buf, const size_t size, const off_t offset , const struct parsedname * pn) {
//    if ( pn->extension > 15 ) return -ERANGE ;
    //printf("FS_w_page: size=%d offset=%d pn->extension=%d (%d)\n", size, offset, pn->extension, (pn->extension)<<5);
    if ( OW_w_mem2D(buf,size,offset+((pn->extension)<<5),pn) ) return -EINVAL ;
    return 0 ;
}

static int OW_r_mem( unsigned char * data , const size_t size , const size_t offset, const struct parsedname * pn) {
    unsigned char p[] = {  0xF0, offset&0xFF, offset>>8, } ;
    int ret ;
    //printf("reading offset=%d size=%d bytes\n", offset, size);

    BUSLOCK(pn);
        ret = BUS_select(pn) || BUS_send_data( p, 3,pn ) || BUS_readin_data( data,size,pn );
    BUSUNLOCK(pn);
    if ( ret ) return 1;

    return 0 ;
}

/* paged, and pre-screened */
static int OW_w_mem( const unsigned char * data , const size_t size , const size_t offset, const struct parsedname * pn ) {
    unsigned char p[3+32+2] = { 0x0F, offset&0xFF , offset>>8, } ;
    int ret ;

    /* Copy to scratchpad */
    memcpy( &p[3], data, size ) ;

    BUSLOCK(pn);
        ret = BUS_select(pn) || BUS_send_data(p,size+3,pn) ;
        if ( ret==0 && ((offset+size)&0x1F)==0 ) ret = BUS_readin_data(&p[size+3],2,pn) || CRC16(p,1+2+size+2) ;
    BUSUNLOCK(pn);
    if ( ret ) return 1 ;

    /* Re-read scratchpad and compare */
    /* Note that we tacitly shift the data one byte down for the E/S byte */
    p[0] = 0xAA ;
    BUSLOCK(pn);
        ret = BUS_select(pn) || BUS_send_data(p,1,pn) || BUS_readin_data(&p[1],3+size,pn) || memcmp( &p[4], data, size) ;
    BUSUNLOCK(pn);
    if ( ret ) return 1 ;

    /* Copy Scratchpad to EPROM */
    p[0] = 0x55 ;
    BUSLOCK(pn);
        ret = BUS_select(pn) || BUS_send_data(p,4,pn) ;
        UT_delay(5) ;
    BUSUNLOCK(pn);
    if ( ret ) return 1 ;

    return 0 ;
}
/* paged, and pre-screened */
static int OW_w_mem2D( const unsigned char * data , const size_t size , const size_t offset, const struct parsedname * pn ) {
    unsigned char p[3+8+3] = { 0x0F, offset&0xFF , offset>>8, } ;
    int ret = 0 ;
    size_t start = offset & 0x07 ; /* place within EPROM row */

    /* if incomplete row (8 bytes) read existing into buffer to fill it up */
    if ( start || (size&0x07) || OW_r_mem( &p[3], 8, offset-start, pn ) ) return 1 ;

    /* Copy to buffer overwriting read data */
    memcpy( &p[3+start], data, size ) ;

    /* send a row (8 bytes) to scratchpad */
    p[1] -= start ; /* Move address to start of row */
    BUSLOCK(pn);
        ret = BUS_select(pn) || BUS_send_data(p,3+8,pn) || BUS_readin_data(&p[3+8],2,pn) || CRC16(p,3+8+2) ;
    BUSUNLOCK(pn);
    if ( ret ) return 1 ;

    /* Re-read scratchpad and compare */
    /* Note that we tacitly shift the data one byte down for the E/S byte */
    p[0] = 0xAA ;
    BUSLOCK(pn);
        ret = BUS_select(pn) || BUS_send_data(p,1,pn) || BUS_readin_data(&p[1],3+size+2,pn) || CRC16(p,4+8+2) || memcmp( &p[4+start], data, size) ;
    BUSUNLOCK(pn);
    if ( ret ) return 1 ;

    /* Copy Scratchpad to EPROM */
    p[0] = 0x55 ;
    BUSLOCK(pn);
        ret = BUS_select(pn) || BUS_send_data(p,4,pn) ;
        UT_delay(13) ;
    BUSUNLOCK(pn);
    if ( ret ) return 1 ;

    return 0 ;
}
