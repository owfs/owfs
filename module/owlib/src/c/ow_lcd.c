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
 aREAD_FUNCTION( FS_r_LCDversion ) ;
 uREAD_FUNCTION( FS_r_LCDcounters ) ;
yWRITE_FUNCTION( FS_w_LCDon ) ;
yWRITE_FUNCTION( FS_w_LCDbacklight ) ;
 yREAD_FUNCTION( FS_r_LCDgpio ) ;
yWRITE_FUNCTION( FS_w_LCDgpio ) ;
 uREAD_FUNCTION( FS_r_LCDdata ) ;
uWRITE_FUNCTION( FS_w_LCDdata ) ;
 bREAD_FUNCTION( FS_r_LCDmemory ) ;
bWRITE_FUNCTION( FS_w_LCDmemory ) ;
 uREAD_FUNCTION( FS_r_LCDregister ) ;
uWRITE_FUNCTION( FS_w_LCDregister ) ;
#ifdef OW_CACHE
 uREAD_FUNCTION( FS_r_LCDcum ) ;
uWRITE_FUNCTION( FS_w_LCDcum ) ;
#endif /* OW_CACHE */
aWRITE_FUNCTION( FS_w_LCDscreen16 ) ;
aWRITE_FUNCTION( FS_w_LCDscreen20 ) ;
aWRITE_FUNCTION( FS_w_LCDscreen40 ) ;
aWRITE_FUNCTION( FS_w_LCDline16 ) ;
aWRITE_FUNCTION( FS_w_LCDline20 ) ;
aWRITE_FUNCTION( FS_w_LCDline40 ) ;

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
    {"LCDon"     ,     1,  NULL,      ft_yesno , ft_stable  , {v:NULL}          , {y:FS_w_LCDon} , NULL,} ,
    {"backlight" ,     1,  NULL,      ft_yesno , ft_stable  , {v:NULL}          , {y:FS_w_LCDbacklight}, NULL, } ,
    {"version"   ,    16,  NULL,      ft_ascii , ft_stable  , {a:FS_r_LCDversion}, {v:NULL}, NULL, } ,
    {"gpio"      ,     1,  &ALCDg,    ft_yesno , ft_volatile, {y:FS_r_LCDgpio}  , {y:FS_w_LCDgpio}, NULL, } ,
    {"register"  ,     1,  NULL,      ft_unsigned, ft_volatile, {u:FS_r_LCDregister}, {u:FS_w_LCDregister}, NULL, } ,
    {"data"      ,     1,  NULL,      ft_unsigned, ft_volatile, {u:FS_r_LCDdata}  , {u:FS_w_LCDdata}, NULL, } ,
    {"counters"  ,    12,  &ALCDc,    ft_unsigned, ft_volatile,{u:FS_r_LCDcounters} , {v:NULL}, NULL, } ,
#ifdef OW_CACHE
    {"cumulative",    12,  &ALCDu,    ft_unsigned, ft_volatile,{u:FS_r_LCDcum}   , {u:FS_w_LCDcum}, NULL, } ,
#endif /*OW_CACHE*/
    {"memory"    ,   112,  NULL,      ft_binary, ft_stable  , {b:FS_r_LCDmemory}, {b:FS_w_LCDmemory}, NULL, } ,
    {"screen16"  ,   128,  NULL,      ft_ascii , ft_stable  , {v:NULL}          , {a:FS_w_LCDscreen16}, NULL, } ,
    {"screen20"  ,   128,  NULL,      ft_ascii , ft_stable  , {v:NULL}          , {a:FS_w_LCDscreen20}, NULL, } ,
    {"screen40"  ,   128,  NULL,      ft_ascii , ft_stable  , {v:NULL}          , {a:FS_w_LCDscreen40}, NULL, } ,
    {"line16"    ,    16,  &ALCD_L16, ft_ascii , ft_stable  , {v:NULL}          , {a:FS_w_LCDline16}, NULL, } ,
    {"line20"    ,    20,  &ALCD_L20, ft_ascii , ft_stable  , {v:NULL}          , {a:FS_w_LCDline20}, NULL, } ,
    {"line40"    ,    40,  &ALCD_L40, ft_ascii , ft_stable  , {v:NULL}          , {a:FS_w_LCDline40}, NULL, } ,
} ;
DeviceEntry( FF, LCD )

/* ------- Functions ------------ */

/* LCD by L. Swart */
static int OW_r_LCDscratch( unsigned char * data , const int length , const struct parsedname* pn ) ;
static int OW_w_LCDscratch( const unsigned char * data , const int length , const struct parsedname* pn ) ;
static int OW_w_LCDon( const int state , const struct parsedname* pn ) ;
static int OW_w_LCDbacklight( const int state , const struct parsedname* pn ) ;
static int OW_w_LCDregister( const unsigned char data , const struct parsedname* pn ) ;
static int OW_r_LCDregister( unsigned char * data , const struct parsedname* pn ) ;
static int OW_w_LCDdata( const unsigned char data , const struct parsedname* pn ) ;
static int OW_r_LCDdata( unsigned char * data , const struct parsedname* pn ) ;
static int OW_w_LCDgpio( const unsigned char data , const struct parsedname* pn ) ;
static int OW_r_LCDgpio( unsigned char * data , const struct parsedname* pn ) ;
static int OW_r_LCDversion( unsigned char * data , const int len, const struct parsedname* pn ) ;
static int OW_r_LCDcounters( unsigned int * data , const struct parsedname* pn ) ;
static int OW_r_LCDmemory( unsigned char * data , int size, const int offset , const struct parsedname* pn ) ;
static int OW_w_LCDmemory( const unsigned char * data , const int size, const int offset , const struct parsedname* pn ) ;
static int OW_LCDclear( const struct parsedname* pn ) ;
static int OW_w_LCDscreen( const unsigned char loc , const char * text , const int size, const struct parsedname* pn ) ;
static int FS_w_LCDline(const int width, const int row, const char *buf, const size_t size, const off_t offset , const struct parsedname * pn ) ;
static int FS_w_LCDscreen(const int width, const char *buf, const size_t size, const off_t offset , const struct parsedname * pn ) ;

/* LCD */
int FS_r_LCDversion(char *buf, const size_t size, const off_t offset , const struct parsedname * pn) {
    /* Not sure if this is valid, but won't allow offset != 0 at first */
    /* otherwise need a buffer */
    if ( offset != 0 ) return -EFAULT ;

    if ( OW_r_LCDversion( buf, (int)size, pn ) ) return -EINVAL ;
    return (size>16)?16:size ;
}

int FS_w_LCDon(const int * y , const struct parsedname * pn ) {
    if ( OW_w_LCDon(y[0],pn) ) return -EINVAL ;
    return 0 ;
}

int FS_w_LCDbacklight(const int * y , const struct parsedname * pn ) {
    if ( OW_w_LCDbacklight(y[0],pn) ) return -EINVAL ;
    return 0 ;
}

int FS_r_LCDgpio(int * y , const struct parsedname * pn ) {
    unsigned char data ;
    int i ;
    if ( OW_r_LCDgpio(&data,pn) ) return -EINVAL ;
    for ( i=0 ; i<4 ; ++i ) y[i] = ~ UT_getbit(&data , i ) ;
    return 0 ;
}

/* 4 value array */
int FS_w_LCDgpio(const int * y , const struct parsedname * pn ) {
    int i ;
    unsigned char data ;

//    /* First get current states */
    if ( OW_r_LCDgpio(&data,pn) ) return -EINVAL ;
    /* Now set pins */
    for ( i=0 ; i<4 ; ++i ) UT_setbit(&data,i,y[i]==0) ;
    if ( OW_w_LCDgpio(data,pn) ) return -EINVAL ;
    return 0 ;
}

int FS_r_LCDregister(unsigned int * u , const struct parsedname * pn ) {
    unsigned char data ;
    if ( OW_r_LCDregister(&data,pn) ) return -EINVAL ;
	u[0] = data ;
    return 1 ;
}

int FS_w_LCDregister(const unsigned int * u , const struct parsedname * pn ) {
    if ( OW_w_LCDregister(u[0],pn) ) return -EINVAL ;
    return 0 ;
}

int FS_r_LCDdata(unsigned int * u , const struct parsedname * pn ) {
    unsigned char data ;
    if ( OW_r_LCDdata(&data,pn) ) return -EINVAL ;
	u[0] = data ;
    return 1 ;
}

int FS_w_LCDdata(const unsigned int * u , const struct parsedname * pn ) {
    if ( OW_w_LCDdata(u[0],pn) ) return -EINVAL ;
    return 0 ;
}

int FS_r_LCDcounters(unsigned int * u , const struct parsedname * pn ) {
    if ( OW_r_LCDcounters(u,pn) ) return -EINVAL ;
    return 0 ;
}

#ifdef OW_CACHE /* Special code for cumulative counters -- read/write -- uses the caching system for storage */
int FS_r_LCDcum(unsigned int * u , const struct parsedname * pn ) {
	int s = 4*sizeof(unsigned int) ;
	char key[] = {
				pn->sn[0],pn->sn[1],pn->sn[2],pn->sn[3], pn->sn[4], pn->sn[5], pn->sn[6], pn->sn[7],
				'/',      'c',      'u',      'm',       '\0',
				} ;

    if ( OW_r_LCDcounters(u,pn) ) return -EINVAL ;
    if ( Storage_Get( key, &s, (void *) u ) ) return -EINVAL ;
    return 0 ;
}

int FS_w_LCDcum(const unsigned int * u , const struct parsedname * pn ) {
	char key[] = {
				pn->sn[0],pn->sn[1],pn->sn[2],pn->sn[3], pn->sn[4], pn->sn[5], pn->sn[6], pn->sn[7],
				'/',      'c',      'u',      'm',       '\0',
				} ;
	return Storage_Add( key, 4*sizeof(unsigned int), (void *) u ) ? -EINVAL  : 0 ;
}
#endif /*OW_CACHE*/

static int FS_w_LCDline(const int width, const int row, const char *buf, const size_t size, const off_t offset , const struct parsedname * pn ) {
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
    if ( OW_w_LCDscreen(loc[row],line,width,pn) ) return -EINVAL ;
    return 0 ;
}

int FS_w_LCDline16(const char *buf, const size_t size, const off_t offset , const struct parsedname * pn ) {
    return FS_w_LCDline( 16, pn->extension, buf, size, offset, pn ) ;
}

int FS_w_LCDline20(const char *buf, const size_t size, const off_t offset , const struct parsedname * pn ) {
    return FS_w_LCDline( 20, pn->extension, buf, size, offset, pn ) ;
}

int FS_w_LCDline40(const char *buf, const size_t size, const off_t offset , const struct parsedname * pn ) {
    return FS_w_LCDline( 40, pn->extension, buf, size, offset, pn ) ;
}

static int FS_w_LCDscreen(const int width, const char *buf, const size_t size, const off_t offset , const struct parsedname * pn ) {
    int row = 0 ;
    int len = size ;
    char * ch ;
    const char * b = buf ;

    /* Not sure if this is valid, but won't allow offset != 0 at first */
    /* otherwise need a buffer */
    if ( offset != 0 ) return -EFAULT ;

    if ( OW_LCDclear(pn) ) return -EFAULT ;

    while ( (row<((width==40)?2:4)) && (buf+size>b) ) {
        if ( len>width ) len = width ;
        if ( (ch=memchr(b, '\n', (size_t) len)) ) {
            len = ch - b ;
            if ( FS_w_LCDline(width,row,b,len,0,pn) ) return -EFAULT ;
            b += len+1 ;
            ++row ;
        } else {
            if ( FS_w_LCDline(width,row,b,len,0,pn) ) return -EFAULT ;
            b += len ;
            ++row ;
        }
    }
    return 0 ;
}

int FS_w_LCDscreen16(const char *buf, const size_t size, const off_t offset , const struct parsedname * pn ) {
    return FS_w_LCDscreen( 16, buf, size, offset, pn ) ;
}

int FS_w_LCDscreen20(const char *buf, const size_t size, const off_t offset , const struct parsedname * pn ) {
    return FS_w_LCDscreen( 20, buf, size, offset, pn ) ;
}

int FS_w_LCDscreen40(const char *buf, const size_t size, const off_t offset , const struct parsedname * pn ) {
    return FS_w_LCDscreen( 40, buf, size, offset, pn ) ;
}

int FS_r_LCDmemory(unsigned char *buf, const size_t size, const off_t offset , const struct parsedname * pn ) {
    if ( offset > 112 ) return 0 ;
    if ( OW_r_LCDmemory((unsigned char *)buf,size,offset,pn) ) return -EFAULT ;
    return (size+offset>112) ? 112-offset : size ;
}

int FS_w_LCDmemory(const unsigned char *buf, const size_t size, const off_t offset , const struct parsedname * pn ) {
    if ( OW_w_LCDmemory((const unsigned char *)buf,size,offset,pn) ) return -EFAULT ;
    return 0 ;
}

static int OW_w_LCDscratch( const unsigned char * data , const int length , const struct parsedname* pn ) {
    unsigned char w = 0x4E ;
    int ret ;

    BUS_lock() ;
        ret = BUS_select(pn) || BUS_send_data( &w , 1 ) || BUS_send_data( data , length ) ;
    BUS_unlock() ;
    return ret ;
}

static int OW_r_LCDscratch( unsigned char * data , const int length , const struct parsedname* pn ) {
    unsigned char r = 0xBE ;
    int ret ;

    BUS_lock() ;
        ret = BUS_select(pn) || BUS_send_data( &r , 1 ) || BUS_readin_data( data , length ) ;
    BUS_unlock() ;
    return ret ;
}

static int OW_w_LCDon( const int state , const struct parsedname* pn ) {
    unsigned char w[] = { 0x03, 0x05, } ; /* on off */
    int ret ;

    BUS_lock() ;
        ret = BUS_select(pn) || BUS_send_data( &w[!state] , 1 ) ;
    BUS_unlock() ;
    return ret ;
}

static int OW_w_LCDbacklight( const int state , const struct parsedname* pn ) {
    unsigned char w[] = { 0x08, 0x07, } ; /* on off */
    int ret ;

    BUS_lock() ;
        ret = BUS_select(pn) || BUS_send_data( &w[!state] , 1 ) ;
    BUS_unlock() ;
    return ret ;
}

static int OW_w_LCDregister( const unsigned char data , const struct parsedname* pn ) {
    unsigned char w[] = { 0x10, data, } ;
    int ret ;

    BUS_lock() ;
        ret = BUS_select(pn) || BUS_send_data( w , 2 ) ;
    BUS_unlock() ;
    if ( ret ) return 1 ;

//    UT_delay(100) ;
    return 0 ;
}

static int OW_r_LCDregister( unsigned char * data , const struct parsedname* pn ) {
    unsigned char w = 0x11 ;
    int ret ;

    BUS_lock() ;
        ret = BUS_select(pn) || BUS_send_data( &w , 1 ) ;
    BUS_unlock() ;
    return ret ;

//    UT_delay(150) ;
    return OW_r_LCDscratch( data, 1, pn ) ;
}

static int OW_w_LCDdata( const unsigned char data , const struct parsedname* pn ) {
    unsigned char w[] = { 0x12, data, } ;
    int ret ;

    BUS_lock() ;
        ret = BUS_select(pn) || BUS_send_data( w , 2 ) ;
    BUS_unlock() ;
    if ( ret ) return 1 ;

//    UT_delay(100) ;
    return 0 ;
}

static int OW_r_LCDdata( unsigned char * data , const struct parsedname* pn ) {
    unsigned char w = 0x13 ;
    int ret ;

    BUS_lock() ;
        ret = BUS_select(pn) || BUS_send_data( &w , 1 ) ;
    BUS_unlock() ;
    if ( ret ) return 1 ;

//    UT_delay(150) ;
    return OW_r_LCDscratch( data, 1, pn ) ;
}

static int OW_w_LCDgpio( const unsigned char data , const struct parsedname* pn ) {
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

static int OW_r_LCDgpio( unsigned char * data , const struct parsedname* pn ) {
    unsigned char w = 0x22 ;
    int ret ;

    BUS_lock() ;
        ret = BUS_select(pn) || BUS_send_data( &w , 1 ) ;
    BUS_unlock() ;
    if ( ret ) return 1 ;

//    UT_delay(150) ;
    return OW_r_LCDscratch( data, 1, pn ) ;
}

static int OW_r_LCDversion( unsigned char * data , const int len, const struct parsedname* pn ) {
    unsigned char w = 0x41 ;
    int ret ;

    BUS_lock() ;
        ret = BUS_select(pn) || BUS_send_data( &w , 1 ) ;
    BUS_unlock() ;
    if ( ret ) return 1 ;

//    UT_delay(500) ;
    return OW_r_LCDscratch( data, (len>16)?16:len, pn ) ;
}

static int OW_r_LCDcounters( unsigned int * data , const struct parsedname* pn ) {
    unsigned char w = 0x23 ;
    unsigned char d[8] ;
    int ret ;

    BUS_lock() ;
        ret = BUS_select(pn) || BUS_send_data( &w , 1 ) ;
    BUS_unlock() ;
    if ( ret ) return 1 ;

//    UT_delay(80) ;
    if ( OW_r_LCDscratch( d, 8, pn ) ) return 1 ;
    data[0] = ((unsigned int) d[1])<<8 | d[0] ;
    data[1] = ((unsigned int) d[3])<<8 | d[2] ;
    data[2] = ((unsigned int) d[5])<<8 | d[4] ;
    data[3] = ((unsigned int) d[7])<<8 | d[6] ;

#ifdef OW_CACHE /* Cache-dependent special cumulative storage code */
    if ( dbstore ) {
	char key[] = {
				pn->sn[0],pn->sn[1],pn->sn[2],pn->sn[3], pn->sn[4], pn->sn[5], pn->sn[6], pn->sn[7],
				'/',      'c',      'u',      'm',       '\0',
				} ;
	    unsigned int cum[4] ;
		int s = sizeof(cum);
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
    }
#endif /* OW_CACHE */
	return 0 ;
}

static int OW_r_LCDmemory( unsigned char * data , const int size, const int offset , const struct parsedname* pn ) {
    unsigned char buf[2] = { (unsigned char)offset , (unsigned char)size, } ;
    unsigned char w = 0x37 ;
    int ret ;

    if ( buf[1] == 0 ) return 0 ;
    if ( offset >= 112 ) return 0 ;
    if ( offset+buf[1] >= 112 ) buf[1] = 112-offset ;
    if ( buf[1] > 16 ) return OW_r_LCDmemory(data,16,offset,pn) || OW_r_LCDmemory(&data[16],size-16,offset+16,pn) ;

    if ( OW_w_LCDscratch(buf,2,pn) ) return 1 ;

    BUS_lock() ;
        ret = BUS_select(pn) || BUS_send_data(&w,1) ;
    BUS_unlock() ;
    if ( ret ) return 1 ;

//    UT_delay(400) ;
    return OW_r_LCDscratch(data,buf[1],pn) ;
}

static int OW_w_LCDmemory( const unsigned char * data , const int size, const int offset , const struct parsedname* pn ) {
    unsigned char len = size ;
    unsigned char buf[17] = { (unsigned char)offset , } ;
    unsigned char w = 0x39 ;
    int ret ;

    if ( size == 0 ) return 0 ;
    if ( offset >= 112 ) return 0 ;
    if ( offset+len >= 112 ) len = 112-offset ;
    if ( len > 16 ) return OW_w_LCDmemory(data,16,offset,pn) || OW_w_LCDmemory(&data[16],size-16,offset+16,pn) ;
    memcpy( &buf[1], data, (size_t) len ) ;

    if ( OW_w_LCDscratch(buf,len+1,pn) ) return 1 ;

    BUS_lock() ;
        ret = BUS_select(pn) || BUS_send_data(&w,1) ;
    BUS_unlock() ;
    if ( ret ) return 1 ;

    UT_delay(4*len) ;
    return 0 ;
}

static int OW_LCDclear( const struct parsedname* pn ) {
    unsigned char c = 0x49 ; /* clear */
    int ret ;

    BUS_lock() ;
        ret = BUS_select(pn) || BUS_send_data( &c, 1) ;
    BUS_unlock() ;
    return ret ;
}

static int OW_w_LCDscreen( const unsigned char loc , const char * text , const int size, const struct parsedname* pn ) {
    unsigned char t[17] = { loc, } ;
    unsigned char w = 0x48 ;
    int s ;
    int l ;
    int ret ;

    for ( s=size ; s>0 ; s -= l ) {
        l = s ;
        if (l>16) l=16 ;
        memcpy( &t[1], &text[size-s] , (size_t) l ) ;

        if ( OW_w_LCDscratch(t,l+1,pn) ) return 1 ;

        BUS_lock() ;
            ret = BUS_select(pn) || BUS_send_data( &w , 1 ) ;
        BUS_unlock() ;
        if ( ret ) return 1 ;

        t[0]+=l ;
    }
//    UT_delay(120*size) ;
    return 0 ;
}
