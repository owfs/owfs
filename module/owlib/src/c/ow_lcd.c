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
#include "ow_lcd.h"

/* ------- Prototypes ----------- */

/* DS2438 Battery */
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
aWRITE_FUNCTION( FS_w_screen16 ) ;
aWRITE_FUNCTION( FS_w_screen20 ) ;
aWRITE_FUNCTION( FS_w_screen40 ) ;
aWRITE_FUNCTION( FS_w_line16 ) ;
aWRITE_FUNCTION( FS_w_line20 ) ;
aWRITE_FUNCTION( FS_w_line40 ) ;

/* ------- Structures ----------- */

struct aggregate ALCDg    = { 4, ag_numbers, ag_aggregate, } ;
struct aggregate ALCDc    = { 4, ag_numbers, ag_aggregate, } ;
#ifdef OW_CACHE
struct aggregate ALCDu    = { 4, ag_numbers, ag_aggregate, } ;
#endif /*OW_CACHE*/
struct aggregate ALCD_L16 = { 4, ag_numbers, ag_separate, } ;
struct aggregate ALCD_L20 = { 4, ag_numbers, ag_separate, } ;
struct aggregate ALCD_L40 = { 2, ag_numbers, ag_separate, } ;
struct filetype LCD[] = {
    F_STANDARD   ,
    {"LCDon"     ,     1,  NULL,      ft_yesno, ft_stable  , {v:NULL}         , {y:FS_w_on}       , NULL, } ,
    {"backlight" ,     1,  NULL,      ft_yesno, ft_stable  , {v:NULL}         , {y:FS_w_backlight}, NULL, } ,
    {"version"   ,    16,  NULL,      ft_ascii, ft_stable  , {a:FS_r_version} , {v:NULL}          , NULL, } ,
    {"gpio"      ,     1,  &ALCDg,    ft_yesno, ft_volatile, {y:FS_r_gpio}    , {y:FS_w_gpio}     , NULL, } ,
    {"register"  ,     1,  NULL,   ft_unsigned, ft_volatile, {u:FS_r_register}, {u:FS_w_register} , NULL, } ,
    {"data"      ,     1,  NULL,   ft_unsigned, ft_volatile, {u:FS_r_data}    , {u:FS_w_data}     , NULL, } ,
    {"counters"  ,    12,  &ALCDc, ft_unsigned, ft_volatile, {u:FS_r_counters}, {v:NULL}          , NULL, } ,
#ifdef OW_CACHE
    {"cumulative",    12,  &ALCDu, ft_unsigned, ft_volatile, {u:FS_r_cum}     , {u:FS_w_cum}      , NULL, } ,
#endif /*OW_CACHE*/
    {"memory"    ,   112,  NULL,     ft_binary, ft_stable  , {b:FS_r_memory}  , {b:FS_w_memory}   , NULL, } ,
    {"screen16"  ,   128,  NULL,      ft_ascii, ft_stable  , {v:NULL}         , {a:FS_w_screen16} , NULL, } ,
    {"screen20"  ,   128,  NULL,      ft_ascii, ft_stable  , {v:NULL}         , {a:FS_w_screen20} , NULL, } ,
    {"screen40"  ,   128,  NULL,      ft_ascii, ft_stable  , {v:NULL}         , {a:FS_w_screen40} , NULL, } ,
    {"line16"    ,    16,  &ALCD_L16, ft_ascii, ft_stable  , {v:NULL}         , {a:FS_w_line16}   , NULL, } ,
    {"line20"    ,    20,  &ALCD_L20, ft_ascii, ft_stable  , {v:NULL}         , {a:FS_w_line20}   , NULL, } ,
    {"line40"    ,    40,  &ALCD_L40, ft_ascii, ft_stable  , {v:NULL}         , {a:FS_w_line40}   , NULL, } ,
} ;
DeviceEntry( FF, LCD )

/* ------- Functions ------------ */

/* LCD by L. Swart */
static int OW_r_scratch( unsigned char * data , const int length , const struct parsedname* pn ) ;
static int OW_w_scratch( const unsigned char * data , const int length , const struct parsedname* pn ) ;
static int OW_w_on( const int state , const struct parsedname* pn ) ;
static int OW_w_backlight( const int state , const struct parsedname* pn ) ;
static int OW_w_register( const unsigned char data , const struct parsedname* pn ) ;
static int OW_r_register( unsigned char * data , const struct parsedname* pn ) ;
static int OW_w_data( const unsigned char data , const struct parsedname* pn ) ;
static int OW_r_data( unsigned char * data , const struct parsedname* pn ) ;
static int OW_w_gpio( const unsigned char data , const struct parsedname* pn ) ;
static int OW_r_gpio( unsigned char * data , const struct parsedname* pn ) ;
static int OW_r_version( unsigned char * data , const int len, const struct parsedname* pn ) ;
static int OW_r_counters( unsigned int * data , const struct parsedname* pn ) ;
static int OW_r_memory( unsigned char * data , int size, const int offset , const struct parsedname* pn ) ;
static int OW_w_memory( const unsigned char * data , const int size, const int offset , const struct parsedname* pn ) ;
static int OW_clear( const struct parsedname* pn ) ;
static int OW_w_screen( const unsigned char loc , const char * text , const int size, const struct parsedname* pn ) ;
static int FS_w_line(const int width, const int row, const char *buf, const size_t size, const off_t offset , const struct parsedname * pn ) ;
static int FS_w_screen(const int width, const char *buf, const size_t size, const off_t offset , const struct parsedname * pn ) ;

/* LCD */
static int FS_r_version(char *buf, const size_t size, const off_t offset , const struct parsedname * pn) {
    /* Not sure if this is valid, but won't allow offset != 0 at first */
    /* otherwise need a buffer */
    if ( offset != 0 ) return -EFAULT ;

    if ( OW_r_version( buf, (int)size, pn ) ) return -EINVAL ;
    return (size>16)?16:size ;
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
    unsigned char data ;
    int i ;
    if ( OW_r_gpio(&data,pn) ) return -EINVAL ;
    for ( i=0 ; i<4 ; ++i ) y[i] = ~ UT_getbit(&data , i ) ;
    return 0 ;
}

/* 4 value array */
static int FS_w_gpio(const int * y , const struct parsedname * pn ) {
    int i ;
    unsigned char data ;

//    /* First get current states */
    if ( OW_r_gpio(&data,pn) ) return -EINVAL ;
    /* Now set pins */
    for ( i=0 ; i<4 ; ++i ) UT_setbit(&data,i,y[i]==0) ;
    if ( OW_w_gpio(data,pn) ) return -EINVAL ;
    return 0 ;
}

static int FS_r_register(unsigned int * u , const struct parsedname * pn ) {
    unsigned char data ;
    if ( OW_r_register(&data,pn) ) return -EINVAL ;
    u[0] = data ;
    return 1 ;
}

static int FS_w_register(const unsigned int * u , const struct parsedname * pn ) {
    if ( OW_w_register(u[0],pn) ) return -EINVAL ;
    return 0 ;
}

static int FS_r_data(unsigned int * u , const struct parsedname * pn ) {
    unsigned char data ;
    if ( OW_r_data(&data,pn) ) return -EINVAL ;
    u[0] = data ;
    return 1 ;
}

static int FS_w_data(const unsigned int * u , const struct parsedname * pn ) {
    if ( OW_w_data(u[0],pn) ) return -EINVAL ;
    return 0 ;
}

static int FS_r_counters(unsigned int * u , const struct parsedname * pn ) {
    if ( OW_r_counters(u,pn) ) return -EINVAL ;
    return 0 ;
}

#ifdef OW_CACHE /* Special code for cumulative counters -- read/write -- uses the caching system for storage */
static int FS_r_cum(unsigned int * u , const struct parsedname * pn ) {
    int s = 4*sizeof(unsigned int) ;
    char key[] = {
                pn->sn[0],pn->sn[1],pn->sn[2],pn->sn[3], pn->sn[4], pn->sn[5], pn->sn[6], pn->sn[7],
                '/',      'c',      'u',      'm',       '\0',
                } ;

    if ( OW_r_counters(u,pn) ) return -EINVAL ;
    if ( Storage_Get( key, &s, (void *) u ) ) return -EINVAL ;
    return 0 ;
}

static int FS_w_cum(const unsigned int * u , const struct parsedname * pn ) {
    char key[] = {
                pn->sn[0],pn->sn[1],pn->sn[2],pn->sn[3], pn->sn[4], pn->sn[5], pn->sn[6], pn->sn[7],
                '/',      'c',      'u',      'm',       '\0',
                } ;
    return Storage_Add( key, 4*sizeof(unsigned int), (void *) u ) ? -EINVAL  : 0 ;
}
#endif /*OW_CACHE*/

static int FS_w_line(const int width, const int row, const char *buf, const size_t size, const off_t offset , const struct parsedname * pn ) {
    unsigned char loc[] = { 0x00, 0x40, 0x00+width, 0x40+width } ;
    char line[40] ;
    int i ;
    int cp =1 ;

    for ( i=offset ; i<width ; ++i ) {
        if ( cp && ((size_t)i<size) && buf[i] ) {
            line[i] = buf[i] ;
        } else {
            cp = 0 ;
            line[i] = ' ' ;
        }
    }
    if ( OW_w_screen(loc[row],line,width,pn) ) return -EINVAL ;
    return 0 ;
}

static int FS_w_line16(const char *buf, const size_t size, const off_t offset , const struct parsedname * pn ) {
    return FS_w_line( 16, pn->extension, buf, size, offset, pn ) ;
}

static int FS_w_line20(const char *buf, const size_t size, const off_t offset , const struct parsedname * pn ) {
    return FS_w_line( 20, pn->extension, buf, size, offset, pn ) ;
}

static int FS_w_line40(const char *buf, const size_t size, const off_t offset , const struct parsedname * pn ) {
    return FS_w_line( 40, pn->extension, buf, size, offset, pn ) ;
}

static int FS_w_screen(const int width, const char *buf, const size_t size, const off_t offset , const struct parsedname * pn ) {
    int row = 0 ; /* screen line */
    int rows = (width==40)?2:4 ; /* max number of rows */
    size_t len ; /* characters to print */
    size_t left = size ; /* number of characters left to print */
    char * ch ; /* search char for newline */
    const char * b = buf ; /* current location in buf */

    /* Not sure if this is valid, but won't allow offset != 0 at first */
    /* otherwise need a buffer */
    if ( offset != 0 ) return -EFAULT ;

    if ( OW_clear(pn) ) return -EFAULT ;

    for ( ; (row<rows) && (left>0) ; b+=len, left-=len, ++row ) {
        len = (left>width) ? width : left ; /* look only in this line */
        /* search for newline */
        if ( (ch=memchr(b, '\n', len)) ) len = ch - b ; /* shorten line up to newline */
        if ( FS_w_line(width,row,b,len,0,pn) ) return -EFAULT ;
        if (ch) { /* newline found */
            ++b ; /* move buf pointer to AFTER the newline */
            --left ;
        }
    }
    return 0 ;
}

static int FS_w_screen16(const char *buf, const size_t size, const off_t offset , const struct parsedname * pn ) {
    return FS_w_screen( 16, buf, size, offset, pn ) ;
}

static int FS_w_screen20(const char *buf, const size_t size, const off_t offset , const struct parsedname * pn ) {
    return FS_w_screen( 20, buf, size, offset, pn ) ;
}

static int FS_w_screen40(const char *buf, const size_t size, const off_t offset , const struct parsedname * pn ) {
    return FS_w_screen( 40, buf, size, offset, pn ) ;
}

static int FS_r_memory(unsigned char *buf, const size_t size, const off_t offset , const struct parsedname * pn ) {
    if ( offset > 112 ) return 0 ;
    if ( OW_r_memory((unsigned char *)buf,size,offset,pn) ) return -EFAULT ;
    return (size+offset>112) ? 112-offset : size ;
}

static int FS_w_memory(const unsigned char *buf, const size_t size, const off_t offset , const struct parsedname * pn ) {
    if ( OW_w_memory((const unsigned char *)buf,size,offset,pn) ) return -EFAULT ;
    return 0 ;
}

static int OW_w_scratch( const unsigned char * data , const int length , const struct parsedname* pn ) {
    unsigned char w = 0x4E ;
    int ret ;

    BUS_lock() ;
        ret = BUS_select(pn) || BUS_send_data( &w , 1 ) || BUS_send_data( data , length ) ;
    BUS_unlock() ;
    return ret ;
}

static int OW_r_scratch( unsigned char * data , const int length , const struct parsedname* pn ) {
    unsigned char r = 0xBE ;
    int ret ;

    BUS_lock() ;
        ret = BUS_select(pn) || BUS_send_data( &r , 1 ) || BUS_readin_data( data , length ) ;
    BUS_unlock() ;
    return ret ;
}

static int OW_w_on( const int state , const struct parsedname* pn ) {
    unsigned char w[] = { 0x03, 0x05, } ; /* on off */
    int ret ;

    BUS_lock() ;
        ret = BUS_select(pn) || BUS_send_data( &w[!state] , 1 ) ;
    BUS_unlock() ;
    return ret ;
}

static int OW_w_backlight( const int state , const struct parsedname* pn ) {
    unsigned char w[] = { 0x08, 0x07, } ; /* on off */
    int ret ;

    BUS_lock() ;
        ret = BUS_select(pn) || BUS_send_data( &w[!state] , 1 ) ;
    BUS_unlock() ;
    return ret ;
}

static int OW_w_register( const unsigned char data , const struct parsedname* pn ) {
    unsigned char w[] = { 0x10, data, } ;
    int ret ;

    BUS_lock() ;
        ret = BUS_select(pn) || BUS_send_data( w , 2 ) ;
    BUS_unlock() ;
    if ( ret ) return 1 ;

//    UT_delay(100) ;
    return 0 ;
}

static int OW_r_register( unsigned char * data , const struct parsedname* pn ) {
    unsigned char w = 0x11 ;
    int ret ;

    BUS_lock() ;
        ret = BUS_select(pn) || BUS_send_data( &w , 1 ) ;
    BUS_unlock() ;
    return ret ;

//    UT_delay(150) ;
    return OW_r_scratch( data, 1, pn ) ;
}

static int OW_w_data( const unsigned char data , const struct parsedname* pn ) {
    unsigned char w[] = { 0x12, data, } ;
    int ret ;

    BUS_lock() ;
        ret = BUS_select(pn) || BUS_send_data( w , 2 ) ;
    BUS_unlock() ;
    if ( ret ) return 1 ;

//    UT_delay(100) ;
    return 0 ;
}

static int OW_r_data( unsigned char * data , const struct parsedname* pn ) {
    unsigned char w = 0x13 ;
    int ret ;

    BUS_lock() ;
        ret = BUS_select(pn) || BUS_send_data( &w , 1 ) ;
    BUS_unlock() ;
    if ( ret ) return 1 ;

//    UT_delay(150) ;
    return OW_r_scratch( data, 1, pn ) ;
}

static int OW_w_gpio( const unsigned char data , const struct parsedname* pn ) {
    /* Note, it would be nice to control separately, nut
       we can't know the set state of the pin, i.e. sensed and set
       are confused */
    unsigned char w[] = { 0x21, data, } ;
    int ret ;

    BUS_lock() ;
        ret = BUS_select(pn) || BUS_send_data( w , 2 ) ;
    BUS_unlock() ;
    if ( ret ) return 1 ;

//    UT_delay(20) ;
    return 0 ;
}

static int OW_r_gpio( unsigned char * data , const struct parsedname* pn ) {
    unsigned char w = 0x22 ;
    int ret ;

    BUS_lock() ;
        ret = BUS_select(pn) || BUS_send_data( &w , 1 ) ;
    BUS_unlock() ;
    if ( ret ) return 1 ;

//    UT_delay(150) ;
    return OW_r_scratch( data, 1, pn ) ;
}

static int OW_r_version( unsigned char * data , const int len, const struct parsedname* pn ) {
    unsigned char w = 0x41 ;
    int ret ;

    BUS_lock() ;
        ret = BUS_select(pn) || BUS_send_data( &w , 1 ) ;
    BUS_unlock() ;
    if ( ret ) return 1 ;

//    UT_delay(500) ;
    return OW_r_scratch( data, (len>16)?16:len, pn ) ;
}

static int OW_r_counters( unsigned int * data , const struct parsedname* pn ) {
    unsigned char w = 0x23 ;
    unsigned char d[8] ;
    char key[] = {
            pn->sn[0],pn->sn[1],pn->sn[2],pn->sn[3], pn->sn[4], pn->sn[5], pn->sn[6], pn->sn[7],
            '/',      'c',      'u',      'm',       '\0',
            } ;
    unsigned int cum[4] ;
    int s = sizeof(cum);
    int ret ;

    BUS_lock() ;
        ret = BUS_select(pn) || BUS_send_data( &w , 1 ) ;
    BUS_unlock() ;
    if ( ret ) return 1 ;

//    UT_delay(80) ;
    if ( OW_r_scratch( d, 8, pn ) ) return 1 ;
    data[0] = ((unsigned int) d[1])<<8 | d[0] ;
    data[1] = ((unsigned int) d[3])<<8 | d[2] ;
    data[2] = ((unsigned int) d[5])<<8 | d[4] ;
    data[3] = ((unsigned int) d[7])<<8 | d[6] ;

//printf("OW_COUNTER key=%s\n",key);
    if ( Storage_Get( key, &s, (void *) cum ) ) { /* First pass at cumulative */
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
    Storage_Add( key, sizeof(cum), (void *) cum ) ;
    return 0 ;
}

static int OW_r_memory( unsigned char * data , const int size, const int offset , const struct parsedname* pn ) {
    unsigned char buf[2] = { (unsigned char)offset , (unsigned char)size, } ;
    unsigned char w = 0x37 ;
    int ret ;

    if ( buf[1] == 0 ) return 0 ;
    if ( offset >= 112 ) return 0 ;
    if ( offset+buf[1] >= 112 ) buf[1] = 112-offset ;
    if ( buf[1] > 16 ) return OW_r_memory(data,16,offset,pn) || OW_r_memory(&data[16],size-16,offset+16,pn) ;

    if ( OW_w_scratch(buf,2,pn) ) return 1 ;

    BUS_lock() ;
        ret = BUS_select(pn) || BUS_send_data(&w,1) ;
    BUS_unlock() ;
    if ( ret ) return 1 ;

//    UT_delay(400) ;
    return OW_r_scratch(data,buf[1],pn) ;
}

static int OW_w_memory( const unsigned char * data , const int size, const int offset , const struct parsedname* pn ) {
    unsigned char len = size ;
    unsigned char buf[17] = { (unsigned char)offset , } ;
    unsigned char w = 0x39 ;
    int ret ;

    if ( size == 0 ) return 0 ;
    if ( offset >= 112 ) return 0 ;
    if ( offset+len >= 112 ) len = 112-offset ;
    if ( len > 16 ) return OW_w_memory(data,16,offset,pn) || OW_w_memory(&data[16],size-16,offset+16,pn) ;
    memcpy( &buf[1], data, (size_t) len ) ;

    if ( OW_w_scratch(buf,len+1,pn) ) return 1 ;

    BUS_lock() ;
        ret = BUS_select(pn) || BUS_send_data(&w,1) ;
    BUS_unlock() ;
    if ( ret ) return 1 ;

    UT_delay(4*len) ;
    return 0 ;
}

static int OW_clear( const struct parsedname* pn ) {
    unsigned char c = 0x49 ; /* clear */
    int ret ;

    BUS_lock() ;
        ret = BUS_select(pn) || BUS_send_data( &c, 1) ;
    BUS_unlock() ;
    return ret ;
}

static int OW_w_screen( const unsigned char loc , const char * text , const int size, const struct parsedname* pn ) {
    unsigned char t[17] = { loc, } ;
    unsigned char w = 0x48 ;
    int s ;
    int l ;
    int ret ;

    for ( s=size ; s>0 ; s -= l ) {
        l = s ;
        if (l>16) l=16 ;
        memcpy( &t[1], &text[size-s] , (size_t) l ) ;

        if ( OW_w_scratch(t,l+1,pn) ) return 1 ;

        BUS_lock() ;
            ret = BUS_select(pn) || BUS_send_data( &w , 1 ) ;
        BUS_unlock() ;
        if ( ret ) return 1 ;

        t[0]+=l ;
    }
//    UT_delay(120*size) ;
    return 0 ;
}
