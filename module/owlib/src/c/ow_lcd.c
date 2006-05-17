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
#include "ow_lcd.h"

/* ------- Prototypes ----------- */

/* LCD display */
 aREAD_FUNCTION( FS_r_version ) ;
 uREAD_FUNCTION( FS_r_counters ) ;
yWRITE_FUNCTION( FS_w_on ) ;
yWRITE_FUNCTION( FS_w_backlight ) ;
 yREAD_FUNCTION( FS_r_gpio ) ;
yWRITE_FUNCTION( FS_w_gpio ) ;
 uREAD_FUNCTION( FS_r_data ) ;
uWRITE_FUNCTION( FS_w_data ) ;
 bREAD_FUNCTION( FS_r_memory ) ;
bWRITE_FUNCTION( FS_w_memory ) ;
 uREAD_FUNCTION( FS_r_register ) ;
uWRITE_FUNCTION( FS_w_register ) ;
#ifdef OW_CACHE
 uREAD_FUNCTION( FS_r_cum ) ;
uWRITE_FUNCTION( FS_w_cum ) ;
#endif /* OW_CACHE */
aWRITE_FUNCTION( FS_w_screenX ) ;
aWRITE_FUNCTION( FS_w_lineX ) ;

/* ------- Structures ----------- */

struct aggregate ALCD     = { 4, ag_numbers, ag_aggregate, } ;
struct aggregate ALCD_L16 = { 4, ag_numbers, ag_separate, } ;
struct aggregate ALCD_L20 = { 4, ag_numbers, ag_separate, } ;
struct aggregate ALCD_L40 = { 2, ag_numbers, ag_separate, } ;
struct filetype LCD[] = {
    F_STANDARD   ,
    {"LCDon"     ,     1,  NULL,      ft_yesno, ft_stable  , {v:NULL}         , {y:FS_w_on}       , {v:NULL}, } ,
    {"backlight" ,     1,  NULL,      ft_yesno, ft_stable  , {v:NULL}         , {y:FS_w_backlight}, {v:NULL}, } ,
    {"version"   ,    16,  NULL,      ft_ascii, ft_stable  , {a:FS_r_version} , {v:NULL}          , {v:NULL}, } ,
    {"gpio"      ,     1,  &ALCD,     ft_yesno, ft_volatile, {y:FS_r_gpio}    , {y:FS_w_gpio}     , {v:NULL}, } ,
    {"register"  ,    12,  NULL,   ft_unsigned, ft_volatile, {u:FS_r_register}, {u:FS_w_register} , {v:NULL}, } ,
    {"data"      ,    12,  NULL,   ft_unsigned, ft_volatile, {u:FS_r_data}    , {u:FS_w_data}     , {v:NULL}, } ,
    {"counters"  ,    12,  &ALCD , ft_unsigned, ft_volatile, {u:FS_r_counters}, {v:NULL}          , {v:NULL}, } ,
#ifdef OW_CACHE
    {"cumulative",    12,  &ALCD , ft_unsigned, ft_volatile, {u:FS_r_cum}     , {u:FS_w_cum}      , {v:NULL}, } ,
#endif /*OW_CACHE*/
    {"memory"    ,   112,  NULL,     ft_binary, ft_stable  , {b:FS_r_memory}  , {b:FS_w_memory}   , {v:NULL} , } ,
    {"screen16"  ,   128,  NULL,      ft_ascii, ft_stable  , {v:NULL}         , {a:FS_w_screenX}  , {i: 16} , } ,
    {"screen20"  ,   128,  NULL,      ft_ascii, ft_stable  , {v:NULL}         , {a:FS_w_screenX}  , {i: 20} , } ,
    {"screen40"  ,   128,  NULL,      ft_ascii, ft_stable  , {v:NULL}         , {a:FS_w_screenX}  , {i: 40} , } ,
    {"line16"    ,    16,  &ALCD_L16, ft_ascii, ft_stable  , {v:NULL}         , {a:FS_w_lineX}    , {i: 16} , } ,
    {"line20"    ,    20,  &ALCD_L20, ft_ascii, ft_stable  , {v:NULL}         , {a:FS_w_lineX}    , {i: 20} , } ,
    {"line40"    ,    40,  &ALCD_L40, ft_ascii, ft_stable  , {v:NULL}         , {a:FS_w_lineX}    , {i: 40} , } ,
} ;
DeviceEntryExtended( FF, LCD , DEV_alarm ) ;

/* ------- Functions ------------ */

/* LCD by L. Swart */
static int OW_r_scratch( BYTE * data , const int length , const struct parsedname* pn ) ;
static int OW_w_scratch( const BYTE * data , const int length , const struct parsedname* pn ) ;
static int OW_w_on( const int state , const struct parsedname* pn ) ;
static int OW_w_backlight( const int state , const struct parsedname* pn ) ;
static int OW_w_register( const BYTE data , const struct parsedname* pn ) ;
static int OW_r_register( BYTE * data , const struct parsedname* pn ) ;
static int OW_w_data( const BYTE data , const struct parsedname* pn ) ;
static int OW_r_data( BYTE * data , const struct parsedname* pn ) ;
static int OW_w_gpio( const BYTE data , const struct parsedname* pn ) ;
static int OW_r_gpio( BYTE * data , const struct parsedname* pn ) ;
static int OW_r_version( BYTE * data , const struct parsedname* pn ) ;
static int OW_r_counters( UINT * data , const struct parsedname* pn ) ;
static int OW_r_memory( BYTE * data , const size_t size, const size_t offset , const struct parsedname* pn ) ;
static int OW_w_memory( const BYTE * data , const size_t size, const size_t offset , const struct parsedname* pn ) ;
static int OW_clear( const struct parsedname* pn ) ;
static int OW_w_screen( const BYTE loc , const char * text , const int size, const struct parsedname* pn ) ;

/* Internal files */
static struct internal_prop ip_cum = { "CUM", ft_persistent } ;

/* LCD */
static int FS_r_version(char *buf, const size_t size, const off_t offset , const struct parsedname * pn) {
    /* Not sure if this is valid, but won't allow offset != 0 at first */
    /* otherwise need a buffer */
    char v[16] ;
    if ( OW_r_version( v, pn ) ) return -EINVAL ;
    memcpy( buf, &v[offset], size ) ;
    return size ;
}

static int FS_w_on(const int * y , const struct parsedname * pn ) {
    if ( OW_w_on(y[0],pn) ) return -EINVAL ;
    return 0 ;
}

static int FS_w_backlight(const int * y , const struct parsedname * pn ) {
    if ( OW_w_backlight(y[0],pn) ) return -EINVAL ;
    return 0 ;
}

static int FS_r_gpio(int * y , const struct parsedname * pn ) {
    BYTE data ;
    int i ;
    if ( OW_r_gpio(&data,pn) ) return -EINVAL ;
    for ( i=0 ; i<4 ; ++i ) y[i] = ~ UT_getbit(&data , i ) ;
    return 0 ;
}

/* 4 value array */
static int FS_w_gpio(const int * y , const struct parsedname * pn ) {
    int i ;
    BYTE data ;

//    /* First get current states */
    if ( OW_r_gpio(&data,pn) ) return -EINVAL ;
    /* Now set pins */
    for ( i=0 ; i<4 ; ++i ) UT_setbit(&data,i,y[i]==0) ;
    if ( OW_w_gpio(data,pn) ) return -EINVAL ;
    return 0 ;
}

static int FS_r_register(UINT * u , const struct parsedname * pn ) {
    BYTE data ;
    if ( OW_r_register(&data,pn) ) return -EINVAL ;
    u[0] = data ;
    return 0 ;
}

static int FS_w_register(const UINT * u , const struct parsedname * pn ) {
    if ( OW_w_register(((BYTE)(u[0]&0xFF)),pn) ) return -EINVAL ;
    return 0 ;
}

static int FS_r_data(UINT * u , const struct parsedname * pn ) {
    BYTE data ;
    if ( OW_r_data(&data,pn) ) return -EINVAL ;
    u[0] = data ;
    return 0 ;
}

static int FS_w_data(const UINT * u , const struct parsedname * pn ) {
    if ( OW_w_data(((BYTE)(u[0]&0xFF)),pn) ) return -EINVAL ;
    return 0 ;
}

static int FS_r_counters(UINT * u , const struct parsedname * pn ) {
    if ( OW_r_counters(u,pn) ) return -EINVAL ;
    return 0 ;
}

#ifdef OW_CACHE /* Special code for cumulative counters -- read/write -- uses the caching system for storage */
static int FS_r_cum(UINT * u , const struct parsedname * pn ) {
    size_t s = 4*sizeof(UINT) ;

    if ( OW_r_counters(u,pn) ) return -EINVAL ; /* just to prime the "CUM" data */
    if ( Cache_Get_Internal( (void *) u, &s, &ip_cum, pn ) ) return -EINVAL ;
    return 0 ;
}

static int FS_w_cum(const UINT * u , const struct parsedname * pn ) {
    return Cache_Add_Internal( (const void *) u, 4*sizeof(UINT), &ip_cum, pn ) ? -EINVAL  : 0 ;
}
#endif /*OW_CACHE*/

static int FS_w_lineX(const char *buf, const size_t size, const off_t offset , const struct parsedname * pn ) {
    int width = pn->ft->data.i ;
    BYTE loc[] = { 0x00, 0x40, 0x00+width, 0x40+width } ;
    char line[width] ;

    if ( offset ) return -EADDRNOTAVAIL ;
    memcpy(line,buf,size) ;
    memset(&line[size],' ',width-size) ;
    if ( OW_w_screen(loc[pn->extension],line,width,pn) ) return -EINVAL ;
    return 0 ;
}

static int FS_w_screenX(const char *buf, const size_t size, const off_t offset , const struct parsedname * pn ) {
    int width = pn->ft->data.i ;
    int rows = (width==40)?2:4 ; /* max number of rows */
    struct parsedname pn2 ;
    const char * nl ;
    const char * b = buf ;
    const char * end = buf + size ;

    if ( offset ) return -EADDRNOTAVAIL ;

    if ( OW_clear(pn) ) return -EFAULT ;

    memcpy( &pn2, pn, sizeof(struct parsedname) ) ; /* shallow copy */

    for ( pn2.extension=0 ; pn2.extension<rows ; ++ pn2.extension ) {
        nl = memchr(b,'\n',end-b) ;
        if ( nl && nl<b+width ) {
            if ( FS_w_lineX(b,b-nl,0,&pn2) ) return -EINVAL ;
            b = nl+1 ; /* skip over newline */
        } else {
            nl = b + width ;
            if ( nl > end ) nl = end ;
            if ( FS_w_lineX(b,b-nl,0,&pn2) ) return -EINVAL ;
            b = nl ;
        }
        if ( b >= end ) break ;
    }
    return 0 ;
}

static int FS_r_memory(BYTE *buf, const size_t size, const off_t offset , const struct parsedname * pn ) {
//    if ( OW_r_memory(buf,size,offset,pn) ) return -EFAULT ;
    if ( OW_read_paged(buf,size,offset,pn, 16,OW_r_memory) ) return -EFAULT ;
    return size ;
}

static int FS_w_memory(const BYTE *buf, const size_t size, const off_t offset , const struct parsedname * pn ) {
//    if ( OW_w_memory(buf,size,offset,pn) ) return -EFAULT ;
    if ( OW_write_paged(buf,size,offset,pn, 16, OW_w_memory) ) return -EFAULT ;
    return 0 ;
}

static int OW_w_scratch( const BYTE * data , const int length , const struct parsedname* pn ) {
    BYTE w = 0x4E ;
    int ret ;

    BUSLOCK(pn);
        ret = BUS_select(pn) || BUS_send_data( &w, 1,pn ) || BUS_send_data( data, length,pn ) ;
    BUSUNLOCK(pn);
    return ret ;
}

static int OW_r_scratch( BYTE * data , const int length , const struct parsedname* pn ) {
    BYTE r = 0xBE ;
    int ret ;

    BUSLOCK(pn);
        ret = BUS_select(pn) || BUS_send_data( &r, 1,pn ) || BUS_readin_data( data, length,pn ) ;
    BUSUNLOCK(pn);
    return ret ;
}

static int OW_w_on( const int state , const struct parsedname* pn ) {
    BYTE w[] = { 0x03, 0x05, } ; /* on off */
    int ret ;

    BUSLOCK(pn);
        ret = BUS_select(pn) || BUS_send_data( &w[!state], 1,pn ) ;
    BUSUNLOCK(pn);
    return ret ;
}

static int OW_w_backlight( const int state , const struct parsedname* pn ) {
    BYTE w[] = { 0x08, 0x07, } ; /* on off */
    int ret ;

    BUSLOCK(pn);
        ret = BUS_select(pn) || BUS_send_data( &w[!state], 1,pn ) ;
    BUSUNLOCK(pn);
    return ret ;
}

static int OW_w_register( const BYTE data , const struct parsedname* pn ) {
    BYTE w[] = { 0x10, data, } ;
    int ret ;

    BUSLOCK(pn);
        ret = BUS_select(pn) || BUS_send_data( w, 2,pn ) ;
    BUSUNLOCK(pn);
    if ( ret ) return 1 ;

    UT_delay(1) ; // 100uS
    return 0 ;
}

static int OW_r_register( BYTE * data , const struct parsedname* pn ) {
    BYTE w = 0x11 ;
    int ret ;

    BUSLOCK(pn);
        ret = BUS_select(pn) || BUS_send_data( &w, 1,pn ) ;
    BUSUNLOCK(pn);
    if ( ret ) return 1 ;

    UT_delay(1) ; // 150uS
    return OW_r_scratch( data, 1, pn ) ;
}

static int OW_w_data( const BYTE data , const struct parsedname* pn ) {
    BYTE w[] = { 0x12, data, } ;
    int ret ;

    BUSLOCK(pn);
        ret = BUS_select(pn) || BUS_send_data( w, 2,pn ) ;
    BUSUNLOCK(pn);
    if ( ret ) return 1 ;

    UT_delay(1) ; // 100uS
    return 0 ;
}

static int OW_r_data( BYTE * data , const struct parsedname* pn ) {
    BYTE w = 0x13 ;
    int ret ;

    BUSLOCK(pn);
        ret = BUS_select(pn) || BUS_send_data( &w, 1,pn ) ;
    BUSUNLOCK(pn);
    if ( ret ) return 1 ;

    UT_delay(1) ; // 150uS
    return OW_r_scratch( data, 1, pn ) ;
}

static int OW_w_gpio( const BYTE data , const struct parsedname* pn ) {
    /* Note, it would be nice to control separately, nut
       we can't know the set state of the pin, i.e. sensed and set
       are confused */
    BYTE w[] = { 0x21, data, } ;
    int ret ;

    BUSLOCK(pn);
        ret = BUS_select(pn) || BUS_send_data( w, 2,pn ) ;
    BUSUNLOCK(pn);
    if ( ret ) return 1 ;

    UT_delay(1) ; // 20uS
    return 0 ;
}

static int OW_r_gpio( BYTE * data , const struct parsedname* pn ) {
    BYTE w = 0x22 ;
    int ret ;

    BUSLOCK(pn);
        ret = BUS_select(pn) || BUS_send_data( &w, 1,pn ) ;
    BUSUNLOCK(pn);
    if ( ret ) return 1 ;

    UT_delay(1) ; // 70uS
    return OW_r_scratch( data, 1, pn ) ;
}

static int OW_r_counters( UINT * data , const struct parsedname* pn ) {
    BYTE w = 0x23 ;
    BYTE d[8] ;
    UINT cum[4] ;
    size_t s = sizeof(cum);
    int ret ;

    BUSLOCK(pn);
        ret = BUS_select(pn) || BUS_send_data( &w, 1,pn ) ;
    BUSUNLOCK(pn);
    if ( ret ) return 1 ;

    UT_delay(1) ; // 80uS
    if ( OW_r_scratch( d, 8, pn ) ) return 1 ;
    data[0] = ((UINT) d[1])<<8 | d[0] ;
    data[1] = ((UINT) d[3])<<8 | d[2] ;
    data[2] = ((UINT) d[5])<<8 | d[4] ;
    data[3] = ((UINT) d[7])<<8 | d[6] ;

//printf("OW_COUNTER key=%s\n",key);
    if ( Cache_Get_Internal( (void *) cum, &s, &ip_cum, pn ) ) { /* First pass at cumulative */
        cum[0] = data[0] ;
        cum[1] = data[1] ;
        cum[2] = data[2] ;
        cum[3] = data[3] ;
    } else {
        cum[0] += data[0] ;
        cum[1] += data[1] ;
        cum[2] += data[2] ;
        cum[3] += data[3] ;
    }
    Cache_Add_Internal( (void *) cum, sizeof(cum), &ip_cum, pn ) ;
    return 0 ;
}

/* memory is 112 bytes */
/* can only read 16 bytes at a time (due to scratchpad size) */
/* Will pretend pagesize = 16 */
/* minor inefficiency if start is not on "page" boundary */
/* Call will not span "page" */
static int OW_r_memory( BYTE * data , const size_t size, const size_t offset , const struct parsedname* pn ) {
    BYTE buf[2] = { offset&0xFF , size&0xFF, } ;
    BYTE w = 0x37 ;
    int ret ;

    if ( buf[1] == 0 ) return 0 ;

    if ( OW_w_scratch(buf,2,pn) ) return 1 ;

    BUSLOCK(pn);
        ret = BUS_select(pn) || BUS_send_data(&w,1,pn) ;
    BUSUNLOCK(pn);
    if ( ret ) return 1 ;

    UT_delay(1) ; // 500uS
    return OW_r_scratch(data,buf[1],pn) ;
}

/* memory is 112 bytes */
/* can only write 16 bytes at a time (due to scratchpad size) */
/* Will pretend pagesize = 16 */
/* minor inefficiency if start is not on "page" boundary */
/* Call will not span "page" */
static int OW_w_memory( const BYTE * data , const size_t size, const size_t offset , const struct parsedname* pn ) {
    BYTE buf[17] = { offset&0xFF , } ;
    BYTE w = 0x39 ;
    int ret ;

    if ( size == 0 ) return 0 ;
    memcpy( &buf[1], data, size ) ;

    if ( OW_w_scratch(buf,size+1,pn) ) return 1 ;

    BUSLOCK(pn);
        ret = BUS_select(pn) || BUS_send_data(&w,1,pn) ;
    BUSUNLOCK(pn);
    if ( ret ) return 1 ;

    UT_delay(4*size) ; // 4mS/byte
    return 0 ;
}

/* data is 16 bytes */
static int OW_r_version( BYTE * data , const struct parsedname* pn ) {
    BYTE w = 0x41 ;
    int ret ;

    BUSLOCK(pn);
        ret = BUS_select(pn) || BUS_send_data( &w, 1,pn ) ;
    BUSUNLOCK(pn);
    if ( ret ) return 1 ;

    UT_delay(1) ; // 500uS
    return OW_r_scratch( data, 16, pn ) ;
}

static int OW_w_screen( const BYTE loc , const char * text , const int size, const struct parsedname* pn ) {
    BYTE t[17] = { loc, } ;
    BYTE w = 0x48 ;
    int s ;
    int l ;
    int ret ;

    for ( s=size ; s>0 ; s -= l ) {
        l = s ;
        if (l>16) l=16 ;
        memcpy( &t[1], &text[size-s] , (size_t) l ) ;

        if ( OW_w_scratch(t,l+1,pn) ) return 1 ;

        BUSLOCK(pn);
            ret = BUS_select(pn) || BUS_send_data( &w , 1,pn ) ;
        BUSUNLOCK(pn);
        if ( ret ) return 1 ;

        t[0]+=l ;
    }
    UT_delay(2) ; // 120uS/byte (max 1.92mS)
    return 0 ;
}

static int OW_clear( const struct parsedname* pn ) {
    BYTE c = 0x49 ; /* clear */
    int ret ;

    BUSLOCK(pn);
        ret = BUS_select(pn) || BUS_send_data( &c, 1,pn) ;
    BUSUNLOCK(pn);
    UT_delay(3) ; // 2.5mS
    return ret ;
}
