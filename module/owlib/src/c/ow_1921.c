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
#include "ow_1921.h"

/* ------- Prototypes ----------- */
/* DS1921 Temperature */
 fREAD_FUNCTION( FS_r_histotemp ) ;
 fREAD_FUNCTION( FS_r_histogap ) ;
 fREAD_FUNCTION( FS_r_resolution ) ;
 aREAD_FUNCTION( FS_r_version ) ;
 fREAD_FUNCTION( FS_r_rangelow ) ;
 fREAD_FUNCTION( FS_r_rangehigh ) ;
 uREAD_FUNCTION( FS_r_histogram ) ;
 fREAD_FUNCTION( FS_r_logtemp ) ;
 uREAD_FUNCTION( FS_r_logtime ) ;
 dREAD_FUNCTION( FS_r_logdate ) ;
 fREAD_FUNCTION( FS_r_temperature ) ;

 dREAD_FUNCTION( FS_r_date ) ;
dWRITE_FUNCTION( FS_w_date ) ;
 bREAD_FUNCTION( FS_r_mem ) ;
bWRITE_FUNCTION( FS_w_mem ) ;
 bREAD_FUNCTION( FS_r_page ) ;
bWRITE_FUNCTION( FS_w_page ) ;
 uREAD_FUNCTION( FS_r_samplerate ) ;
uWRITE_FUNCTION( FS_w_samplerate ) ;
 yREAD_FUNCTION( FS_r_run ) ;
yWRITE_FUNCTION( FS_w_run ) ;
 uREAD_FUNCTION( FS_r__3byte ) ;
 uREAD_FUNCTION( FS_r_atime ) ;
uWRITE_FUNCTION( FS_w_atime ) ;
 uREAD_FUNCTION( FS_r_atrig ) ;
uWRITE_FUNCTION( FS_w_atrig ) ;
 yREAD_FUNCTION( FS_r_mip ) ;
yWRITE_FUNCTION( FS_w_mip ) ;
/* ------- Structures ----------- */

struct aggregate A1921p = { 16, ag_numbers, ag_separate, } ;
struct aggregate A1921l = { 2048, ag_numbers, ag_separate, } ;
struct aggregate A1921h = { 63, ag_numbers, ag_aggregate, } ;
struct filetype DS1921[] = {
    F_STANDARD        ,
    {"memory"         ,512,   NULL,   ft_binary,  ft_stable, {b:FS_r_mem}        , {b:FS_w_mem}       , NULL, } ,
    {"pages"          ,  0,   NULL,   ft_subdir,ft_volatile, {v:NULL}            , {v:NULL}           , NULL, } ,
    {"pages/page"     ,256,&A1921p,   ft_binary,  ft_stable, {b:FS_r_page}       , {b:FS_w_page}      , NULL, } ,
    {"histogram"      ,  0,   NULL,   ft_subdir,ft_volatile, {v:NULL}            , {v:NULL}           , NULL, } ,
    {"histogram/counts"     ,  6,&A1921h, ft_unsigned,ft_volatile, {u:FS_r_histogram}  , {v:NULL}           , NULL, } ,
    {"histogram/temperature",  6,&A1921h,    ft_float,  ft_static, {f:FS_r_histotemp}  , {v:NULL}           , NULL, } ,
    {"histogram/gap"        ,  9,   NULL,    ft_float,  ft_static, {f:FS_r_histogap}   , {v:NULL}           , NULL, } ,
    {"tempresolution" ,  9,   NULL,    ft_float,  ft_static, {f:FS_r_resolution} , {v:NULL}           , NULL, } ,
    {"temprangelow"   ,  9,   NULL,    ft_float,  ft_static, {f:FS_r_rangelow}   , {v:NULL}           , NULL, } ,
    {"temprangehigh"  ,  9,   NULL,    ft_float,  ft_static, {f:FS_r_rangehigh}  , {v:NULL}           , NULL, } ,
    {"temperature"    , 12,   NULL,    ft_float,ft_volatile, {f:FS_r_temperature}, {v:NULL}           , NULL, } ,
    {"date"           , 24,   NULL,    ft_date ,  ft_second, {d:FS_r_date}       , {d:FS_w_date}      , NULL, } ,
    {"log"            ,  0,   NULL,   ft_subdir,ft_volatile, {v:NULL}            , {v:NULL}           , NULL, } ,
    {"log/temperature",  5,&A1921l,    ft_float,ft_volatile, {f:FS_r_logtemp}    , {v:NULL}           , NULL, } ,
    {"log/time"       , 12,&A1921l, ft_unsigned,ft_volatile, {u:FS_r_logtime}    , {v:NULL}           , NULL, } ,
    {"log/date"       , 24,&A1921l,    ft_date ,ft_volatile, {d:FS_r_logdate}    , {v:NULL}           , NULL, } ,
    {"samplerate"     ,  5,   NULL, ft_unsigned,  ft_stable, {u:FS_r_samplerate} , {u:FS_w_samplerate}, NULL, } ,
    {"running"        ,  1,   NULL,    ft_yesno,  ft_stable, {y:FS_r_run}        , {y:FS_w_run}       , NULL, } ,
    {"mission_samples", 12,   NULL, ft_unsigned,ft_volatile, {u:FS_r__3byte}     , {v:NULL}           , (void *)0x021A, } ,
    {"total_samples"  , 12,   NULL, ft_unsigned,ft_volatile, {u:FS_r__3byte}     , {v:NULL}           , (void *)0x021D, } ,
    {"alarm_second"   , 12,   NULL, ft_unsigned,  ft_stable, {u:FS_r_atime}      , {u:FS_w_atime}     , (void *)0x0207, } ,
    {"alarm_minute"   , 12,   NULL, ft_unsigned,  ft_stable, {u:FS_r_atime}      , {u:FS_w_atime}     , (void *)0x0208, } ,
    {"alarm_hour"     , 12,   NULL, ft_unsigned,  ft_stable, {u:FS_r_atime}      , {u:FS_w_atime}     , (void *)0x0209, } ,
    {"alarm_dow"      , 12,   NULL, ft_unsigned,  ft_stable, {u:FS_r_atime}      , {u:FS_w_atime}     , (void *)0x020A, } ,
    {"alarm_trigger"  , 12,   NULL, ft_unsigned,  ft_stable, {u:FS_r_atrig}      , {u:FS_w_atrig}     , NULL, } ,
    {"in_mission"     ,  1,   NULL,    ft_yesno,ft_volatile, {y:FS_r_mip}        , {y:FS_w_mip}       , NULL, } ,
    {"version"        , 11,   NULL,    ft_ascii,  ft_stable, {a:FS_r_version}    , {v:NULL}           , NULL, } ,
} ;
DeviceEntry( 21, DS1921 )

/* Different version of the Thermocron, sorted by ID[11,12] of name. Keep in sorted order */
struct Version { unsigned int ID ; char * name ; FLOAT histolow ; FLOAT resolution ; FLOAT rangelow ; FLOAT rangehigh ; unsigned int delay ;} ;
struct Version Versions[] =
    {
        { 0x000, "DS1921G-F5" , -40.0, 0.500, -40., +85.,  90, } ,
        { 0x064, "DS1921L-F50", -40.0, 0.500, -40., +85., 300, } ,
        { 0x15C, "DS1921L-F53", -40.0, 0.500, -30., +85., 300, } ,
        { 0x254, "DS1921L-F52", -40.0, 0.500, -20., +85., 300, } ,
        { 0x34C, "DS1921L-F51", -40.0, 0.500, -10., +85., 300, } ,
        { 0x3B2, "DS1921Z-F5" ,  -5.5, 0.125,  -5., +26., 360, } ,
        { 0x4F2, "DS1921H-F5" , +14.5, 0.125, +15., +46., 360, } ,
    } ;
#define VersionElements ( sizeof(Versions) / sizeof(struct Version) )
int VersionCmp( const void * pn , const void * version ) {
//printf("VC sn[5]=%.4X sn[6]=%.4X versionID=%.4X\n",((const struct parsedname *)pn)->sn[5],((const struct parsedname *)pn)->sn[6],((const struct Version *)version)->ID) ;
//printf("VC combo=%.4X\n", ( ((((const struct parsedname *)pn)->sn[5])>>4) |  (((unsigned int)((const struct parsedname *)pn)->sn[6])<<4) ) );
    return ( ((((const struct parsedname *)pn)->sn[5])>>4) |  (((unsigned int)((const struct parsedname *)pn)->sn[6])<<4) ) - ((const struct Version *)version)->ID ;
}
/* ------- Functions ------------ */

/* DS1921 */
static int OW_w_mem( const unsigned char * data , const size_t length , const size_t location, const struct parsedname * pn ) ;
static int OW_r_mem( unsigned char * data , const int length , const int location, const struct parsedname * pn ) ;
static int OW_temperature( int * T , const int delay, const struct parsedname * pn ) ;
static int OW_r_logtime(time_t *t, const struct parsedname * pn) ;
static int OW_clearmemory( const struct parsedname * const pn) ;

/* histogram lower bound */
static int FS_r_histogram(unsigned int * h , const struct parsedname * pn) {
    int j ;
    int i = 0 ;
    unsigned char data[32] ;

    if ( OW_r_mem( data , 32, 0x0800, pn ) ) return -EINVAL ;
    for ( j=0 ; j<32 ; j+=2 ) h[i++] =  ((unsigned int)data[j+1])<<8 | data[j] ;
//for ( j=0 ; j<16 ; j+=2 ) printf("HIS j=%3d %2X %2X %4X %u\n",j,data[j],data[j+1],((unsigned int)data[j+1])<<8 | data[j],((unsigned int)data[j+1])<<8 | data[j]) ;

    if ( OW_r_mem( data , 32, 0x0820, pn ) ) return -EINVAL ;
    for ( j=0 ; j<32 ; j+=2 ) h[i++] =  ((unsigned int)data[j+1])<<8 | data[j] ;
//for ( j=0 ; j<16 ; j+=2 ) printf("HIS j=%3d %2X %2X %4X %u\n",j,data[j],data[j+1],((unsigned int)data[j+1])<<8 | data[j],((unsigned int)data[j+1])<<8 | data[j]) ;
    if ( OW_r_mem( data , 32, 0x0840, pn ) ) return -EINVAL ;
    for ( j=0 ; j<32 ; j+=2 ) h[i++] =  ((unsigned int)data[j+1])<<8 | data[j] ;
//for ( j=0 ; j<16 ; j+=2 ) printf("HIS j=%3d %2X %2X %4X %u\n",j,data[j],data[j+1],((unsigned int)data[j+1])<<8 | data[j],((unsigned int)data[j+1])<<8 | data[j]) ;
    if ( OW_r_mem( data , 32, 0x0860, pn ) ) return -EINVAL ;
    for ( j=0 ; j<30 ; j+=2 ) h[i++] =  ((unsigned int)data[j+1])<<8 | data[j] ;
//for ( j=0 ; j<14 ; j+=2 ) printf("HIS j=%3d %2X %2X %4X %u\n",j,data[j],data[j+1],((unsigned int)data[j+1])<<8 | data[j],((unsigned int)data[j+1])<<8 | data[j]) ;
//for ( j=0;j<63;++j) printf("HIH j=%3d, %u\n",j,h[j]) ;
    return 0 ;
}

static int FS_r_histotemp(FLOAT * h , const struct parsedname * pn) {
    int i ;
    struct Version *v = (struct Version*) bsearch( pn , Versions , VersionElements, sizeof(struct Version), VersionCmp ) ;
    if ( v==NULL ) return -EINVAL ;
    for ( i=0 ; i<63 ; ++i ) h[i] = Temperature(v->histolow + 4*i*v->resolution) ;
    return 0 ;
}

static int FS_r_histogap(FLOAT * g , const struct parsedname * pn) {
    struct Version *v = (struct Version*) bsearch( pn , Versions , VersionElements, sizeof(struct Version), VersionCmp ) ;
    if ( v==NULL ) return -EINVAL ;
    *g = TemperatureGap(v->resolution*4) ;
    return 0 ;
}

static int FS_r_version(char *buf, const size_t size, const off_t offset , const struct parsedname * pn) {
    struct Version *v = (struct Version*) bsearch( pn , Versions , VersionElements, sizeof(struct Version), VersionCmp ) ;
    if ( v==NULL ) return -EINVAL ;
    if ( offset > strlen(v->name) ) return -EMSGSIZE ;
    strncpy(buf,(v->name)+offset,size) ;
    return size ;
}

static int FS_r_resolution(FLOAT * r , const struct parsedname * pn) {
    struct Version *v = (struct Version*) bsearch( pn , Versions , VersionElements, sizeof(struct Version), VersionCmp ) ;
    if ( v==NULL ) return -EINVAL ;
    *r = TemperatureGap(v->resolution) ;
    return 0 ;
}

static int FS_r_rangelow(FLOAT * rl , const struct parsedname * pn) {
    struct Version *v = (struct Version*) bsearch( pn , Versions , VersionElements, sizeof(struct Version), VersionCmp ) ;
    if ( v==NULL ) return -EINVAL ;
    *rl = Temperature(v->rangelow) ;
    return 0 ;
}

static int FS_r_rangehigh(FLOAT * rh , const struct parsedname * pn) {
    struct Version *v = (struct Version*) bsearch( pn , Versions , VersionElements, sizeof(struct Version), VersionCmp ) ;
    if ( v==NULL ) return -EINVAL ;
    *rh = Temperature(v->rangehigh) ;
    return 0 ;
}

/* Temperature -- force if not in progress */
static int FS_r_temperature(FLOAT * T , const struct parsedname * pn) {
    int temp ;
    struct Version *v = (struct Version*) bsearch( pn , Versions , VersionElements, sizeof(struct Version), VersionCmp ) ;
    if ( v==NULL ) return -EINVAL ;
    if ( OW_temperature( &temp , v->delay, pn ) ) return -EINVAL ;
    *T = Temperature( (FLOAT)temp * v->resolution + v->histolow ) ;
    return 0 ;
}

/* read counter */
/* Save a function by shoving address in data field */
static int FS_r__3byte(unsigned int * u , const struct parsedname * pn) {
    int addr = (int) pn->ft->data ;
    unsigned char data[3] ;
    if ( OW_r_mem(data,3,addr,pn) ) return -EINVAL ;
    *u = (((((unsigned int)data[2])<<8)|data[1])<<8)|data[0] ;
    return 0 ;
}

/* read clock */
static int FS_r_date(DATE * d , const struct parsedname * pn) {
    int ampm[8] = {0,10,20,30,0,10,12,22} ;
    unsigned char data[7] ;
    struct tm tm ;
    time_t t ;

    /* Prefill entries */
    t = time(NULL) ;
    if ( gmtime_r(&t,&tm)==NULL ) return -EINVAL ;

    /* Get date from chip */
    if ( OW_r_mem(data,7,0x0200,pn) ) return -EINVAL ;
    tm.tm_sec  = (data[0]&0x0F) + 10*(data[0]>>4) ; /* BCD->dec */
    tm.tm_min  = (data[1]&0x0F) + 10*(data[1]>>4) ; /* BCD->dec */
    tm.tm_hour = (data[2]&0x0F) + ampm[data[2]>>4] ; /* BCD->dec */
    tm.tm_mday = (data[4]&0x0F) + 10*(data[4]>>4) ; /* BCD->dec */
    tm.tm_mon  = (data[5]&0x0F) + 10*((data[5]&0x10)>>4) ; /* BCD->dec */
    tm.tm_year = (data[6]&0x0F) + 10*(data[6]>>4) + 100*(2-(data[5]>>7)); /* BCD->dec */
//printf("DATE_READ data=%2X, %2X, %2X, %2X, %2X, %2X, %2X\n",data[0],data[1],data[2],data[3],data[4],data[5],data[6]);
//printf("DATE: sec=%d, min=%d, hour=%d, mday=%d, mon=%d, year=%d, wday=%d, isdst=%d\n",tm.tm_sec,tm.tm_min,tm.tm_hour,tm.tm_mday,tm.tm_mon,tm.tm_year,tm.tm_wday,tm.tm_isdst) ;

    /* Pass through time_t again to validate */
    if ( (t=mktime(&tm)) == (time_t)-1 ) return -EINVAL ;
    d[0] = t ;
    return 0 ;
}

/* set clock */
static int FS_w_date(const DATE * d , const struct parsedname * pn) {
    struct tm tm ;
    unsigned char data[7] ;
    int year ;
    unsigned char sr ;

    /* Busy if in mission */
    if ( OW_r_mem(&sr,1,0x0214,pn) ) return -EINVAL ;
    if ( sr&0x20 ) return -EBUSY ;

    /* Convert time format */
    gmtime_r(d,&tm) ;

    data[0] = tm.tm_sec + 6*(tm.tm_sec/10) ; /* dec->bcd */
    data[1] = tm.tm_min + 6*(tm.tm_min/10) ; /* dec->bcd */
    data[2] = tm.tm_hour + 6*(tm.tm_hour/10) ; /* dec->bcd */
    data[3] = tm.tm_wday ; /* dec->bcd */
    data[4] = tm.tm_mday + 6*(tm.tm_mday/10) ; /* dec->bcd */
    data[5] = tm.tm_mon + 6*(tm.tm_mon/10) ; /* dec->bcd */
    year = tm.tm_year % 100 ;
    data[6] = year + 6*(year/10) ; /* dec->bcd */
    if ( tm.tm_year>99 && tm.tm_year<200 ) data[5] |= 0x80 ;
//printf("DATE_WRITE data=%2X, %2X, %2X, %2X, %2X, %2X, %2X\n",data[0],data[1],data[2],data[3],data[4],data[5],data[6]);
//printf("DATE: sec=%d, min=%d, hour=%d, mday=%d, mon=%d, year=%d, wday=%d, isdst=%d\n",tm.tm_sec,tm.tm_min,tm.tm_hour,tm.tm_mday,tm.tm_mon,tm.tm_year,tm.tm_wday,tm.tm_isdst) ;
    return OW_w_mem(data,7,0x0200,pn)?-EINVAL:0 ;
}

/* stop/start clock running */
static int FS_w_run(const int * y, const struct parsedname * pn) {
    unsigned char cr ;

    if ( OW_r_mem( &cr, 1, 0x020E, pn) ) return -EINVAL ;
    cr = y[0] ? cr&0x7F : cr|0x80 ;
    if ( OW_w_mem( &cr, 1, 0x020E, pn) ) return -EINVAL ;
    return 0 ;
}

/* clock running? */
static int FS_r_run(int * y , const struct parsedname * pn) {
    unsigned char cr ;
    if ( OW_r_mem(&cr, 1, 0x020E,pn) ) return -EINVAL ;
    y[0] = ((cr&0x80)==0) ;
    return 0 ;
}

/* start/stop mission */
static int FS_w_mip(const int * y, const struct parsedname * pn) {
    unsigned char cr ;
    if ( OW_r_mem(&cr, 1, 0x0214,pn) ) return -EINVAL ;
    if ( y[0] ) { /* start a mission! */
        int clockstate ;
        if ( (cr&0x10) == 0x10 ) return 0 ; /* already in progress */
        /* Make sure the clock is running */
        if ( FS_r_run( &clockstate, pn ) ) return -EINVAL ;
        if ( clockstate==0 ) {
        clockstate = 1 ;
            if ( FS_w_run( &clockstate, pn ) ) return -EINVAL ;
        UT_delay(1000) ;
    }
    /* Clear memory */
    if ( OW_clearmemory(pn ) ) return -EINVAL ;
        if ( OW_r_mem(&cr, 1, 0x020E,pn) ) return -EINVAL ;
    cr = (cr&0x3F) | 0x40 ;
        if ( OW_w_mem( &cr, 1, 0x020E, pn) ) return -EINVAL ;
    } else { /* turn off */
        if ( (cr&0x10) == 0x00 ) return 0 ; /* already off */
        cr ^= 0x10 ;
        if ( OW_w_mem( &cr, 1, 0x0214, pn) ) return -EINVAL ;
    }
    return 0 ;
}

/* mission is progress? */
static int FS_r_mip(int * y , const struct parsedname * pn) {
    unsigned char cr ;
    if ( OW_r_mem(&cr, 1, 0x0214,pn) ) return -EINVAL ;
    *y = ((cr&0x10)!=0) ;
    return 0 ;
}

/* read the interval between samples during a mission */
static int FS_r_samplerate(unsigned int * u , const struct parsedname * pn) {
    unsigned char data ;
    if ( OW_r_mem(&data,1,0x020D,pn) ) return -EINVAL ;
    *u = data ;
    return 0 ;
}

/* write the interval between samples during a mission */
static int FS_w_samplerate(const unsigned int * u , const struct parsedname * pn) {
    unsigned char data ;
    unsigned char sr ;

    /* Busy if in mission */
    if ( OW_r_mem(&sr,1,0x0214,pn) ) return -EINVAL ;
    if ( sr&0x20 ) return -EBUSY ;

    if ( OW_r_mem(&data,1,0x020E,pn) ) return -EFAULT ;
    data |= 0x10 ; /* EM on */
    if ( OW_w_mem(&data,1,0x020E,pn) ) return -EFAULT ;
    data = u[0] ;
    if (OW_w_mem(&data,1,0x020D,pn) ) return -EFAULT ;
    return 0 ;
}

/* read the alarm time field (not bit 7, though) */
static int FS_r_atime(unsigned int * u , const struct parsedname * pn) {
    unsigned char data ;
    if ( OW_r_mem(&data,1,(int) pn->ft->data,pn) ) return -EFAULT ;
    *u = data & 0x7F ;
    return 0 ;
}

/* write one of the alarm fields */
/* NOTE: keep first bit */
static int FS_w_atime(const unsigned int * u , const struct parsedname * pn) {
    unsigned char data ;

    if ( OW_r_mem(&data,1,(int) pn->ft->data,pn) ) return -EFAULT ;
    data = ( (unsigned char) u ) | (data&0x80) ; /* EM on */
    if ( OW_w_mem(&data,1,(int) pn->ft->data,pn) ) return -EFAULT ;
    return 0 ;
}

/* read the alarm time field (not bit 7, though) */
static int FS_r_atrig(unsigned int * u , const struct parsedname * pn) {
    unsigned char data[4] ;
    if ( OW_r_mem(data,4,0x0207,pn) ) return -EFAULT ;
    if ( data[3] & 0x80 ) {
        *u = 4 ;
    } else if (data[2] & 0x80) {
        *u = 3 ;
    } else if (data[1] & 0x80) {
        *u = 2 ;
    } else if (data[0] & 0x80) {
        *u = 1 ;
    } else {
        *u = 0 ;
    }
    return 0 ;
}

/* write one of the alarm fields */
/* NOTE: keep first bit */
static int FS_r_mem(unsigned char *buf, const size_t size, const off_t offset , const struct parsedname * pn) {
    if ( OW_r_mem( buf, size, offset, pn ) ) return -EINVAL ;
    return size ;
}

static int FS_w_mem(const unsigned char *buf, const size_t size, const off_t offset , const struct parsedname * pn) {
    if ( OW_w_mem( buf, size, offset, pn ) ) return -EINVAL ;
    return 0 ;
}

static int FS_w_atrig(const unsigned int * u , const struct parsedname * pn) {
    unsigned char data[4] ;
    if ( OW_r_mem(data,4,0x0207,pn) ) return -EFAULT ;
    data[0] &= 0x7F ;
    data[1] &= 0x7F ;
    data[2] &= 0x7F ;
    data[3] &= 0x7F ;
    switch (*u) { /* Intentional fall-throughs in cases */
    case 1:
        data[0] |= 0x80 ;
    case 2:
        data[1] |= 0x80 ;
    case 3:
        data[2] |= 0x80 ;
    case 4:
        data[3] |= 0x80 ;
    }
    if ( OW_w_mem(data,4,0x0207,pn) ) return -EFAULT ;
    return 0 ;
}

static int FS_r_page(unsigned char *buf, const size_t size, const off_t offset , const struct parsedname * pn) {
    if ( OW_r_mem( buf, size, offset+((pn->extension)<<5), pn ) ) return -EINVAL ;
    return size ;
}

static int FS_w_page(const unsigned char *buf, const size_t size, const off_t offset , const struct parsedname * pn) {
    if ( OW_w_mem( buf, size, offset+((pn->extension)<<5), pn ) ) return -EINVAL ;
    return 0 ;
}

/* temperature log */
static int FS_r_logtemp(FLOAT * T , const struct parsedname * pn) {
    unsigned char data ;
    struct Version *v = (struct Version*) bsearch( pn , Versions , VersionElements, sizeof(struct Version), VersionCmp ) ;
    if ( v==NULL ) return -EINVAL ;
    if ( OW_r_mem( &data , 1, 0x1000+pn->extension, pn ) ) return -EINVAL ;
    *T = (FLOAT)data * v->resolution + v->histolow ;
    return 0 ;
}

/* temperature log */
static int FS_r_logtime(unsigned int * u , const struct parsedname * pn) {
    time_t t ;
    if ( OW_r_logtime( &t, pn ) ) return -EINVAL ;
    *u = t ;
    return 0 ;
}

static int FS_r_logdate( DATE * d , const struct parsedname * pn) {
    if ( OW_r_logtime( d, pn ) ) return -EINVAL ;
    return 0 ;
}

static int OW_w_mem( const unsigned char * data , const size_t length , const size_t location, const struct parsedname * pn ) {
    unsigned char p[4+32+2] = { 0x0F, location&0xFF , location>>8, } ;
    int offset = location & 0x1F ;
    int rest = 32-offset ;
    int ret ;

    if ( offset+length > 32 ) return OW_w_mem( data, (size_t) rest, location, pn) || OW_w_mem( &data[rest], length-rest, location+rest, pn) ;

    /* Copy to scratchpad */
    memcpy( &p[3], data , length ) ;
    BUSLOCK
        ret = BUS_select(pn) || BUS_send_data(p,3+length) ;
        if ( rest==length && ret==0 ) ret = BUS_readin_data( &p[3+length], 2) || CRC16(p,3+length+2) ;
    BUSUNLOCK
    if ( ret ) return 1 ;

    /* Re-read scratchpad and compare */
    p[0] = 0xAA ;
    BUSLOCK
        ret = BUS_select(pn) || BUS_send_data(p,1) || BUS_readin_data(&p[1],3+length) || memcmp(&p[4], data, length) ;
        if ( rest==length && ret==0) ret = BUS_readin_data( &p[4+length], 2) || CRC16(p,4+length+2) ;
    BUSUNLOCK
    if ( ret ) return 1 ;

    /* Copy Scratchpad to SRAM */
    p[0] = 0x55 ;
    BUSLOCK
        ret = BUS_select(pn) || BUS_send_data(p,4) ;
    BUSUNLOCK
    if ( ret ) return 1 ;

    UT_delay(2*length) ;
    return 0 ;
}

static int OW_r_mem( unsigned char * data , const int length , const int location, const struct parsedname * pn ) {
    unsigned char p[3+32+2] = { 0xA5, location&0xFF , location>>8, } ;
    int offset = location & 0x1F ;
    int rest = 32-offset ;
    int ret ;

    if ( offset+length > 32 ) return OW_r_mem( data, (size_t) rest, location, pn) || OW_r_mem( &data[rest], length-rest, location+rest, pn) ;

    BUSLOCK
        ret = BUS_select(pn) || BUS_send_data( p , 3 ) || BUS_readin_data( &p[3], rest+2 ) || CRC16(p,3+rest+2) ;
    BUSUNLOCK
    memcpy( data , &p[3], length ) ;
    return ret ;
}

static int OW_temperature( int * T , const int delay, const struct parsedname * pn ) {
    unsigned char data ;
    int ret ;

    BUSLOCK
        ret =OW_r_mem( &data, 1, 0x0214, pn) ; /* read status register */
    BUSUNLOCK
    if ( ret ) return 1 ;

    if ( (data & 0x20)==0 ) { /* Mission not progress, force conversion */
        BUSLOCK
            ret = BUS_select(pn) || BUS_PowerByte( 0x44, delay ) ;
        BUSUNLOCK
        if ( ret ) return 1 ;
    }

    BUSLOCK
        ret =OW_r_mem( &data, 1, 0x0211, pn) ; /* read temp register */
    BUSUNLOCK
    *T = (int) data ;
    return ret ;
}

/* temperature log */
static int OW_r_logtime(time_t *t, const struct parsedname * pn) {
    int ampm[8] = {0,10,20,30,0,10,12,22} ;
    unsigned char data[8] ;
    struct tm tm ;

    /* Prefill entries */
    *t = time(NULL) ;
    if ( gmtime_r(t,&tm)==NULL ) return -EINVAL ;

    /* Get date from chip */
    if ( OW_r_mem(data,8,0x0215,pn) ) return -EINVAL ;
    tm.tm_sec  = 0 ; /* BCD->dec */
    tm.tm_min  = (data[0]&0x0F) + 10*(data[0]>>4) ; /* BCD->dec */
    tm.tm_hour = (data[1]&0x0F) + ampm[data[1]>>4] ; /* BCD->dec */
    tm.tm_mday = (data[2]&0x0F) + 10*(data[2]>>4) ; /* BCD->dec */
    tm.tm_mon  = (data[3]&0x0F) + 10*((data[3]&0x10)>>4) ; /* BCD->dec */
    tm.tm_year = (data[4]&0x0F) + 10*(data[4]>>4) + 100; /* BCD->dec */

    /* Pass through time_t again to validate */
    if ( (*t=mktime(&tm)) == (time_t)-1 ) return -EINVAL ;

    if ( OW_r_mem(data,2,0x020D,pn) ) return -EINVAL ;
    if ( data[1] & 0x08 ) { /* rollover */
        int u = (((((unsigned int) data[7])<<8)|data[6])<<8)|data[5] ;
        int e = pn->extension ;
        if ( u % 2048 < e ) e-=2048 ;
        *t += 60*data[0]*((u/2048)+e) ;
    } else {
        *t += 60*pn->extension*data[0] ;
    }
    return 0 ;
}

static int OW_clearmemory( const struct parsedname * const pn) {
    unsigned char cr ;
    int ret ;
    /* Clear memory flag */
    if ( OW_r_mem(&cr, 1, 0x020E,pn) ) return -EINVAL ;
    cr = (cr&0x3F) | 0x40 ;
    if ( OW_w_mem( &cr, 1, 0x020E, pn) ) return -EINVAL ;

    /* Clear memory command */
    cr = 0x3C ;
    BUSLOCK
    ret = BUS_select(pn) || BUS_send_data( &cr, 1 ) ;
    BUSUNLOCK
    return ret ;
}
