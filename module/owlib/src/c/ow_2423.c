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

#include <config.h>
#include "owfs_config.h"
#include "ow_2423.h"

/* ------- Prototypes ----------- */

/* DS2423 counter */
 bREAD_FUNCTION( FS_r_mem ) ;
bWRITE_FUNCTION( FS_w_mem ) ;
 bREAD_FUNCTION( FS_r_page ) ;
bWRITE_FUNCTION( FS_w_page ) ;
 uREAD_FUNCTION( FS_counter ) ;
 uREAD_FUNCTION( FS_pagecount ) ;
#if OW_CACHE
 uREAD_FUNCTION( FS_r_mincount ) ;
uWRITE_FUNCTION( FS_w_mincount ) ;
#endif /* OW_CACHE */

/* ------- Structures ----------- */

struct aggregate A2423 = { 16, ag_numbers, ag_separate,} ;
struct aggregate A2423c = { 2, ag_letters, ag_separate,} ;
struct filetype DS2423[] = {
    F_STANDARD   ,
    {"memory"    ,   512,  NULL,    ft_binary  , ft_stable  , {b:FS_r_mem}    , {b:FS_w_mem},        {v:NULL}, } ,
    {"pages"     ,     0,  NULL,    ft_subdir  , ft_volatile, {v:NULL}        , {v:NULL}           , {v:NULL}, } ,
    {"pages/page",    32,  &A2423,  ft_binary  , ft_stable  , {b:FS_r_page}   , {b:FS_w_page},       {v:NULL}, } ,
    {"counters"  ,    12,  &A2423c, ft_unsigned, ft_volatile, {u:FS_counter}  , {v:NULL},            {v:NULL}, } ,
#if OW_CACHE
    {"mincount"  ,    12,  NULL,    ft_unsigned, ft_volatile, {u:FS_r_mincount} , {u:FS_w_mincount}, {v:NULL}, } ,
#endif /*OW_CACHE*/
    {"pages/count",   12,  &A2423,  ft_unsigned, ft_volatile, {u:FS_pagecount}, {v:NULL},            {v:NULL}, } ,
} ;
DeviceEntryExtended( 1D, DS2423 , DEV_ovdr ) ;

/* Persistent storage */
static struct internal_prop ip_cum = { "CUM", ft_persistent } ;

/* ------- Functions ------------ */

/* DS2423 */
static int OW_w_mem( const BYTE * data , const size_t size , const off_t offset, const struct parsedname * pn ) ;
static int OW_r_mem( BYTE * data , const size_t size , const off_t offset, const struct parsedname * pn ) ;
static int OW_counter( UINT * counter , const int page, const struct parsedname * pn ) ;
static int OW_r_mem_counter( BYTE * p, UINT * counter, const size_t size, const off_t offset, const struct parsedname * pn ) ;

/* 2423A/D Counter */
static int FS_r_page(BYTE *buf, const size_t size, const off_t offset , const struct parsedname * pn) {
    if ( OW_r_mem( buf, size, offset+((pn->extension)<<5), pn) ) return -EINVAL ;
    return size ;
}

static int FS_w_page(const BYTE *buf, const size_t size, const off_t offset , const struct parsedname * pn) {
    if ( OW_w_mem( buf, size, offset+((pn->extension)<<5), pn) ) return -EINVAL ;
    return 0 ;
}

static int FS_r_mem(BYTE *buf, const size_t size, const off_t offset , const struct parsedname * pn) {
    if ( OW_read_paged( buf, size, offset, pn, 32, OW_r_mem ) ) return -EINVAL ;
    return size ;
}

static int FS_w_mem(const BYTE *buf, const size_t size, const off_t offset , const struct parsedname * pn) {
    if ( OW_write_paged( buf, size, offset, pn, 32, OW_w_mem ) ) return -EINVAL ;
    return 0 ;
}

static int FS_counter(UINT * u , const struct parsedname * pn) {
    if ( OW_counter( u , (pn->extension)+14,  pn ) ) return -EINVAL ;
    return 0 ;
}

static int FS_pagecount(UINT * u , const struct parsedname * pn) {
    if ( OW_counter( u , pn->extension,  pn ) ) return -EINVAL ;
    return 0 ;
}

#if OW_CACHE /* Special code for cumulative counters -- read/write -- uses the caching system for storage */
/* Different from LCD system, counters are NOT reset with each read */
static int FS_r_mincount(UINT * u , const struct parsedname * pn ) {
    size_t s = 3*sizeof(UINT) ;
    UINT st[3], ct[2] ; // stored and current counter values

    if ( OW_counter( &ct[0] , 0,  pn ) || OW_counter( &ct[1] , 1,  pn ) ) return -EINVAL ; // current counters
    if ( Cache_Get_Internal( (void *) st, &s, &ip_cum, pn ) ) { // record doesn't (yet) exist
        st[2] = ct[0]<ct[1] ? ct[0] : ct[1] ;
    } else {
        UINT d0 = ct[0] - st[0] ; //delta counter.A
        UINT d1 = ct[1] - st[1] ; // delta counter.B
        st[2] += d0<d1 ? d0 : d1 ; // add minimum delta
    }
    st[0] = ct[0] ;
    st[1] = ct[1] ;
    u[0] = st[2] ;
    return Cache_Add_Internal( (void *) st, s, &ip_cum, pn ) ? -EINVAL  : 0 ;
}

static int FS_w_mincount(const UINT * u , const struct parsedname * pn ) {
    UINT st[3] ; // stored and current counter values

    if ( OW_counter( &st[0] , 0,  pn ) || OW_counter( &st[1] , 1,  pn ) ) return -EINVAL ;
    st[2] = u[0] ;
    return Cache_Add_Internal( (void *) st, 3*sizeof(UINT), &ip_cum, pn ) ? -EINVAL  : 0 ;
}
#endif /*OW_CACHE*/

static int OW_w_mem( const BYTE * data , const size_t size , const off_t offset, const struct parsedname * pn ) {
    BYTE p[1+2+32+2] = { 0x0F, offset&0xFF , offset>>8, } ;
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
    if ( ret ) return 1 ;

    UT_delay(32) ;
    return 0 ;
}

static int OW_r_mem( BYTE * data , const size_t size , const off_t offset, const struct parsedname * pn ) {
    return OW_r_mem_counter(data,NULL,size,offset,pn) ;
}

static int OW_counter( UINT * counter , const int page, const struct parsedname * pn ) {
    return OW_r_mem_counter(NULL,counter,1,(page<<5)+31,pn) ;
}

/* read memory area and counter (just past memory) */
/* Nathan Holmes help troubleshoot this one! */
static int OW_r_mem_counter( BYTE * p, UINT * counter, const size_t size, const off_t offset, const struct parsedname * pn ) {
    BYTE data[1+2+32+10] = { 0xA5, offset&0xFF , offset>>8, } ;
    int ret ;
    /* rest in the remaining length of the 32 byte page */
    size_t rest = 32 - (offset&0x1F) ;

    BUSLOCK(pn);
        /* read in (after command and location) 'rest' memory bytes, 4 counter bytes, 4 zero bytes, 2 CRC16 bytes */
        /* Incorporates fix by Fredrik Simonsson */
        ret = BUS_select(pn) || BUS_send_data(data,3,pn) || BUS_readin_data(&data[3],rest+10,pn) || CRC16(data,rest+13) ;
    BUSUNLOCK(pn);
    if ( ret ) return 1 ;

    /* counter is held in the 4 bytes after the data */
    if ( counter ) *counter = (((((((UINT) data[rest+6])<<8)|data[rest+5])<<8)|data[rest+4])<<8)|data[rest+3] ;
    /* memory contents after the command and location */
    if ( p ) memcpy(p,&data[3],size) ;

    return 0 ;
}
