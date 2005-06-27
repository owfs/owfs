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
 uREAD_FUNCTION( FS_counter) ;

/* ------- Structures ----------- */

struct aggregate A1963S = { 16, ag_numbers, ag_separate, } ;
struct filetype DS1963S[] = {
    F_STANDARD   ,
    {"pages"     ,     0,  NULL,   ft_subdir, ft_volatile, {v:NULL}       , {v:NULL}       , NULL, } ,
    {"pages/page",    32,  &A1963S, ft_binary, ft_stable  , {b:FS_r_page}   , {b:FS_w_page}, NULL, } ,
    {"pages/count",   12,  &A1963S, ft_unsigned, ft_volatile  , {u:FS_counter},  {v:NULL}, NULL, } ,
    {"pages/password", 8,  NULL, ft_binary, ft_stable  ,       {v:NULL} , {b:FS_w_password}, NULL, } ,
    {"memory"    ,   512,  NULL, ft_binary, ft_stable  , {b:FS_r_memory} , {b:FS_w_memory}, NULL, } ,
    {"password"  ,     8,  NULL, ft_binary, ft_stable  ,       {v:NULL} , {b:FS_w_password}, NULL, } ,
} ;
DeviceEntryExtended( 18, DS1963S, DEV_resume )

struct aggregate A1963L = { 16, ag_numbers, ag_separate, } ;
struct filetype DS1963L[] = {
    F_STANDARD   ,
    {"pages"     ,     0,  NULL,   ft_subdir, ft_volatile, {v:NULL}       , {v:NULL}       , NULL, } ,
    {"pages/page",    32,  &A1963L, ft_binary, ft_stable  , {b:FS_r_page}   , {b:FS_w_page}, NULL, } ,
    {"pages/count",   12,  &A1963L, ft_unsigned, ft_volatile  , {u:FS_counter},  {v:NULL}, NULL, } ,
    {"memory"    ,   512,  NULL, ft_binary, ft_stable  , {b:FS_r_memory} , {b:FS_w_memory}, NULL, } ,
} ;
DeviceEntry( 1A, DS1963L )

/* ------- Functions ------------ */

static int OW_w_mem( const unsigned char * data , const size_t size , const size_t offset, const struct parsedname * pn ) ;
static int OW_r_mem( unsigned char * data, const size_t size, const size_t offset, const struct parsedname * pn ) ;
static int OW_r_mem_counter( unsigned char * p, unsigned int * counter, const size_t size, const size_t offset, const struct parsedname * pn ) ;

static int FS_w_password(const unsigned char *buf, const size_t size, const off_t offset , const struct parsedname * pn) {
    (void) buf ;
    (void) size ;
    (void) offset ;
    (void) pn ;
  return -EINVAL;
}

static int FS_r_page(unsigned char *buf, const size_t size, const off_t offset , const struct parsedname * pn) {
    if ( OW_r_mem_counter(buf,NULL,size,offset+((pn->extension)<<5),pn) ) return -EINVAL;
    return size ;
}

static int FS_r_memory(unsigned char *buf, const size_t size, const off_t offset , const struct parsedname * pn) {
    /* read is not page-limited */
    if ( OW_read_paged( buf, size, offset, pn, 32, OW_r_mem ) ) return -EINVAL;
    return size ;
}

static int FS_counter( unsigned int * u, const struct parsedname * pn ) {
    if ( OW_r_mem_counter(NULL,u,1,((pn->extension)<<5)+31,pn) ) return -EINVAL ;
    return 0 ;
}

static int FS_w_page(const unsigned char *buf, const size_t size, const off_t offset , const struct parsedname * pn) {
  if ( OW_w_mem( buf, size, offset+((pn->extension)<<5), pn) ) return -EINVAL ;
  return 0 ;
}

static int FS_w_memory( const unsigned char *buf, const size_t size, const off_t offset , const struct parsedname * pn) {
    if ( OW_write_paged( buf, size, offset, pn, 32, OW_w_mem ) ) return -EINVAL ;
    return 0 ;
}

/* paged, and pre-screened */
static int OW_w_mem( const unsigned char * data , const size_t size , const size_t offset, const struct parsedname * pn ) {
    unsigned char p[1+2+32+2] = { 0x0F, offset&0xFF , offset>>8, } ;
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

    /* Copy Scratchpad to SRAM */
    p[0] = 0x5A ;
    BUSLOCK(pn);
        ret = BUS_select(pn) || BUS_send_data(p,4,pn) ;
    BUSUNLOCK(pn);
    return ret ;
}

static int OW_r_mem( unsigned char * data, const size_t size, const size_t offset, const struct parsedname * pn ) {
    return OW_r_mem_counter(data,NULL,size,((pn->extension)<<5)+offset,pn) ;
}

/* read memory area and counter (just past memory) */
/* Nathan Holmes help troubleshoot this one! */
static int OW_r_mem_counter( unsigned char * p, unsigned int * counter, const size_t size, const size_t offset, const struct parsedname * pn ) {
    unsigned char data[1+2+32+10] = { 0xA5, offset&0xFF , offset>>8, } ;
    int ret ;
    /* rest in the remaining length of the 32 byte page */
    size_t rest = 32 - (offset&0x1F) ;

    BUSLOCK(pn);
      /* read in (after command and location) 'rest' memory bytes, 4 counter bytes, 4 zero bytes, 2 CRC16 bytes */
      ret = BUS_select(pn) || BUS_send_data(data,3,pn) || BUS_readin_data(&data[3],rest+10,pn) || CRC16(data,rest+13) || data[rest+7]!=0x55 || data[rest+8]!=0x55 || data[rest+9]!=0x55 || data[rest+10]!=0x55 ;
    BUSUNLOCK(pn);
    if ( ret ) return 1 ;

    /* counter is held in the 4 bytes after the data */
    if ( counter ) 
        counter[0] = 
             ((unsigned int)data[rest+3])
           + (((unsigned int)data[rest+4])<<8) 
           + (((unsigned int)data[rest+5])<<16) 
           + (((unsigned int)data[rest+6])<<24) ;
    /* memory contents after the command and location */
    if ( p ) memcpy(p,&data[3],size) ;

    return 0 ;
}
