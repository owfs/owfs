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
#include "ow_1921.h"

/* ------- Prototypes ----------- */
/* DS1921 Temperature */
 fREAD_FUNCTION( FS_r_histotemp ) ;
 fREAD_FUNCTION( FS_r_histogap ) ;
 uREAD_FUNCTION( FS_r_histoelem ) ;
 fREAD_FUNCTION( FS_r_resolution ) ;
 aREAD_FUNCTION( FS_r_version ) ;
 fREAD_FUNCTION( FS_r_rangelow ) ;
 fREAD_FUNCTION( FS_r_rangehigh ) ;
 uREAD_FUNCTION( FS_r_histogram ) ;
 fREAD_FUNCTION( FS_r_logtemp ) ;
 dREAD_FUNCTION( FS_r_logdate ) ;
 uREAD_FUNCTION( FS_r_logudate ) ;
 uREAD_FUNCTION( FS_logelements ) ;
 fREAD_FUNCTION( FS_r_temperature ) ;
 yREAD_FUNCTION( FS_bitread ) ;
yWRITE_FUNCTION( FS_bitwrite ) ;
 yREAD_FUNCTION( FS_rbitread ) ;
yWRITE_FUNCTION( FS_rbitwrite ) ;
uWRITE_FUNCTION( FS_easystart ) ;

 uREAD_FUNCTION( FS_alarmudate ) ;
 dREAD_FUNCTION( FS_alarmstart ) ;
 dREAD_FUNCTION( FS_alarmend ) ;
 uREAD_FUNCTION( FS_alarmcnt ) ;
 uREAD_FUNCTION( FS_alarmelems ) ;
 fREAD_FUNCTION( FS_r_alarmtemp ) ;
fWRITE_FUNCTION( FS_w_alarmtemp ) ;
 
 dREAD_FUNCTION( FS_mdate ) ;
 uREAD_FUNCTION( FS_umdate ) ;
 dREAD_FUNCTION( FS_r_date ) ;
 dWRITE_FUNCTION( FS_w_date ) ;
 uREAD_FUNCTION( FS_r_counter ) ;
uWRITE_FUNCTION( FS_w_counter ) ;
 uREAD_FUNCTION( FS_r_delay ) ;
uWRITE_FUNCTION( FS_w_delay ) ;
 bREAD_FUNCTION( FS_r_mem ) ;
 bWRITE_FUNCTION( FS_w_mem ) ;
 bREAD_FUNCTION( FS_r_page ) ;
bWRITE_FUNCTION( FS_w_page ) ;
 uREAD_FUNCTION( FS_r_samplerate ) ;
uWRITE_FUNCTION( FS_w_samplerate ) ;
 yREAD_FUNCTION( FS_r_run ) ;
yWRITE_FUNCTION( FS_w_run ) ;
 uREAD_FUNCTION( FS_r_3byte ) ;
 uREAD_FUNCTION( FS_r_atime ) ;
uWRITE_FUNCTION( FS_w_atime ) ;
 uREAD_FUNCTION( FS_r_atrig ) ;
uWRITE_FUNCTION( FS_w_atrig ) ;
yWRITE_FUNCTION( FS_w_mip ) ;

/* ------- Structures ----------- */
struct BitRead { size_t location ; int bit ; } ;
static struct BitRead BitReads[] =
{
    { 0x0214, 7, } , //temperature in progress
    { 0x0214, 5, } , // Mission in progress
    { 0x0214, 4, } , //sample in progress
    { 0x020E, 3, } , // rollover
    { 0x020E, 7, } , // clock running (reversed)
} ;

struct Mission {
    _DATE start ;
    int rollover ;
    int interval ;
    int samples ;
} ;
    
struct aggregate A1921p = { 16, ag_numbers, ag_separate, } ;
struct aggregate A1921l = { 2048, ag_numbers, ag_mixed, } ;
struct aggregate A1921h = { 63, ag_numbers, ag_mixed, } ;
struct aggregate A1921m = { 12, ag_numbers, ag_aggregate, } ;
struct filetype DS1921[] = {
    F_STANDARD              ,
    {"memory"               ,512,   NULL,  ft_binary   ,   fc_stable, {b:FS_r_mem}        , {b:FS_w_mem}       , {v:NULL}, } ,

    {"pages"                ,  0,   NULL,  ft_subdir   , fc_volatile, {v:NULL}            , {v:NULL}           , {v:NULL}, } ,
    {"pages/page"           , 32,&A1921p,  ft_binary   ,   fc_stable, {b:FS_r_page}       , {b:FS_w_page}      , {v:NULL}, } ,

    {"histogram"            ,  0,   NULL,  ft_subdir   , fc_volatile, {v:NULL}            , {v:NULL}           , {v:NULL}, } ,
    {"histogram/counts"     ,  6,&A1921h,ft_unsigned   , fc_volatile, {u:FS_r_histogram}  , {v:NULL}           , {v:NULL}, } ,
    {"histogram/elements"   ,  9,   NULL,ft_unsigned   ,   fc_static, {u:FS_r_histoelem}  , {v:NULL}           , {v:NULL}, } ,
    {"histogram/gap"        ,  9,   NULL,  ft_tempgap  ,   fc_static, {f:FS_r_histogap}   , {v:NULL}           , {v:NULL}, } ,
    {"histogram/temperature",  6,&A1921h,ft_temperature,   fc_static, {f:FS_r_histotemp}  , {v:NULL}           , {v:NULL}, } ,

    {"clock"                ,  0,   NULL,  ft_subdir   , fc_volatile, {v:NULL}            , {v:NULL}           , {v:NULL}, } ,
    {"clock/date"           , 24,   NULL,    ft_date   ,   fc_second, {d:FS_r_date}       , {d:FS_w_date}      , {v:NULL}, } ,
    {"clock/udate"          , 12,   NULL,ft_unsigned   ,   fc_second, {u:FS_r_counter}    , {u:FS_w_counter}   , {v:NULL}, } ,
    {"clock/running"        ,  1,   NULL,   ft_yesno   ,   fc_stable, {y:FS_rbitread}     , {y:FS_rbitwrite}   , {v:&BitReads[4]}, } ,

    {"about"                ,  0,   NULL,  ft_subdir   , fc_volatile, {v:NULL}            , {v:NULL}           , {v:NULL}, } ,
    {"about/resolution"     ,  9,   NULL, ft_tempgap   ,   fc_static, {f:FS_r_resolution} , {v:NULL}           , {v:NULL}, } ,
    {"about/templow"        ,  9,   NULL,ft_temperature,   fc_static, {f:FS_r_rangelow}   , {v:NULL}           , {v:NULL}, } ,
    {"about/temphigh"       ,  9,   NULL,ft_temperature,   fc_static, {f:FS_r_rangehigh}  , {v:NULL}           , {v:NULL}, } ,
    {"about/version"        , 11,   NULL,   ft_ascii   ,   fc_stable, {a:FS_r_version}    , {v:NULL}           , {v:NULL}, } ,
    {"about/samples"        , 11,   NULL,ft_unsigned   , fc_volatile, {u:FS_r_3byte}      , {v:NULL}           , {s:0x021D}, } ,
    {"about/measuring"      ,  1,   NULL,   ft_yesno   , fc_volatile, {y:FS_rbitread}     , {v:NULL}           , {v:&BitReads[0]}, } ,

    {"temperature"          , 12,   NULL,ft_temperature, fc_volatile, {f:FS_r_temperature}, {v:NULL}           , {v:NULL}, } ,

    {"mission"              ,  0,   NULL,  ft_subdir   , fc_volatile, {v:NULL}            , {v:NULL}           , {v:NULL}, } ,
    {"mission/running"      ,  1,   NULL,   ft_yesno   , fc_volatile, {y:FS_bitread}      , {y:FS_w_mip}       , {v:&BitReads[1]}, } ,
    {"mission/frequency"    ,  1,   NULL,   ft_yesno   , fc_volatile, {u:FS_r_samplerate} , {u:FS_w_samplerate}, {v:NULL}, } ,
    {"mission/samples"      , 12,   NULL,ft_unsigned   , fc_volatile, {u:FS_r_3byte}      , {v:NULL}           , {s:0x021A}, } ,
    {"mission/delay"        , 12,   NULL,ft_unsigned   , fc_volatile, {u:FS_r_delay}      , {u:FS_w_delay}     , {v:NULL}, } ,
    {"mission/rollover"     ,  1,   NULL,   ft_yesno   ,   fc_stable, {y:FS_bitread}      , {y:FS_bitwrite}    , {v:&BitReads[3]}, } ,
    {"mission/date"         , 24,   NULL,    ft_date   , fc_volatile, {d:FS_mdate}        , {v:NULL}           , {v:NULL}, } ,
    {"mission/udate"        , 12,   NULL,ft_unsigned   , fc_volatile, {u:FS_umdate}       , {v:NULL}           , {v:NULL}, } ,
    {"mission/sampling"     ,  1,   NULL,   ft_yesno   , fc_volatile, {y:FS_bitread}      , {v:NULL}           , {v:&BitReads[2]}, } ,
    {"mission/easystart"    , 12,   NULL,ft_unsigned   ,   fc_stable, {v:NULL}            , {u:FS_easystart}   , {v:NULL}, } ,

    {"overtemp"              ,  0,   NULL,  ft_subdir  , fc_volatile, {v:NULL}            , {v:NULL}           , {v:NULL}, } ,
    {"overtemp/date"         , 24,&A1921m,   ft_date   , fc_volatile, {d:FS_alarmstart}   , {v:NULL}           , {s:0x0250}, } ,
    {"overtemp/udate"        , 12,&A1921m,ft_unsigned  , fc_volatile, {u:FS_alarmudate}   , {v:NULL}           , {s:0x0250}, } ,
    {"overtemp/end"          , 24,&A1921m,   ft_date   , fc_volatile, {d:FS_alarmend}     , {v:NULL}           , {s:0x0250}, } ,
    {"overtemp/count"        , 12,&A1921m,ft_unsigned  , fc_volatile, {u:FS_alarmcnt}     , {v:NULL}           , {s:0x0250}, } ,
    {"overtemp/elements"     , 12,   NULL,ft_unsigned  , fc_volatile, {u:FS_alarmelems}   , {v:NULL}           , {s:0x0250}, } ,
    {"overtemp/temperature"  , 12,   NULL,ft_temperature,  fc_stable, {f:FS_r_alarmtemp}  , {f:FS_w_alarmtemp} , {s:0x020C}, } ,

    {"undertemp"             ,  0,   NULL,  ft_subdir  , fc_volatile, {v:NULL}            , {v:NULL}           , {v:NULL}, } ,
    {"undertemp/date"        , 24,&A1921m,   ft_date   , fc_volatile, {d:FS_alarmstart}   , {v:NULL}           , {s:0x0220}, } ,
    {"undertemp/udate"       , 12,&A1921m,ft_unsigned  , fc_volatile, {u:FS_alarmudate}   , {v:NULL}           , {s:0x0220}, } ,
    {"undertemp/end"         , 24,&A1921m,   ft_date   , fc_volatile, {d:FS_alarmend}     , {v:NULL}           , {s:0x0220}, } ,
    {"undertemp/count"       , 12,&A1921m,ft_unsigned  , fc_volatile, {u:FS_alarmcnt}     , {v:NULL}           , {s:0x0220}, } ,
    {"undertemp/elements"    , 12,   NULL,ft_unsigned  , fc_volatile, {u:FS_alarmelems}   , {v:NULL}           , {s:0x0220}, } ,
    {"undertemp/temperature" , 12,   NULL,ft_temperature,  fc_stable, {f:FS_r_alarmtemp}  , {f:FS_w_alarmtemp} , {s:0x020C}, } ,

    {"log"                  ,  0,   NULL,  ft_subdir   , fc_volatile, {v:NULL}            , {v:NULL}           , {v:NULL}, } ,
    {"log/temperature"      ,  5,&A1921l,ft_temperature, fc_volatile, {f:FS_r_logtemp}    , {v:NULL}           , {v:NULL}, } ,
    {"log/date"             , 24,&A1921l,   ft_date    , fc_volatile, {d:FS_r_logdate}    , {v:NULL}           , {v:NULL}, } ,
    {"log/udate"            , 12,&A1921l,ft_unsigned   , fc_volatile, {u:FS_r_logudate}   , {v:NULL}           , {v:NULL}, } ,
    {"log/elements"         , 12,   NULL,ft_unsigned   , fc_volatile, {u:FS_logelements}  , {v:NULL}           , {v:NULL}, } ,

    {"set_alarm"            ,  0,   NULL,  ft_subdir, fc_volatile, {v:NULL}            , {v:NULL}           , {v:NULL}, } ,
    {"set_alarm/trigger"    ,  0,   NULL,  ft_subdir, fc_volatile, {v:NULL}            , {v:NULL}           , {v:NULL}, } ,
    {"set_alarm/templow"    ,  0,   NULL,  ft_subdir, fc_volatile, {v:NULL}            , {v:NULL}           , {v:NULL}, } ,
    {"set_alarm/temphigh"   ,  0,   NULL,  ft_subdir, fc_volatile, {v:NULL}            , {v:NULL}           , {v:NULL}, } ,
    {"set_alarm/date"       ,  0,   NULL,  ft_subdir, fc_volatile, {v:NULL}            , {v:NULL}           , {v:NULL}, } ,

    {"alarm_state"          ,  5,   NULL,ft_unsigned,   fc_stable, {u:FS_r_samplerate} , {u:FS_w_samplerate}, {v:NULL}, } ,

    {"running"        ,  1,   NULL,    ft_yesno,  fc_stable, {y:FS_r_run}        , {y:FS_w_run}       , {v:NULL}, } ,
    {"alarm_second"   , 12,   NULL, ft_unsigned,  fc_stable, {u:FS_r_atime}      , {u:FS_w_atime}     , {s:0x0207}, } ,
    {"alarm_minute"   , 12,   NULL, ft_unsigned,  fc_stable, {u:FS_r_atime}      , {u:FS_w_atime}     , {s:0x0208}, } ,
    {"alarm_hour"     , 12,   NULL, ft_unsigned,  fc_stable, {u:FS_r_atime}      , {u:FS_w_atime}     , {s:0x0209}, } ,
    {"alarm_dow"      , 12,   NULL, ft_unsigned,  fc_stable, {u:FS_r_atime}      , {u:FS_w_atime}     , {s:0x020A}, } ,
    {"alarm_trigger"  , 12,   NULL, ft_unsigned,  fc_stable, {u:FS_r_atrig}      , {u:FS_w_atrig}     , {v:NULL}, } ,
} ;
DeviceEntryExtended( 21, DS1921, DEV_alarm | DEV_temp | DEV_ovdr ) ;

/* Different version of the Thermocron, sorted by ID[11,12] of name. Keep in sorted order */
struct Version { UINT ID ; char * name ; _FLOAT histolow ; _FLOAT resolution ; _FLOAT rangelow ; _FLOAT rangehigh ; UINT delay ;} ;
static struct Version Versions[] =
    {
        { 0x000, "DS1921G-F5" , -40.0, 0.500, -40., +85.,  90, } ,
        { 0x064, "DS1921L-F50", -40.0, 0.500, -40., +85., 300, } ,
        { 0x15C, "DS1921L-F53", -40.0, 0.500, -30., +85., 300, } ,
        { 0x254, "DS1921L-F52", -40.0, 0.500, -20., +85., 300, } ,
        { 0x34C, "DS1921L-F51", -40.0, 0.500, -10., +85., 300, } ,
        { 0x3B2, "DS1921Z-F5" ,  -5.5, 0.125,  -5., +26., 360, } ,
        { 0x4F2, "DS1921H-F5" , +14.5, 0.125, +15., +46., 360, } ,
    } ;
    /* AM/PM for hours field */    
const int ampm[8] = {0,10,20,30,0,10,12,22} ;
#define VersionElements ( sizeof(Versions) / sizeof(struct Version) )
static int VersionCmp( const void * pn , const void * version ) {
    return ( ((((const struct parsedname *)pn)->sn[5])>>4) |  (((UINT)((const struct parsedname *)pn)->sn[6])<<4) ) - ((const struct Version *)version)->ID ;
}

/* ------- Functions ------------ */

/* DS1921 */
static int OW_w_mem( const BYTE * data , const size_t length , const off_t offset, const struct parsedname * pn ) ;
static int OW_r_mem( BYTE * data , const size_t size , const off_t offset, const struct parsedname * pn ) ;
static int OW_temperature( int * T , const UINT delay, const struct parsedname * pn ) ;
static int OW_clearmemory( const struct parsedname * pn) ;
static int OW_2date(_DATE * d, const BYTE * data) ;
static int OW_2mdate(_DATE * d, const BYTE * data) ;
static void OW_date(const _DATE * d , BYTE * data) ;
static int OW_MIP( const struct parsedname * pn ) ;
static int OW_FillMission( struct Mission * m , const struct parsedname * pn ) ;
static int OW_alarmlog( int * t, int * c, const off_t offset, const struct parsedname * pn ) ;
static int OW_stopmission( const struct parsedname * pn ) ;
static int OW_startmission( UINT freq, const struct parsedname * pn ) ;

static int FS_bitread( int * y , const struct parsedname * pn ) {
    BYTE d ;
    struct BitRead * br ;
    if (pn->ft->data.v==NULL) return -EINVAL ;
    br = ((struct BitRead *)(pn->ft->data.v)) ;
    if ( OW_r_mem_crc16(&d,1,br->location,pn,32) ) return -EINVAL ;
    y[0] = UT_getbit(&d,br->bit ) ;
    return 0 ;
}

static int FS_bitwrite( const int * y , const struct parsedname * pn ) {
    BYTE d ;
    struct BitRead * br ;
    if (pn->ft->data.v==NULL) return -EINVAL ;
    br = ((struct BitRead *)(pn->ft->data.v)) ;
    if ( OW_r_mem_crc16(&d,1,br->location,pn,32) ) return -EINVAL ;
    UT_setbit(&d,br->bit,y[0] ) ;
    if ( OW_w_mem(&d,1,br->location,pn) ) return -EINVAL ;
    return 0 ;
}

static int FS_rbitread( int * y , const struct parsedname * pn ) {
    int ret = FS_bitread(y,pn) ;
    y[0] = !y[0] ;
    return ret ;
}

static int FS_rbitwrite( const int * y , const struct parsedname * pn ) {
    int z = !y[0] ;
    return FS_bitwrite(&z,pn) ;
}

/* histogram counts */
static int FS_r_histogram(UINT * h , const struct parsedname * pn) {
    if ( pn->extension < 0 ) { /* ALL */
        int i ;
        BYTE data[63*2] ;
        if ( OW_read_paged(data,sizeof(data),0x800,pn,32,OW_r_mem) ) return -EINVAL ;
        for ( i=0 ; i<63 ; ++i ) {
            h[i] = (((UINT)data[(i<<1)+1])<<8)|data[i<<1] ;
        }
    } else { /* single element */
        BYTE data[2] ;
        if ( OW_r_mem_crc16(data,2,(size_t)0x800+((pn->extension)<<1),pn,32) ) return -EINVAL ;
        h[0] = (((UINT)data[1])<<8)|data[0] ;
    }
    return 0 ;
}
    
static int FS_r_histotemp(_FLOAT * h , const struct parsedname * pn) {
    struct Version *v = (struct Version*) bsearch( pn , Versions , VersionElements, sizeof(struct Version), VersionCmp ) ;
    if ( v==NULL ) return -EINVAL ;
    if ( pn->extension < 0 ) { /* ALL */
        int i ;
        for ( i=0 ; i<63 ; ++i ) h[i] = v->histolow + 4*i*v->resolution ;
    } else { /* element */
        h[0] = v->histolow + 4*(pn->extension)*v->resolution ;
    }
    return 0 ;
}

static int FS_r_histogap(_FLOAT * g , const struct parsedname * pn) {
    struct Version *v = (struct Version*) bsearch( pn , Versions , VersionElements, sizeof(struct Version), VersionCmp ) ;
    if ( v==NULL ) return -EINVAL ;
    *g = v->resolution*4 ;
    return 0 ;
}
static int FS_r_histoelem(UINT * u , const struct parsedname * pn) {
    (void) pn ;
    u[0] = 63 ;
    return 0 ;
}


static int FS_r_version(char *buf, const size_t size, const off_t offset , const struct parsedname * pn) {
    struct Version *v = (struct Version*) bsearch( pn , Versions , VersionElements, sizeof(struct Version), VersionCmp ) ;
    return v?FS_output_ascii( buf, size, offset, v->name, strlen(v->name) ):-ENOENT ;
}

static int FS_r_resolution(_FLOAT * r , const struct parsedname * pn) {
    struct Version *v = (struct Version*) bsearch( pn , Versions , VersionElements, sizeof(struct Version), VersionCmp ) ;
    if ( v==NULL ) return -EINVAL ;
    r[0] = v->resolution ;
    return 0 ;
}

static int FS_r_rangelow(_FLOAT * rl , const struct parsedname * pn) {
    struct Version *v = (struct Version*) bsearch( pn , Versions , VersionElements, sizeof(struct Version), VersionCmp ) ;
    if ( v==NULL ) return -EINVAL ;
    rl[0] = v->rangelow ;
    return 0 ;
}

static int FS_r_rangehigh(_FLOAT * rh , const struct parsedname * pn) {
    struct Version *v = (struct Version*) bsearch( pn , Versions , VersionElements, sizeof(struct Version), VersionCmp ) ;
    if ( v==NULL ) return -EINVAL ;
    rh[0] = v->rangehigh ;
    return 0 ;
}

/* Temperature -- force if not in progress */
static int FS_r_temperature(_FLOAT * T , const struct parsedname * pn) {
    int temp ;
    struct Version *v = (struct Version*) bsearch( pn , Versions , VersionElements, sizeof(struct Version), VersionCmp ) ;
    if ( v==NULL ) return -EINVAL ;
    if ( OW_MIP(pn) ) return -EBUSY ; /* Mission in progress */
    if ( OW_temperature( &temp , v->delay, pn ) ) return -EINVAL ;
    T[0] = (_FLOAT)temp * v->resolution + v->histolow ;
    return 0 ;
}

/* read counter */
/* Save a function by shoving address in data field */
static int FS_r_3byte(UINT * u , const struct parsedname * pn) {
    size_t addr = pn->ft->data.s ;
    BYTE data[3] ;
    if ( OW_r_mem_crc16(data,3,addr,pn,32) ) return -EINVAL ;
    u[0] = (((((UINT)data[2])<<8)|data[1])<<8)|data[0] ;
    return 0 ;
}

/* mission start date */
static int FS_mdate(_DATE * d , const struct parsedname * pn) {
    struct Mission mission ;

    if ( OW_FillMission( &mission, pn ) ) return -EINVAL ;
    /* Get date from chip */
    d[0] = mission.start ;
    return 0 ;
}

/* mission start date */
static int FS_umdate(UINT * u , const struct parsedname * pn) {
    struct Mission mission ;

    if ( OW_FillMission( &mission, pn ) ) return -EINVAL ;
    /* Get date from chip */
    u[0] = mission.start ;
    return 0 ;
}

static int FS_alarmelems(UINT * u , const struct parsedname * pn) {
    struct Mission mission ;
    int t[12] ;
    int c[12] ;
    int i ;

    if ( OW_FillMission( &mission, pn ) ) return -EINVAL ;
    if ( OW_alarmlog(t,c,pn->ft->data.s,pn ) ) return -EINVAL ;
    for ( i=0 ; i<12 ; ++i ) if ( c[i]==0 ) break ;
    u[0] = i ;
    return 0 ;
}

/* Temperature -- force if not in progress */
static int FS_r_alarmtemp(_FLOAT * T , const struct parsedname * pn) {
    BYTE data[1] ;
    struct Version *v = (struct Version*) bsearch( pn , Versions , VersionElements, sizeof(struct Version), VersionCmp ) ;
    if ( v==NULL ) return -EINVAL ;
    if ( OW_r_mem_crc16(data,1,pn->ft->data.s,pn,32) ) return -EINVAL ;
    T[0] = (_FLOAT)data[0] * v->resolution + v->histolow ;
    return 0 ;
}

/* Temperature -- force if not in progress */
static int FS_w_alarmtemp(const _FLOAT * T , const struct parsedname * pn) {
    BYTE data[1] ;
    struct Version *v = (struct Version*) bsearch( pn , Versions , VersionElements, sizeof(struct Version), VersionCmp ) ;
    if ( v==NULL ) return -EINVAL ;
    if ( OW_MIP(pn) ) return -EBUSY ;
    data[0] = ( T[0] - v->histolow ) / v->resolution ;
    return ( OW_w_mem(data,1,pn->ft->data.s,pn) ) ? -EINVAL : 0 ;
}

static int FS_alarmudate(UINT * u , const struct parsedname * pn) {
    struct Mission mission ;
    int t[12] ;
    int c[12] ;
    int i ;

    if ( OW_FillMission( &mission, pn ) ) return -EINVAL ;
    if ( OW_alarmlog(t,c,pn->ft->data.s,pn ) ) return -EINVAL ;
    for ( i=0 ; i<12 ; ++i ) u[i] = mission.start + t[i]*mission.interval ;
    return 0 ;
}

static int FS_alarmstart(_DATE * d , const struct parsedname * pn) {
    struct Mission mission ;
    int t[12] ;
    int c[12] ;
    int i ;

    if ( OW_FillMission( &mission, pn ) ) return -EINVAL ;
    if ( OW_alarmlog(t,c,pn->ft->data.s,pn ) ) return -EINVAL ;
    for ( i=0 ; i<12 ; ++i ) d[i] = mission.start + t[i]*mission.interval ;
    return 0 ;
}

static int FS_alarmend(_DATE * d , const struct parsedname * pn) {
    struct Mission mission ;
    int t[12] ;
    int c[12] ;
    int i ;

    if ( OW_FillMission( &mission, pn ) ) return -EINVAL ;
    if ( OW_alarmlog(t,c,pn->ft->data.s,pn ) ) return -EINVAL ;
    for ( i=0 ; i<12 ; ++i ) d[i] = mission.start + (t[i]+c[i])*mission.interval ;
    return 0 ;
}

static int FS_alarmcnt(UINT * u , const struct parsedname * pn) {
    int t[12] ;
    int c[12] ;
    int i ;

    if ( OW_alarmlog(t,c,pn->ft->data.s,pn ) ) return -EINVAL ;
    for ( i=0 ; i<12 ; ++i ) u[i] = c[i] ;
    return 0 ;
}

/* read clock */
static int FS_r_date(_DATE * d , const struct parsedname * pn) {
    BYTE data[7] ;

    /* Get date from chip */
    if ( OW_r_mem_crc16(data,7,0x0200,pn,32) ) return -EINVAL ;
    return OW_2date(d,data) ;
}

/* read clock */
static int FS_r_counter(UINT * u , const struct parsedname * pn) {
    BYTE data[7] ;
    _DATE d ;
    int ret ;

    /* Get date from chip */
    if ( OW_r_mem_crc16(data,7,0x0200,pn,32) ) return -EINVAL ;
    if ( (ret=OW_2date(&d,data)) ) return ret ;
    u[0]  = (UINT) d ;
    return 0 ;
}

/* set clock */
static int FS_w_date(const _DATE * d , const struct parsedname * pn) {
    BYTE data[7] ;
    int y = 1 ;

    /* Busy if in mission */
    if ( OW_MIP(pn) ) return -EBUSY ;

    OW_date( d, data ) ;
    if ( OW_w_mem(data,7,0x0200,pn) ) return -EINVAL ;
    return FS_w_run(&y,pn) ;
}

static int FS_w_counter(const UINT * u , const struct parsedname * pn) {
    BYTE data[7] ;
    _DATE d = (_DATE) u[0] ;

    /* Busy if in mission */
    if ( OW_MIP(pn) ) return -EBUSY ;

    OW_date( &d, data ) ;
    return OW_w_mem(data,7,0x0200,pn)?-EINVAL:0 ;
}

/* stop/start clock running */
static int FS_w_run(const int * y, const struct parsedname * pn) {
    BYTE cr ;

    if ( OW_r_mem_crc16( &cr, 1, 0x020E, pn,32) ) return -EINVAL ;
    cr = y[0] ? cr&0x7F : cr|0x80 ;
    if ( OW_w_mem( &cr, 1, 0x020E, pn) ) return -EINVAL ;
    return 0 ;
}

/* clock running? */
static int FS_r_run(int * y , const struct parsedname * pn) {
    BYTE cr ;
    if ( OW_r_mem_crc16(&cr, 1, 0x020E,pn,32) ) return -EINVAL ;
    y[0] = ((cr&0x80)==0) ;
    return 0 ;
}

/* start/stop mission */
static int FS_w_mip(const int * y, const struct parsedname * pn) {
    if ( y[0] ) { /* start a mission! */
        BYTE data ;
        if ( OW_r_mem_crc16(&data, 1, 0x020D,pn,32) ) return -EINVAL ;
        return OW_startmission( (UINT) data, pn ) ;
    } else {
        return OW_stopmission(pn ) ;
    }
}

/* read the interval between samples during a mission */
static int FS_r_samplerate(UINT * u , const struct parsedname * pn) {
    BYTE data ;
    if ( OW_r_mem_crc16(&data,1,0x020D,pn,32) ) return -EINVAL ;
    *u = data ;
    return 0 ;
}

/* write the interval between samples during a mission */
static int FS_w_samplerate(const UINT * u , const struct parsedname * pn) {
    if ( u[0] ) {
        return OW_startmission( u[0], pn ) ;
    } else {
        return OW_stopmission( pn ) ;
    }
}

/* read the alarm time field (not bit 7, though) */
static int FS_r_atime(UINT * u , const struct parsedname * pn) {
    BYTE data ;
    if ( OW_r_mem_crc16(&data,1,pn->ft->data.s,pn,32) ) return -EFAULT ;
    *u = data & 0x7F ;
    return 0 ;
}

/* write one of the alarm fields */
/* NOTE: keep first bit */
static int FS_w_atime(const UINT * u , const struct parsedname * pn) {
    BYTE data ;

    if ( OW_r_mem_crc16(&data,1,pn->ft->data.s,pn,32) ) return -EFAULT ;
    data = ( (BYTE) u[0] ) | (data&0x80) ; /* EM on */
    if ( OW_w_mem(&data,1,pn->ft->data.s,pn) ) return -EFAULT ;
    return 0 ;
}

/* read the alarm time field (not bit 7, though) */
static int FS_r_atrig(UINT * u , const struct parsedname * pn) {
    BYTE data[4] ;
    if ( OW_r_mem_crc16(data,4,0x0207,pn,32) ) return -EFAULT ;
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

static int FS_r_mem(BYTE *buf, const size_t size, const off_t offset , const struct parsedname * pn) {
    if ( OW_read_paged( buf, size, offset, pn, 32 , OW_r_mem ) ) return -EINVAL ;
    return size ;
}

static int FS_w_mem(const BYTE *buf, const size_t size, const off_t offset , const struct parsedname * pn) {
    if ( OW_write_paged( buf, size, offset, pn, 32, OW_w_mem ) ) return -EINVAL ;
    return 0 ;
}

static int FS_w_atrig(const UINT * u , const struct parsedname * pn) {
    BYTE data[4] ;
    if ( OW_r_mem_crc16(data,4,0x0207,pn,32) ) return -EFAULT ;
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

static int FS_r_page(BYTE *buf, const size_t size, const off_t offset , const struct parsedname * pn) {
    if ( OW_r_mem_crc16( buf, size, (size_t)(offset+((pn->extension)<<5)), pn,32 ) ) return -EINVAL ;
    return size ;
}

static int FS_w_page(const BYTE *buf, const size_t size, const off_t offset , const struct parsedname * pn) {
    if ( OW_w_mem( buf, size, (size_t)(offset+((pn->extension)<<5)), pn ) ) return -EINVAL ;
    return 0 ;
}

/* temperature log */
static int FS_logelements( UINT * u , const struct parsedname * pn) {
    struct Mission mission ;

    if ( OW_FillMission( &mission , pn ) ) return -EINVAL ;

    u[0] = (mission.samples>2048) ? 2048 : mission.samples ;
    return 0 ;
}

/* temperature log */
static int FS_r_logdate( _DATE * d , const struct parsedname * pn) {
    struct Mission mission ;
    int pass=0 ;

    if ( OW_FillMission( &mission , pn ) ) return -EINVAL ;
    if ( mission.rollover) pass = mission.samples >> 11 ; // samples/2048

    if ( pn->extension> -1 ) {
        if ( pass ) {
            d[0] = mission.start + (mission.samples-2048 - pn->extension) * mission.interval ;
        } else {
            d[0] = mission.start + pn->extension * mission.interval ;
        }
    } else {
        int i ;
        if ( pass ) {
            for (i=0;i<2048;++i) d[i] = mission.start + (mission.samples-2048 - i) * mission.interval ;
        } else {
            for (i=0;i<2048;++i) d[i] = mission.start + i * mission.interval ;
        }
    }
    return 0 ;
}

/* mission delay */
static int FS_r_delay( UINT * u , const struct parsedname * pn) {
    BYTE data[2] ;
    if ( OW_r_mem_crc16( data, 2, (size_t)0x0212, pn,32 ) ) return -EINVAL ;
    u[0] = (((UINT)data[1])<<8) | data[0] ;
    return 0 ;
}
    
/* mission delay */
static int FS_w_delay( const UINT * u , const struct parsedname * pn) {
    BYTE data[] = { u[0]&0xFF, (u[0]>>8)&0xFF, } ;
    if ( OW_MIP(pn) ) return -EBUSY ;
    if ( OW_w_mem( data, 2, (size_t)0x0212, pn ) ) return -EINVAL ;
    return 0 ;
}
    
/* temperature log */
static int FS_r_logudate( UINT * u , const struct parsedname * pn) {
    struct Mission mission ;
    int pass=0 ;

    if ( OW_FillMission( &mission , pn ) ) return -EINVAL ;
    if ( mission.rollover) pass = mission.samples >> 11 ; // samples/2048

    if ( pn->extension> -1 ) {
        if ( pass ) {
            u[0] = mission.start + (mission.samples-2048 - pn->extension) * mission.interval ;
        } else {
            u[0] = mission.start + pn->extension * mission.interval ;
        }
    } else {
        int i ;
        if ( pass ) {
            for (i=0;i<2048;++i) u[i] = mission.start + (mission.samples-2048 - i) * mission.interval ;
        } else {
            for (i=0;i<2048;++i) u[i] = mission.start + i * mission.interval ;
        }
    }
    return 0 ;
}

/* temperature log */
static int FS_r_logtemp(_FLOAT * T , const struct parsedname * pn) {
    struct Mission mission ;
    int pass=0 ;
    int off=0 ;
    struct Version *v = (struct Version*) bsearch( pn , Versions , VersionElements, sizeof(struct Version), VersionCmp ) ;
    
    if ( v==NULL ) return -EINVAL ;

    if ( OW_FillMission( &mission , pn ) ) return -EINVAL ;
    if ( mission.rollover) {
        pass = mission.samples >> 11 ; // samples/2048
        off = mission.samples & 0x07FF ; // samples%2048
    }

    if ( pn->extension> -1 ) {
        BYTE data[1] ;
        if ( pass ) {
            if ( OW_r_mem_crc16( data , 1,(size_t) (0x1000+((pn->extension+off)&0x07FF)), pn,32 ) ) return -EINVAL ;
        } else {
            if ( OW_r_mem_crc16( data , 1,(size_t) (0x1000+pn->extension), pn,32 ) ) return -EINVAL ;
        }
        T[0] = (_FLOAT)data[0] * v->resolution + v->histolow ;
    } else {
        int i ;
        BYTE data[1] ;
        if ( OW_read_paged( data, 2048, 0x1000, pn, 32 , OW_r_mem ) ) return -EINVAL ;
        if ( pass ) {
            for (i=0;i<2048;++i) T[i] = (_FLOAT)data[(i+off)&0x07FF] * v->resolution + v->histolow ;
        } else {
            for (i=0;i<2048;++i) T[i] = (_FLOAT)data[i] * v->resolution + v->histolow ;
        }
    }
    return 0 ;
}

static int FS_easystart( const UINT * u, const struct parsedname * pn ) {
    /* write 0x020E -- 0x0214 */
    BYTE data[] = { 0x86, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, } ;
    
    /* Stop clock, no rollover, no delay, temp alarms on, alarms cleared */
    if ( OW_w_mem(data, 7, 0x020E, pn ) ) return -EINVAL ;

    return OW_startmission( u[0], pn ) ;
}

static int OW_w_mem( const BYTE * data , const size_t size , const off_t offset, const struct parsedname * pn ) {
    BYTE p[3+1+32+2] = { 0x0F, offset&0xFF , (offset>>8)&0xFF, } ;
    size_t rest = 32 - (offset&0x1F) ;
    int ret ;

    /* Copy to scratchpad -- use CRC16 if write to end of page, but don't force it */
    memcpy( &p[3], data , size ) ;
    if ( (offset+size)&0x1F ) { /* to end of page */
        BUSLOCK(pn);
            ret = BUS_select(pn) || BUS_send_data( p,3+size,pn) ;
        BUSUNLOCK(pn);
    } else {
        BUSLOCK(pn);
            ret = BUS_select(pn) || BUS_send_data( p,3+size,pn) || BUS_readin_data(&p[3+size],2,pn) || CRC16(p,3+size+2) ;
        BUSUNLOCK(pn);
    }
    if ( ret ) return 1 ;

    /* Re-read scratchpad and compare */
    /* Note: location of data has now shifted down a byte for E/S register */
    p[0] = 0xAA ;
    BUSLOCK(pn);
        ret = BUS_select(pn) || BUS_send_data( p,3,pn) || BUS_readin_data( &p[3],1+rest+2,pn) || CRC16(p,4+rest+2) || memcmp(&p[4], data, size) ;
    BUSUNLOCK(pn);
            if ( ret ) return 1 ;

    /* Copy Scratchpad to SRAM */
    p[0] = 0x55 ;
    BUSLOCK(pn);
        ret = BUS_select(pn) || BUS_send_data( p,4,pn) ;
    BUSUNLOCK(pn);
    if ( ret ) return 1 ;

    UT_delay(1) ; /* 1 msec >> 2 usec per byte */
    return 0 ;
}

/* Pre-paged */
/* read memory as "pages" with CRC16 check */
/* Only read within a page */
static int OW_r_mem( BYTE * data , const size_t size , const off_t offset, const struct parsedname * pn ) {
    return OW_r_mem_crc16(data,size,offset,pn,32) ;
}

static int OW_temperature( int * T , const UINT delay, const struct parsedname * pn ) {
    BYTE data = 0x44 ;
    int ret ;

    /* Mission not progress, force conversion */
    BUSLOCK(pn);
        ret = BUS_select(pn) || BUS_send_data( &data,1,pn ) ;
    BUSUNLOCK(pn);
    if ( ret ) return 1 ;
    
    /* Thermochron is powered (internally by battery) -- no reason to hold bus */
    UT_delay(delay) ;

    ret = OW_r_mem_crc16( &data, 1, 0x0211, pn, 32) ; /* read temp register */
    *T = (int) data ;
    return ret ;
}

static int OW_clearmemory( const struct parsedname * pn) {
    BYTE cr ;
    int ret ;
    /* Clear memory flag */
    if ( OW_r_mem_crc16(&cr, 1, 0x020E,pn,32) ) return -EINVAL ;
    cr = (cr&0x3F) | 0x40 ;
    if ( OW_w_mem( &cr, 1, 0x020E, pn) ) return -EINVAL ;

    /* Clear memory command */
    cr = 0x3C ;
    BUSLOCK(pn);
    ret = BUS_select(pn) || BUS_send_data( &cr, 1,pn ) ;
    BUSUNLOCK(pn);

    UT_delay(1) ; /* wait 500 usec */
    return ret ;
}

/* translate 7 byte field to a Unix-style date (number) */
static int OW_2date(_DATE * d, const BYTE * data) {
    struct tm t ;

    /* Prefill entries */
    d[0] = time(NULL) ;
    if ( gmtime_r(d,&t)==NULL ) return -EINVAL ;

    /* Get date from chip */
    t.tm_sec  = (data[0]&0x0F) + 10*(data[0]>>4) ; /* BCD->dec */
    t.tm_min  = (data[1]&0x0F) + 10*(data[1]>>4) ; /* BCD->dec */
    t.tm_hour = (data[2]&0x0F) + ampm[data[2]>>4] ; /* BCD->dec */
    t.tm_mday = (data[4]&0x0F) + 10*(data[4]>>4) ; /* BCD->dec */
    t.tm_mon  = (data[5]&0x0F) + 10*((data[5]&0x10)>>4) ; /* BCD->dec */
    t.tm_year = (data[6]&0x0F) + 10*(data[6]>>4) + 100*(2-(data[5]>>7)); /* BCD->dec */
//printf("_DATE_READ data=%2X, %2X, %2X, %2X, %2X, %2X, %2X\n",data[0],data[1],data[2],data[3],data[4],data[5],data[6]);
//printf("_DATE: sec=%d, min=%d, hour=%d, mday=%d, mon=%d, year=%d, wday=%d, isdst=%d\n",tm.tm_sec,tm.tm_min,tm.tm_hour,tm.tm_mday,tm.tm_mon,tm.tm_year,tm.tm_wday,tm.tm_isdst) ;

    /* Pass through time_t again to validate */
    if ( (d[0]=mktime(&t)) == (time_t)-1 ) return -EINVAL ;
    return 0 ;
}

/* translate m byte field to a Unix-style date (number) */
static int OW_2mdate(_DATE * d, const BYTE * data) {
    struct tm t ;
    int year ;
//printf("M_DATE data=%.2X %.2X %.2X %.2X %.2X\n",data[0],data[1],data[2],data[3],data[4]);
    /* Prefill entries */
    d[0] = time(NULL) ;
    if ( gmtime_r(d,&t)==NULL ) return -EINVAL ;
    year = t.tm_year ;
//printf("M_DATE year=%d\n",year);

    /* Get date from chip */
    t.tm_sec  = 0 ; /* BCD->dec */
    t.tm_min  = (data[0]&0x0F) +  10*(data[0]>>4) ; /* BCD->dec */
    t.tm_hour = (data[1]&0x0F) + ampm[data[1]>>4] ; /* BCD->dec */
    t.tm_mday = (data[2]&0x0F) +  10*(data[2]>>4) ; /* BCD->dec */
    t.tm_mon  = (data[3]&0x0F) + 10*((data[3]&0x10)>>4) ; /* BCD->dec */
    t.tm_year = (data[4]&0x0F) +  10*(data[4]>>4) ; /* BCD->dec */
//printf("M_DATE tm=(%d-%d-%d %d:%d:%d)\n",t.tm_year,t.tm_mon,t.tm_mday,t.tm_hour,t.tm_min,t.tm_sec);
    /* Adjust the century -- should be within 50 years of current */
    while ( t.tm_year+50 < year ) t.tm_year += 100 ;
//printf("M_DATE tm=(%d-%d-%d %d:%d:%d)\n",t.tm_year,t.tm_mon,t.tm_mday,t.tm_hour,t.tm_min,t.tm_sec);
    
    /* Pass through time_t again to validate */
    if ( (d[0]=mktime(&t)) == (time_t)-1 ) return -EINVAL ;
    return 0 ;
}

/* set clock */
static void OW_date(const _DATE * d , BYTE * data) {
    struct tm tm ;
    int year ;

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
//printf("_DATE_WRITE data=%2X, %2X, %2X, %2X, %2X, %2X, %2X\n",data[0],data[1],data[2],data[3],data[4],data[5],data[6]);
//printf("_DATE: sec=%d, min=%d, hour=%d, mday=%d, mon=%d, year=%d, wday=%d, isdst=%d\n",tm.tm_sec,tm.tm_min,tm.tm_hour,tm.tm_mday,tm.tm_mon,tm.tm_year,tm.tm_wday,tm.tm_isdst) ;
}

/* many things are disallowed if mission in progress */
/* returns 1 if MIP, 0 if not, <0 if error */
static int OW_MIP( const struct parsedname * pn ) {
    BYTE data ;
    int ret = OW_r_mem_crc16( &data, 1, 0x0214, pn,32) ; /* read status register */
    
    if ( ret ) return -EINVAL ;
    return UT_getbit(&data,5) ;
}

static int OW_FillMission( struct Mission * mission , const struct parsedname * pn ) {
    BYTE data[16] ;

    /* Get date from chip */
    if ( OW_r_mem_crc16(data,16,0x020D,pn,32) ) return -EINVAL ;
//printf("FILL data= %.2x %.2X %.2X %.2x  %.2x %.2X %.2X %.2x  %.2x %.2X %.2X %.2x  %.2x %.2X %.2X %.2x\n",data[0],data[1],data[2],data[3],data[4],data[5],data[6],data[7],data[8],data[9],data[10],data[11],data[12],data[13],data[14],data[15]);
    mission->interval = 60 * (int)data[0] ;
    mission->rollover = UT_getbit(&data[1],3) ;
    mission->samples = (((((UINT)data[15])<<8)|data[14])<<8)|data[13] ;
    return OW_2mdate(&(mission->start),&data[8]) ;
}

static int OW_alarmlog( int * t, int * c, const off_t offset, const struct parsedname * pn ) {
    BYTE data[48] ;
    int i,j=0 ;
    
    if ( OW_read_paged( data, 48, offset, pn, 32 , OW_r_mem ) ) return -EINVAL ;

    for ( i=0 ; i<12 ; ++i ) {
        t[i] = (((((UINT)data[j+2])<<8)|data[j+1])<<8)|data[j] ;
        c[i] = data[j+3] ;
        j += 4 ;
    }
    return 0 ;
}

static int OW_stopmission( const struct parsedname * pn ) {
    BYTE data = 0x00 ; /* dummy */
    return OW_r_mem_crc16( &data, 1, 0x0210, pn,32 ) ;
}

static int OW_startmission( UINT freq, const struct parsedname * pn ) {
    BYTE data ;
   
    /* stop the mission */
    if ( OW_stopmission( pn )  ) return -EINVAL; /* stop */
    
    if ( freq==0 ) return 0 ; /* stay stopped */
    
    if ( freq > 255 ) return -ERANGE ; /* Bad interval */

    if ( OW_r_mem_crc16( &data, 1, 0x020E, pn,32 ) ) return -EINVAL ;
    if ( (data&0x80) ) { /* clock stopped */
        _DATE d = time(NULL) ;
        /* start clock */
        if ( FS_w_date(&d,pn ) ) return -EINVAL ; /* set the clock to current time */
        UT_delay(1000) ; /* wait for the clock to count a second */
    }

    /* clear memory */
    if ( OW_clearmemory(pn) ) return -EINVAL ;

    /* finally, set the sample interval (to start the mission) */
    data = freq & 0xFF ;
    return OW_w_mem( &data, 1, 0x020D, pn ) ;
}

