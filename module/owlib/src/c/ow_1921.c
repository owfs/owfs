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
 fREAD_FUNCTION( FS_r_21histotemp ) ;
 fREAD_FUNCTION( FS_r_21histogap ) ;
 fREAD_FUNCTION( FS_r_21resolution ) ;
 aREAD_FUNCTION( FS_r_21version ) ;
 fREAD_FUNCTION( FS_r_21rangelow ) ;
 fREAD_FUNCTION( FS_r_21rangehigh ) ;
 uREAD_FUNCTION( FS_r_21histogram ) ;
 fREAD_FUNCTION( FS_r_21logtemp ) ;
 uREAD_FUNCTION( FS_r_21logtime ) ;
 aREAD_FUNCTION( FS_r_21logdate ) ;
 fREAD_FUNCTION( FS_r_21temperature ) ;

 aREAD_FUNCTION( FS_r_21date ) ;
aWRITE_FUNCTION( FS_w_21date ) ;
 bREAD_FUNCTION( FS_r_21mem ) ;
bWRITE_FUNCTION( FS_w_21mem ) ;
 bREAD_FUNCTION( FS_r_21page ) ;
bWRITE_FUNCTION( FS_w_21page ) ;
 uREAD_FUNCTION( FS_r_21samplerate ) ;
uWRITE_FUNCTION( FS_w_21samplerate ) ;
 yREAD_FUNCTION( FS_r_21run ) ;
yWRITE_FUNCTION( FS_w_21run ) ;
 uREAD_FUNCTION( FS_r_21_3byte ) ;
 uREAD_FUNCTION( FS_r_21atime ) ;
uWRITE_FUNCTION( FS_w_21atime ) ;
 uREAD_FUNCTION( FS_r_21atrig ) ;
uWRITE_FUNCTION( FS_w_21atrig ) ;
 yREAD_FUNCTION( FS_r_21mip ) ;
yWRITE_FUNCTION( FS_w_21mip ) ;
/* ------- Structures ----------- */

struct aggregate A1921p = { 16, ag_numbers, ag_separate, } ;
struct aggregate A1921l = { 2048, ag_numbers, ag_separate, } ;
struct aggregate A1921h = { 63, ag_numbers, ag_aggregate, } ;
struct filetype DS1921[] = {
    F_STANDARD        ,
    {"memory"         ,512,   NULL,   ft_binary,  ft_stable, {b:FS_r_21mem}        , {b:FS_w_21mem}       , NULL, } ,
    {"page"           ,256,&A1921p,   ft_binary,  ft_stable, {b:FS_r_21page}       , {b:FS_w_21page}      , NULL, } ,
    {"histogram"      ,  6,&A1921h, ft_unsigned,ft_volatile, {u:FS_r_21histogram}  , {v:NULL}             , NULL, } ,
    {"histotemp"      ,  6,&A1921h,    ft_float,  ft_static, {f:FS_r_21histotemp}  , {v:NULL}             , NULL, } ,
    {"histogap"       ,  9,   NULL,    ft_float,  ft_static, {f:FS_r_21histogap}   , {v:NULL}             , NULL, } ,
    {"tempresolution" ,  9,   NULL,    ft_float,  ft_static, {f:FS_r_21resolution} , {v:NULL}             , NULL, } ,
    {"temprangelow"   ,  9,   NULL,    ft_float,  ft_static, {f:FS_r_21rangelow}   , {v:NULL}             , NULL, } ,
    {"temprangehigh"  ,  9,   NULL,    ft_float,  ft_static, {f:FS_r_21rangehigh}  , {v:NULL}             , NULL, } ,
    {"temperature"    , 12,   NULL,    ft_float,ft_volatile, {f:FS_r_21temperature}, {v:NULL}             , NULL, } ,
    {"date"           , 24,   NULL,    ft_ascii,  ft_second, {a:FS_r_21date}       , {a:FS_w_21date}      , NULL, } ,
    {"logtemp"        ,  5,&A1921l,    ft_float,ft_volatile, {f:FS_r_21logtemp}    , {v:NULL}             , NULL, } ,
    {"logtime"        , 12,&A1921l, ft_unsigned,ft_volatile, {u:FS_r_21logtime}    , {v:NULL}             , NULL, } ,
    {"logdate"        , 24,&A1921l,    ft_ascii,ft_volatile, {a:FS_r_21logdate}    , {v:NULL}             , NULL, } ,
    {"samplerate"     ,  5,   NULL, ft_unsigned,  ft_stable, {u:FS_r_21samplerate} , {u:FS_w_21samplerate}, NULL, } ,
    {"running"        ,  1,   NULL,    ft_yesno,  ft_stable, {y:FS_r_21run}        , {y:FS_w_21run}       , NULL, } ,
    {"mission_samples", 12,   NULL, ft_unsigned,ft_volatile, {u:FS_r_21_3byte}     , {v:NULL}             , (void *)0x021A, } ,
    {"total_samples"  , 12,   NULL, ft_unsigned,ft_volatile, {u:FS_r_21_3byte}     , {v:NULL}             , (void *)0x021D, } ,
    {"alarm_second"   , 12,   NULL, ft_unsigned,  ft_stable, {u:FS_r_21atime}      , {u:FS_w_21atime}     , (void *)0x0207, } ,
    {"alarm_minute"   , 12,   NULL, ft_unsigned,  ft_stable, {u:FS_r_21atime}      , {u:FS_w_21atime}     , (void *)0x0208, } ,
    {"alarm_hour"     , 12,   NULL, ft_unsigned,  ft_stable, {u:FS_r_21atime}      , {u:FS_w_21atime}     , (void *)0x0209, } ,
    {"alarm_dow"      , 12,   NULL, ft_unsigned,  ft_stable, {u:FS_r_21atime}      , {u:FS_w_21atime}     , (void *)0x020A, } ,
    {"alarm_trigger"  , 12,   NULL, ft_unsigned,  ft_stable, {u:FS_r_21atrig}      , {u:FS_w_21atrig}     , NULL, } ,
    {"in_mission"     ,  1,   NULL,    ft_yesno,ft_volatile, {y:FS_r_21mip}        , {y:FS_w_21mip}       , NULL, } ,
    {"version"        , 11,   NULL,    ft_ascii,  ft_stable, {a:FS_r_21version}    , {v:NULL}             , NULL, } ,
} ;
DeviceEntry( 21, DS1921 )

/* Different version of the Thermocron, sorted by ID[11,12] of name. Keep in sorted order */
struct Version { unsigned int ID ; char * name ; float histolow ; float resolution ; float rangelow ; float rangehigh ; unsigned int delay ;} ;
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
static int OW_w_21mem( const unsigned char * data , const size_t length , const size_t location, const struct parsedname * pn ) ;
static int OW_r_21mem( unsigned char * data , const int length , const int location, const struct parsedname * pn ) ;
static int OW_21temperature( int * T , const int delay, const struct parsedname * pn ) ;
static int OW_r_21logtime(time_t *t, const struct parsedname * pn) ;
static int OW_21clearmemory( const struct parsedname * const pn) ;

/* histogram lower bound */
int FS_r_21histogram(unsigned int * h , const struct parsedname * pn) {
    int j ;
    int i = 0 ;
    unsigned char data[32] ;

    if ( OW_r_21mem( data , 32, 0x0800, pn ) ) return -EINVAL ;
    for ( j=0 ; j<32 ; j+=2 ) h[i++] =  ((unsigned int)data[j+1])<<8 | data[j] ;
//for ( j=0 ; j<16 ; j+=2 ) printf("HIS j=%3d %2X %2X %4X %u\n",j,data[j],data[j+1],((unsigned int)data[j+1])<<8 | data[j],((unsigned int)data[j+1])<<8 | data[j]) ;

    if ( OW_r_21mem( data , 32, 0x0820, pn ) ) return -EINVAL ;
    for ( j=0 ; j<32 ; j+=2 ) h[i++] =  ((unsigned int)data[j+1])<<8 | data[j] ;
//for ( j=0 ; j<16 ; j+=2 ) printf("HIS j=%3d %2X %2X %4X %u\n",j,data[j],data[j+1],((unsigned int)data[j+1])<<8 | data[j],((unsigned int)data[j+1])<<8 | data[j]) ;
    if ( OW_r_21mem( data , 32, 0x0840, pn ) ) return -EINVAL ;
    for ( j=0 ; j<32 ; j+=2 ) h[i++] =  ((unsigned int)data[j+1])<<8 | data[j] ;
//for ( j=0 ; j<16 ; j+=2 ) printf("HIS j=%3d %2X %2X %4X %u\n",j,data[j],data[j+1],((unsigned int)data[j+1])<<8 | data[j],((unsigned int)data[j+1])<<8 | data[j]) ;
    if ( OW_r_21mem( data , 32, 0x0860, pn ) ) return -EINVAL ;
    for ( j=0 ; j<30 ; j+=2 ) h[i++] =  ((unsigned int)data[j+1])<<8 | data[j] ;
//for ( j=0 ; j<14 ; j+=2 ) printf("HIS j=%3d %2X %2X %4X %u\n",j,data[j],data[j+1],((unsigned int)data[j+1])<<8 | data[j],((unsigned int)data[j+1])<<8 | data[j]) ;
//for ( j=0;j<63;++j) printf("HIH j=%3d, %u\n",j,h[j]) ;
    return 0 ;
}

int FS_r_21histotemp(float * h , const struct parsedname * pn) {
    int i ;
    struct Version *v = (struct Version*) bsearch( pn , Versions , VersionElements, sizeof(struct Version), VersionCmp ) ;
    if ( v==NULL ) return -EINVAL ;
    for ( i=0 ; i<63 ; ++i ) h[i] = Temperature(v->histolow + 4*i*v->resolution) ;
    return 0 ;
}

int FS_r_21histogap(float * g , const struct parsedname * pn) {
    struct Version *v = (struct Version*) bsearch( pn , Versions , VersionElements, sizeof(struct Version), VersionCmp ) ;
    if ( v==NULL ) return -EINVAL ;
    *g = TemperatureGap(v->resolution*4) ;
    return 0 ;
}

int FS_r_21version(char *buf, const size_t size, const off_t offset , const struct parsedname * pn) {
    struct Version *v = (struct Version*) bsearch( pn , Versions , VersionElements, sizeof(struct Version), VersionCmp ) ;
    if ( v==NULL ) return -EINVAL ;
    return FS_read_return( buf, size, offset , v->name , strlen(v->name) ) ;
}

int FS_r_21resolution(float * r , const struct parsedname * pn) {
    struct Version *v = (struct Version*) bsearch( pn , Versions , VersionElements, sizeof(struct Version), VersionCmp ) ;
    if ( v==NULL ) return -EINVAL ;
    *r = TemperatureGap(v->resolution) ;
    return 0 ;
}

int FS_r_21rangelow(float * rl , const struct parsedname * pn) {
    struct Version *v = (struct Version*) bsearch( pn , Versions , VersionElements, sizeof(struct Version), VersionCmp ) ;
    if ( v==NULL ) return -EINVAL ;
    *rl = Temperature(v->rangelow) ;
    return 0 ;
}

int FS_r_21rangehigh(float * rh , const struct parsedname * pn) {
    struct Version *v = (struct Version*) bsearch( pn , Versions , VersionElements, sizeof(struct Version), VersionCmp ) ;
    if ( v==NULL ) return -EINVAL ;
    *rh = Temperature(v->rangehigh) ;
    return 0 ;
}

/* Temperature -- force if not in progress */
int FS_r_21temperature(float * T , const struct parsedname * pn) {
    int temp ;
    struct Version *v = (struct Version*) bsearch( pn , Versions , VersionElements, sizeof(struct Version), VersionCmp ) ;
    if ( v==NULL ) return -EINVAL ;
    if ( OW_21temperature( &temp , v->delay, pn ) ) return -EINVAL ;
    *T = Temperature( (float)temp * v->resolution + v->histolow ) ;
    return 0 ;
}

/* read counter */
/* Save a function by shoving address in data field */
int FS_r_21_3byte(unsigned int * u , const struct parsedname * pn) {
    int addr = (int) pn->ft->data ;
    unsigned char data[3] ;
    if ( OW_r_21mem(data,3,addr,pn) ) return -EINVAL ;
    *u = (((((unsigned int)data[2])<<8)|data[1])<<8)|data[0] ;
    return 0 ;
}

/* read clock */
int FS_r_21date(char *buf, const size_t size, const off_t offset , const struct parsedname * pn) {
    int ampm[8] = {0,10,20,30,0,10,12,22} ;
    unsigned char data[7] ;
    char d[26] ;
    struct tm tm ;
    time_t t ;

    /* Prefill entries */
    t = time(NULL) ;
    if ( gmtime_r(&t,&tm)==NULL ) return -EINVAL ;

    /* Get date from chip */
    if ( OW_r_21mem(data,7,0x0200,pn) ) return -EINVAL ;
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
    if ( ctime_r(&t,d)==NULL ) return -EINVAL ;
    return FS_read_return( buf, size, offset, d, 24 ) ;
}

/* set clock */
int FS_w_21date(const char *buf, const size_t size, const off_t offset , const struct parsedname * pn) {
    struct tm tm ;
    unsigned char data[7] ;
    int year ;

    /* Not sure if this is valid, but won't allow offset != 0 at first */
    /* otherwise need a buffer */
    if ( offset ) return -EFAULT ;
//printf("WDATE size=%d, buf=%2X %2X %2X\n",size,buf[0],buf[1],buf[2]);
    if ( size<3 || buf[0]=='\0' ) { /* Set to current time */
        time_t t = time(NULL) ;
        gmtime_r(&t,&tm) ;
    } else if (!( strptime(buf,"%a %b %d %T %Y",&tm) || strptime(buf,"%b %d %T %Y",&tm) || strptime(buf,"%c",&tm) || strptime(buf,"%D %T",&tm) )) {
//printf("WDATE bad\n");
        return -EINVAL ;
    }
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
    return OW_w_21mem(data,7,0x0200,pn)?-EINVAL:0 ;
}

/* stop/start clock running */
int FS_w_21run(const int * y, const struct parsedname * pn) {
    unsigned char cr ;

    if ( OW_r_21mem( &cr, 1, 0x020E, pn) ) return -EINVAL ;
    cr = y[0] ? cr&0x7F : cr|0x80 ;
    if ( OW_w_21mem( &cr, 1, 0x020E, pn) ) return -EINVAL ;
    return 0 ;
}

/* clock running? */
int FS_r_21run(int * y , const struct parsedname * pn) {
    unsigned char cr ;
    if ( OW_r_21mem(&cr, 1, 0x020E,pn) ) return -EINVAL ;
    y[0] = ((cr&0x80)==0) ;
    return 0 ;
}

/* start/stop mission */
int FS_w_21mip(const int * y, const struct parsedname * pn) {
    unsigned char cr ;
    if ( OW_r_21mem(&cr, 1, 0x0214,pn) ) return -EINVAL ;
    if ( y[0] ) { /* start a mission! */
        int clockstate ;
        if ( (cr&0x10) == 0x10 ) return 0 ; /* already in progress */
        /* Make sure the clock is running */
    if ( FS_r_21run( &clockstate, pn ) ) return -EINVAL ;
    if ( clockstate==0 ) {
        clockstate = 1 ;
            if ( FS_w_21run( &clockstate, pn ) ) return -EINVAL ;
        UT_delay(1000) ;
    }
    /* Clear memory */
    if ( OW_21clearmemory(pn ) ) return -EINVAL ;
        if ( OW_r_21mem(&cr, 1, 0x020E,pn) ) return -EINVAL ;
    cr = (cr&0x3F) | 0x40 ;
        if ( OW_w_21mem( &cr, 1, 0x020E, pn) ) return -EINVAL ;
    } else { /* turn off */
        if ( (cr&0x10) == 0x00 ) return 0 ; /* already off */
        cr ^= 0x10 ;
        if ( OW_w_21mem( &cr, 1, 0x0214, pn) ) return -EINVAL ;
    }
    return 0 ;
}

/* mission is progress? */
int FS_r_21mip(int * y , const struct parsedname * pn) {
    unsigned char cr ;
    if ( OW_r_21mem(&cr, 1, 0x0214,pn) ) return -EINVAL ;
    *y = ((cr&0x10)!=0) ;
    return 0 ;
}

/* read the interval between samples during a mission */
int FS_r_21samplerate(unsigned int * u , const struct parsedname * pn) {
    unsigned char data ;
    if ( OW_r_21mem(&data,1,0x020D,pn) ) return -EINVAL ;
    *u = data ;
    return 0 ;
}

/* write the interval between samples during a mission */
/* NOTE: mission is turned OFF to avoid unintensional start! */
int FS_w_21samplerate(const unsigned int * u , const struct parsedname * pn) {
    unsigned char data ;

    if ( OW_r_21mem(&data,1,0x020E,pn) ) return -EFAULT ;
    data |= 0x10 ; /* EM on */
    if ( OW_w_21mem(&data,1,0x020E,pn) ) return -EFAULT ;
    data = u[0] ;
    if (OW_w_21mem(&data,1,0x020D,pn) ) return -EFAULT ;
    return 0 ;
}

/* read the alarm time field (not bit 7, though) */
int FS_r_21atime(unsigned int * u , const struct parsedname * pn) {
    unsigned char data ;
    if ( OW_r_21mem(&data,1,(int) pn->ft->data,pn) ) return -EFAULT ;
    *u = data & 0x7F ;
    return 0 ;
}

/* write one of the alarm fields */
/* NOTE: keep first bit */
int FS_w_21atime(const unsigned int * u , const struct parsedname * pn) {
    unsigned char data ;

    if ( OW_r_21mem(&data,1,(int) pn->ft->data,pn) ) return -EFAULT ;
    data = ( (unsigned char) u ) | (data&0x80) ; /* EM on */
    if ( OW_w_21mem(&data,1,(int) pn->ft->data,pn) ) return -EFAULT ;
    return 0 ;
}

/* read the alarm time field (not bit 7, though) */
int FS_r_21atrig(unsigned int * u , const struct parsedname * pn) {
    unsigned char data[4] ;
    if ( OW_r_21mem(data,4,0x0207,pn) ) return -EFAULT ;
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
int FS_w_21atrig(const unsigned int * u , const struct parsedname * pn) {
    unsigned char data[4] ;
    if ( OW_r_21mem(data,4,0x0207,pn) ) return -EFAULT ;
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
    if ( OW_w_21mem(data,4,0x0207,pn) ) return -EFAULT ;
    return 0 ;
}

int FS_r_21mem(unsigned char *buf, const size_t size, const off_t offset , const struct parsedname * pn) {
    return OW_r_21mem( buf, size, offset, pn ) ? -EFAULT : (int) size ;
}

int FS_w_21mem(const unsigned char *buf, const size_t size, const off_t offset , const struct parsedname * pn) {
    return OW_w_21mem( buf, size, offset, pn ) ? -EFAULT : 0 ;
}

int FS_r_21page(unsigned char *buf, const size_t size, const off_t offset , const struct parsedname * pn) {
    if ( offset+size > 32 ) return -EMSGSIZE ;
    return OW_r_21mem( buf, size, offset+((pn->extension)<<5), pn ) ? -EFAULT : (int) size ;
}

int FS_w_21page(const unsigned char *buf, const size_t size, const off_t offset , const struct parsedname * pn) {
    if ( offset+size > 32 ) return -EMSGSIZE ;
    return OW_w_21mem( buf, size, offset+((pn->extension)<<5), pn ) ? -EFAULT : 0 ;
}

/* temperature log */
int FS_r_21logtemp(float * T , const struct parsedname * pn) {
    unsigned char data ;
    struct Version *v = (struct Version*) bsearch( pn , Versions , VersionElements, sizeof(struct Version), VersionCmp ) ;
    if ( v==NULL ) return -EINVAL ;
    if ( OW_r_21mem( &data , 1, 0x1000+pn->extension, pn ) ) return -EINVAL ;
    *T = (float)data * v->resolution + v->histolow ;
    return 0 ;
}

/* temperature log */
int FS_r_21logtime(unsigned int * u , const struct parsedname * pn) {
    time_t t ;
    if ( OW_r_21logtime( &t, pn ) ) return -EINVAL ;
    *u = t ;
    return 0 ;
}

int FS_r_21logdate(char *buf, const size_t size, const off_t offset , const struct parsedname * pn) {
    time_t t ;
    char d[26] ;
    if ( offset ) return -EINVAL ;
    if ( OW_r_21logtime( &t, pn ) ) return -EINVAL ;
    if ( ctime_r(&t,d)==NULL ) return -EINVAL ;
    return FS_read_return( buf, size, offset, d, 24 ) ;
}


static int OW_w_21mem( const unsigned char * data , const size_t length , const size_t location, const struct parsedname * pn ) {
    unsigned char p[4+32+2] = { 0x0F, location&0xFF , location>>8, } ;
    int offset = location & 0x1F ;
    int rest = 32-offset ;
    int ret ;

    if ( offset+length > 32 ) return OW_w_21mem( data, (size_t) rest, location, pn) || OW_w_21mem( &data[rest], length-rest, location+rest, pn) ;

    /* Copy to scratchpad */
    memcpy( &p[3], data , length ) ;
    BUS_lock() ;
        ret = BUS_select(pn) || BUS_send_data(p,3+length) ;
        if ( rest==length && ret==0 ) ret = BUS_readin_data( &p[3+length], 2) || CRC16(p,3+length+2) ;
    BUS_unlock() ;
    if ( ret ) return 1 ;

    /* Re-read scratchpad and compare */
    p[0] = 0xAA ;
    BUS_lock() ;
        ret = BUS_select(pn) || BUS_send_data(p,1) || BUS_readin_data(&p[1],3+length) || memcmp(&p[4], data, length) ;
        if ( rest==length && ret==0) ret = BUS_readin_data( &p[4+length], 2) || CRC16(p,4+length+2) ;
    BUS_unlock() ;
    if ( ret ) return 1 ;

    /* Copy Scratchpad to SRAM */
    p[0] = 0x55 ;
    BUS_lock() ;
        ret = BUS_select(pn) || BUS_send_data(p,4) ;
    BUS_unlock() ;
    if ( ret ) return 1 ;

    UT_delay(2*length) ;
    return 0 ;
}

static int OW_r_21mem( unsigned char * data , const int length , const int location, const struct parsedname * pn ) {
    unsigned char p[3+32+2] = { 0xA5, location&0xFF , location>>8, } ;
    int offset = location & 0x1F ;
    int rest = 32-offset ;
    int ret ;

    if ( offset+length > 32 ) return OW_r_21mem( data, (size_t) rest, location, pn) || OW_r_21mem( &data[rest], length-rest, location+rest, pn) ;

    BUS_lock() ;
        ret = BUS_select(pn) || BUS_send_data( p , 3 ) || BUS_readin_data( &p[3], rest+2 ) || CRC16(p,3+rest+2) ;
    BUS_unlock() ;
    memcpy( data , &p[3], length ) ;
    return ret ;
}

static int OW_21temperature( int * T , const int delay, const struct parsedname * pn ) {
    unsigned char data ;
    int ret ;

    BUS_lock() ;
        ret =OW_r_21mem( &data, 1, 0x0214, pn) ; /* read status register */
    BUS_unlock() ;
    if ( ret ) return 1 ;

    if ( (data & 0x20)==0 ) { /* Mission not progress, force conversion */
        BUS_lock() ;
            ret = BUS_select(pn) || BUS_PowerByte( 0x44, delay ) ;
        BUS_unlock() ;
        if ( ret ) return 1 ;
    }

    BUS_lock() ;
        ret =OW_r_21mem( &data, 1, 0x0211, pn) ; /* read temp register */
    BUS_unlock() ;
    *T = (int) data ;
    return ret ;
}

/* temperature log */
static int OW_r_21logtime(time_t *t, const struct parsedname * pn) {
    int ampm[8] = {0,10,20,30,0,10,12,22} ;
    unsigned char data[8] ;
    struct tm tm ;

    /* Prefill entries */
    *t = time(NULL) ;
    if ( gmtime_r(t,&tm)==NULL ) return -EINVAL ;

    /* Get date from chip */
    if ( OW_r_21mem(data,8,0x0215,pn) ) return -EINVAL ;
    tm.tm_sec  = 0 ; /* BCD->dec */
    tm.tm_min  = (data[0]&0x0F) + 10*(data[0]>>4) ; /* BCD->dec */
    tm.tm_hour = (data[1]&0x0F) + ampm[data[1]>>4] ; /* BCD->dec */
    tm.tm_mday = (data[2]&0x0F) + 10*(data[2]>>4) ; /* BCD->dec */
    tm.tm_mon  = (data[3]&0x0F) + 10*((data[3]&0x10)>>4) ; /* BCD->dec */
    tm.tm_year = (data[4]&0x0F) + 10*(data[4]>>4) + 100; /* BCD->dec */

    /* Pass through time_t again to validate */
    if ( (*t=mktime(&tm)) == (time_t)-1 ) return -EINVAL ;

    if ( OW_r_21mem(data,2,0x020D,pn) ) return -EINVAL ;
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

static int OW_21clearmemory( const struct parsedname * const pn) {
    unsigned char cr ;
    int ret ;
    /* Clear memory flag */
    if ( OW_r_21mem(&cr, 1, 0x020E,pn) ) return -EINVAL ;
    cr = (cr&0x3F) | 0x40 ;
    if ( OW_w_21mem( &cr, 1, 0x020E, pn) ) return -EINVAL ;

    /* Clear memory command */
    cr = 0x3C ;
    BUS_lock() ;
    ret = BUS_select(pn) || BUS_send_data( &cr, 1 ) ;
    BUS_unlock() ;
    return ret ;
}
