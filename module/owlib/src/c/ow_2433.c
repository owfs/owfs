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
 bREAD_FUNCTION( FS_r_memory ) ;
bWRITE_FUNCTION( FS_w_memory ) ;

/* ------- Structures ----------- */

struct aggregate A2433 = { 16, ag_numbers, ag_separate,} ;
struct filetype DS2433[] = {
    F_STANDARD   ,
    {"pages"     ,     0,  NULL,   ft_subdir, ft_volatile, {v:NULL}       , {v:NULL}       , NULL, } ,
    {"pages/page",   32,  &A2433, ft_binary, ft_stable  , {b:FS_r_page}   , {b:FS_w_page}, NULL, } ,
    {"memory"    ,  512,  NULL, ft_binary, ft_stable  , {b:FS_r_memory} , {b:FS_w_memory}, NULL, } ,
} ;
DeviceEntry( 23, DS2433 )

/* ------- Functions ------------ */

/* DS2433 */

static int OW_w_mem( const unsigned char * data , const size_t size , const size_t offset, const struct parsedname * pn ) ;
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

static int FS_r_page(unsigned char *buf, const size_t size, const off_t offset , const struct parsedname * pn) {
    if ( pn->extension > 15 ) return -ERANGE ;
    if ( OW_r_mem( buf, size, offset+((pn->extension)<<5), pn) ) return -EINVAL ;
    return size ;
}

static int FS_w_page(const unsigned char *buf, const size_t size, const off_t offset , const struct parsedname * pn) {
    if ( pn->extension > 15 ) return -ERANGE ;
    //printf("FS_w_page: size=%d offset=%d pn->extension=%d (%d)\n", size, offset, pn->extension, (pn->extension)<<5);
    if ( OW_w_mem(buf,size,offset+((pn->extension)<<5),pn) ) return -EINVAL ;
    return 0 ;
}

static int OW_r_mem( unsigned char * p , const size_t size , const size_t offset, const struct parsedname * pn) {
    unsigned char data[513] ;
    unsigned char memread[] = {  0xF0, offset&0xFF, (offset>>8)&0x01, } ;
    size_t rest = 512-(offset&0x1FF) ;
    int ret ;

    //printf("reading offset=%d rest=%d size=%d bytes\n", offset, rest, size);
      
    BUSLOCK
    ret = BUS_select(pn) || BUS_send_data( memread, 3 ) || BUS_readin_data( data,rest+1 );
    BUSUNLOCK
    if ( ret ) return 1;

    // copy to buffer
    memcpy( p , data , size ) ;
    return 0 ;
}

static int OW_w_mem( const unsigned char * p , const size_t size , const size_t offset , const struct parsedname * pn ) {
    unsigned char data[32+4] ;
    unsigned char scratchcopy[] =  {  0x55, offset&0xFF, (offset>>8)&0x01, size } ;
    unsigned char scratchwrite[] = {  0x0F, offset&0xFF, (offset>>8)&0x01, } ;
    unsigned char scratchread[] =  {  0xAA, } ;
    int ret ;
    int i;

    //printf("writing offset=%d size=%d\n", offset, size);

    memcpy( data , p , size ) ;
    //for(i=0; i<size; i++) printf("%02X ", data[i]);
    //printf("\n");

    // write data to scratchpad
    BUSLOCK
    ret = BUS_select(pn) || BUS_send_data( scratchwrite , 3 ) || BUS_send_data( data, size) ;
    BUSUNLOCK
    if ( ret ) return 1;

    // read data from scratchpad
    BUSLOCK
    ret = BUS_select(pn) || BUS_send_data( scratchread , 1 ) || BUS_readin_data( data,3+size );
    BUSUNLOCK
    if ( ret ) return 1;

    //printf("got %02X %02X %02X data=%02X %02X\n", data[0], data[1], data[2], data[3], data[4]);
    // copy scratchpad to memory
    BUSLOCK
    scratchcopy[1] = data[0];
    scratchcopy[2] = data[1];
    scratchcopy[3] = data[2];
    ret = BUS_select(pn) || BUS_send_data( scratchcopy , 4 );
    BUSUNLOCK
    if ( ret ) return 1;

    UT_delay(5); // 5ms pullup

    // Could recheck with a read, but no point
    return 0 ;
}
