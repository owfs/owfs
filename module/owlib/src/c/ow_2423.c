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
#include "ow_2423.h"

/* ------- Prototypes ----------- */

/* DS2423 counter */
 bREAD_FUNCTION( FS_r_page ) ;
bWRITE_FUNCTION( FS_w_page ) ;
 uREAD_FUNCTION( FS_counter ) ;
 uREAD_FUNCTION( FS_pagecount ) ;
#ifdef OW_CACHE
 uREAD_FUNCTION( FS_r_mincount ) ;
uWRITE_FUNCTION( FS_w_mincount ) ;
#endif /* OW_CACHE */

/* ------- Structures ----------- */

struct aggregate A2423 = { 16, ag_numbers, ag_separate,} ;
struct aggregate A2423c = { 2, ag_letters, ag_separate,} ;
struct filetype DS2423[] = {
    F_STANDARD   ,
    {"pages"     ,     0,  NULL,    ft_subdir  , ft_volatile, {v:NULL}        , {v:NULL}           , NULL, } ,
    {"pages/page",    32,  &A2423,  ft_binary  , ft_stable  , {b:FS_r_page}   , {b:FS_w_page},       NULL, } ,
    {"counters"  ,    32,  &A2423c, ft_unsigned, ft_volatile, {u:FS_counter}  , {v:NULL},            NULL, } ,
#ifdef OW_CACHE
    {"mincount"  ,    12,  NULL,    ft_unsigned, ft_volatile, {u:FS_r_mincount} , {u:FS_w_mincount}, NULL, } ,
#endif /*OW_CACHE*/
    {"pages/count",   32,  &A2423,  ft_unsigned, ft_volatile, {u:FS_pagecount}, {v:NULL},            NULL, } ,
} ;
DeviceEntry( 1D, DS2423 )

/* ------- Functions ------------ */

/* DS2423 */
static int OW_w_mem( const unsigned char * data , const size_t size , const size_t offset, const struct parsedname * pn ) ;
static int OW_r_mem( unsigned char * data , const size_t length , const size_t location, const struct parsedname * pn ) ;
static int OW_counter( unsigned int * counter , const int page, const struct parsedname * pn ) ;
static int OW_r_mem_counter( unsigned char * p, unsigned int * counter, const size_t size, const size_t offset, const struct parsedname * pn ) ;

/* 2423A/D Counter */
static int FS_r_page(unsigned char *buf, const size_t size, const off_t offset , const struct parsedname * pn) {
    if ( OW_r_mem( buf, size, offset+((pn->extension)<<5), pn) ) return -EINVAL ;
    return size ;
}

static int FS_w_page(const unsigned char *buf, const size_t size, const off_t offset , const struct parsedname * pn) {
    if ( OW_w_mem( buf, size, offset+((pn->extension)<<5), pn) ) return -EINVAL ;
    return 0 ;
}

static int FS_counter(unsigned int * u , const struct parsedname * pn) {
    if ( OW_counter( u , (pn->extension)+14,  pn ) ) return -EINVAL ;
    return 0 ;
}

static int FS_pagecount(unsigned int * u , const struct parsedname * pn) {
    if ( OW_counter( u , pn->extension,  pn ) ) return -EINVAL ;
    return 0 ;
}

#ifdef OW_CACHE /* Special code for cumulative counters -- read/write -- uses the caching system for storage */
/* Different from LCD system, counters are NOT reset with each read */
static int FS_r_mincount(unsigned int * u , const struct parsedname * pn ) {
    int s = 3*sizeof(unsigned int) ;
    unsigned int st[3], ct[2] ; // stored and current counter values
    char key[] = {
                pn->sn[0],pn->sn[1],pn->sn[2],pn->sn[3], pn->sn[4], pn->sn[5], pn->sn[6], pn->sn[7],
                '/',      'c',      'u',      'm',       '\0',
                } ;

    if ( OW_counter( &ct[0] , 0,  pn ) || OW_counter( &ct[1] , 1,  pn ) ) return -EINVAL ; // current counters
    if ( Storage_Get( key, &s, (void *) st ) ) { // record doesn't (yet) exist
        st[2] = ct[0]<ct[1] ? ct[0] : ct[1] ;
    } else {
        unsigned int d0 = ct[0] - st[0] ; //delta counter.A
        unsigned int d1 = ct[1] - st[1] ; // delta counter.B
        st[2] += d0<d1 ? d0 : d1 ; // add minimum delta
    }
    st[0] = ct[0] ;
    st[1] = ct[1] ;
    u[0] = st[2] ;
    return Storage_Add( key, s, (void *) st ) ? -EINVAL  : 0 ;
}

static int FS_w_mincount(const unsigned int * u , const struct parsedname * pn ) {
    unsigned int st[3] ; // stored and current counter values
    char key[] = {
                pn->sn[0],pn->sn[1],pn->sn[2],pn->sn[3], pn->sn[4], pn->sn[5], pn->sn[6], pn->sn[7],
                '/',      'c',      'u',      'm',       '\0',
                } ;

    if ( OW_counter( &st[0] , 0,  pn ) || OW_counter( &st[1] , 1,  pn ) ) return -EINVAL ;
    st[2] = u[0] ;
    return Storage_Add( key, 3*sizeof(unsigned int), (void *) st ) ? -EINVAL  : 0 ;
}
#endif /*OW_CACHE*/

static int OW_w_mem( const unsigned char * data , const size_t size , const size_t offset, const struct parsedname * pn ) {
    unsigned char p[1+2+32+2] = { 0x0F, offset&0xFF , offset>>8, } ;
    size_t rest = 32-(offset&0x1F) ;
    int ret ;

    /* fill rest of data if needed */
    if ( (size<rest) && OW_r_mem_counter(&p[3+size],NULL,rest-size,offset+size,pn) ) return 1 ;

    /* Copy to scratchpad */
    memcpy( &p[3], data, size ) ;

    BUS_lock() ;
        ret = BUS_select(pn) || BUS_send_data(p,rest+3) || BUS_readin_data(&p[rest+3],2) || CRC16(p,1+2+rest+2) ;
    BUS_unlock() ;
    if ( ret ) return 1 ;

    /* Re-read scratchpad and compare */
    /* Note that we tacitly shift the data one byte down for the E/S byte */
    p[0] = 0xAA ;
    BUS_lock() ;
        ret = BUS_select(pn) || BUS_send_data(p,1) || BUS_readin_data(&p[1],3+size) || memcmp( &p[4], data, size) ;
    BUS_unlock() ;
    if ( ret ) return 1 ;

    /* Copy Scratchpad to SRAM */
    p[0] = 0x5A ;
    BUS_lock() ;
        ret = BUS_select(pn) || BUS_send_data(p,4) ;
    BUS_unlock() ;
    if ( ret ) return 1 ;

    UT_delay(32) ;
    return 0 ;
}

static int OW_r_mem( unsigned char * data , const size_t size , const size_t offset, const struct parsedname * pn ) {
    return OW_r_mem_counter(data,NULL,size,offset,pn) ;
}

static int OW_counter( unsigned int * counter , const int page, const struct parsedname * pn ) {
    return OW_r_mem_counter(NULL,counter,1,(page<<5)+31,pn) ;
}

static int OW_r_mem_counter( unsigned char * p, unsigned int * counter, const size_t size, const size_t offset, const struct parsedname * pn ) {
    unsigned char data[1+2+32+10] = { 0xA5, offset&0xFF , offset>>8, } ;
    int ret ;
    size_t rest = 32 - (offset&0x1F) ;

    BUS_lock() ;
        ret = BUS_select(pn) || BUS_send_data(data,3) || BUS_readin_data(&data[3],rest+10) || CRC16(p,rest+13) || data[rest+3] || data[rest+4] || data[rest+5] || data[rest+6];
    BUS_unlock() ;
    if ( ret ) return 1 ;

    if ( counter ) *counter = (((((((unsigned int) data[rest+10])<<8)|data[rest+9])<<8)|data[rest+8])<<8)|data[rest+7] ;
    if ( p ) memcpy(p,&data[3],size) ;

    return 0 ;
}
