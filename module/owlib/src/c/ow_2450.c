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
#include "ow_2450.h"

/* ------- Prototypes ----------- */

/* DS2450 A/D */
 bREAD_FUNCTION( FS_r_page ) ;
bWRITE_FUNCTION( FS_w_page ) ;
 bREAD_FUNCTION( FS_r_mem ) ;
bWRITE_FUNCTION( FS_w_mem ) ;
 fREAD_FUNCTION( FS_volts ) ;
 yREAD_FUNCTION( FS_r_cont ) ;
yWRITE_FUNCTION( FS_w_cont ) ;
 yREAD_FUNCTION( FS_r_high ) ;
yWRITE_FUNCTION( FS_w_high ) ;
 yREAD_FUNCTION( FS_r_PIO ) ;
yWRITE_FUNCTION( FS_w_PIO ) ;
 fREAD_FUNCTION( FS_r_setvolt ) ;
fWRITE_FUNCTION( FS_w_setvolt ) ;
 yREAD_FUNCTION( FS_r_flag ) ;
yWRITE_FUNCTION( FS_w_flag ) ;
 yREAD_FUNCTION( FS_r_por ) ;
yWRITE_FUNCTION( FS_w_por ) ;

/* ------- Structures ----------- */

struct aggregate A2450  = { 4, ag_numbers, ag_separate, } ;
struct aggregate A2450m = { 4, ag_letters, ag_mixed, } ;
struct aggregate A2450v = { 4, ag_letters, ag_aggregate, } ;
struct filetype DS2450[] = {
    F_STANDARD            ,
    {"pages"              ,     0,  NULL,    ft_subdir, ft_volatile, {v:NULL}        , {v:NULL}        , NULL, } ,
    {"pages/page"         ,     8,  &A2450,  ft_binary, ft_stable  , {b:FS_r_page}   , {b:FS_w_page}   , NULL, } ,
    {"continuous"         ,     1,  NULL,    ft_yesno , ft_stable  , {y:FS_r_cont}   , {y:FS_w_cont}   , NULL, } ,
    {"memory"             ,    32,  NULL,    ft_binary, ft_stable  , {b:FS_r_mem}    , {b:FS_w_mem}    , NULL, } ,
    {"PIO"                ,     1,  &A2450m, ft_yesno , ft_stable  , {y:FS_r_PIO}    , {y:FS_w_PIO}    , NULL, } ,
    {"volt"               ,    12,  &A2450m, ft_float , ft_volatile, {f:FS_volts}    , {v:NULL}        , (void *) 1, } ,
    {"volt2"              ,    12,  &A2450m, ft_float , ft_volatile, {f:FS_volts}    , {v:NULL}        , (void *) 0, } ,
    {"set_alarm"          ,     0,  NULL,    ft_subdir, ft_volatile, {v:NULL}        , {v:NULL}        , NULL, } ,
    {"set_alarm/volthigh" ,    12,  &A2450v, ft_float , ft_stable  , {f:FS_r_setvolt}, {f:FS_w_setvolt}, (void *) 3, } ,
    {"set_alarm/volt2high",    12,  &A2450v, ft_float , ft_stable  , {f:FS_r_setvolt}, {f:FS_w_setvolt}, (void *) 2, } ,
    {"set_alarm/voltlow"  ,    12,  &A2450v, ft_float , ft_stable  , {f:FS_r_setvolt}, {f:FS_w_setvolt}, (void *) 1, } ,
    {"set_alarm/volt2low" ,    12,  &A2450v, ft_float , ft_stable  , {f:FS_r_setvolt}, {f:FS_w_setvolt}, (void *) 0, } ,
    {"set_alarm/high"     ,     1,  &A2450v, ft_yesno , ft_stable  , {y:FS_r_high}   , {y:FS_w_high}   , (void *) 1, } ,
    {"set_alarm/low"      ,     1,  &A2450v, ft_yesno , ft_stable  , {y:FS_r_high}   , {y:FS_w_high}   , (void *) 0, } ,
    {"set_alarm/unset"    ,     1,  NULL,    ft_yesno , ft_stable  , {y:FS_r_por}    , {y:FS_w_por}    , NULL, } ,
    {"alarm"              ,     0,  NULL,    ft_subdir, ft_volatile, {v:NULL}        , {v:NULL}        , NULL, } ,
    {"alarm/high"         ,     1,  &A2450v, ft_yesno , ft_volatile, {y:FS_r_flag}   , {y:FS_w_flag}   , (void *) 1, } ,
    {"alarm/low"          ,     1,  &A2450v, ft_yesno , ft_volatile, {y:FS_r_flag}   , {y:FS_w_flag}   , (void *) 0, } ,
} ;
DeviceEntry( 20, DS2450 )

/* ------- Functions ------------ */

/* DS2450 */
static int OW_r_mem( char * const p , const unsigned int size, const int location , const struct parsedname * const pn) ;
static int OW_w_mem( const char * p , const unsigned int size , const int location , const struct parsedname * const pn) ;
static int OW_volts( FLOAT * const f , const int resolution , const struct parsedname * const pn ) ;
static int OW_1_volts( FLOAT * const f , const int element, const int resolution , const struct parsedname * const pn ) ;
static int OW_convert( const int continuous, const struct parsedname * const pn ) ;
static int OW_r_pio( int * const pio , const struct parsedname * const pn ) ;
static int OW_r_1_pio( int * const pio , const int element , const struct parsedname * const pn ) ;
static int OW_w_pio( const int * const pio , const struct parsedname * const pn ) ;
static int OW_w_1_pio( const int pio , const int element , const struct parsedname * const pn ) ;
static int OW_r_vset( FLOAT * const V , const int high, const int two, const struct parsedname * const pn ) ;
static int OW_w_vset( const FLOAT * const V , const int high, const int two, const struct parsedname * const pn ) ;
static int OW_r_high( unsigned int * const y , const int high, const struct parsedname * const pn ) ;
static int OW_w_high( const unsigned int * const y , const int high, const struct parsedname * const pn ) ;
static int OW_r_flag( unsigned int * const y , const int high, const struct parsedname * const pn ) ;
static int OW_w_flag( const unsigned int * const y , const int high, const struct parsedname * const pn ) ;
static int OW_r_por( unsigned int * const y , const struct parsedname * const pn ) ;
static int OW_w_por( const int por , const struct parsedname * const pn ) ;

/* read a page of memory (8 bytes) */
static int FS_r_page(unsigned char *buf, const size_t size, const off_t offset , const struct parsedname * pn) {
    unsigned char p[8] ;
    size_t s = size ;
    if ( size>8 )  s = 8 ;
    if ( OW_r_mem(buf,size,((pn->extension)<<3)+offset,pn) ) return -EINVAL ;
    return 0 ;
}

/* write an 8-byte page */
static int FS_w_page(const unsigned char *buf, const size_t size, const off_t offset , const struct parsedname * pn) {
    size_t s = size ;
    if ( size>8 )  s = 8 ;
    if ( OW_w_mem(buf,s,((pn->extension)<<3),pn) ) return -EINVAL ;
    return 0 ;
}

/* read "continuous conversion" flag */
static int FS_r_cont(int *y, const struct parsedname * pn) {
    unsigned char p ;
    if ( OW_r_mem(&p,1,0x1C,pn) ) return -EINVAL ;
//printf("Cont %d\n",p) ;
    y[0] = (p==0x40) ;
    return 0 ;
}

/* write "continuous conversion" flag */
static int FS_w_cont(const int *y, const struct parsedname * pn) {
    unsigned char p = 0x40 ; /* continuous */
    unsigned char q = 0x00 ; /* on demand */
    if ( OW_w_mem(y[0]?&p:&q,1,0x1C,pn) ) return -EINVAL ;
    return 0 ;
}

/* read "unset" PowerOnReset flag */
static int FS_r_por(int *y, const struct parsedname * pn) {
    if ( OW_r_por(y,pn) ) return -EINVAL ;
    return 0 ;
}

/* write "unset" PowerOnReset flag */
static int FS_w_por(const int *y, const struct parsedname * pn) {
    if ( OW_w_por(y[0]&0x01,pn) ) return -EINVAL ;
    return 0 ;
}

/* read high/low voltage enable alarm flags */
static int FS_r_high(int *y, const struct parsedname * pn) {
    if ( OW_r_high(y,((int) pn->ft->data)&0x01,pn) ) return -EINVAL ;
    return 0 ;
}

/* write high/low voltage enable alarm flags */
static int FS_w_high(const int *y, const struct parsedname * pn) {
    if ( OW_w_high(y,((int) pn->ft->data)&0x01,pn) ) return -EINVAL ;
    return 0 ;
}

/* read high/low voltage triggered state alarm flags */
static int FS_r_flag(int *y, const struct parsedname * pn) {
    if ( OW_r_flag(y,((int) pn->ft->data)&0x01,pn) ) return -EINVAL ;
    return 0 ;
}

/* write high/low voltage triggered state alarm flags */
static int FS_w_flag(const int *y, const struct parsedname * pn) {
    if ( OW_w_flag(y,((int) pn->ft->data)&0x01,pn) ) return -EINVAL ;
    return 0 ;
}

/* 2450 A/D */
static int FS_r_PIO(int *y, const struct parsedname * pn) {
    int element = pn->extension ;
    if ( (element<0)?OW_r_pio(y,pn):OW_r_1_pio(y,element,pn) ) return -EINVAL ;
    return 0 ;
}

/* 2450 A/D */
/* pio status -- special code for ag_mixed */
static int FS_w_PIO(const int *y, const struct parsedname * pn) {
    int element = pn->extension ;
    if ( (element<0)?OW_w_pio(y,pn):OW_w_1_pio(y[0],element,pn) ) return -EINVAL ;
    return 0 ;
}

/* 2450 A/D */
static int FS_r_mem(unsigned char *buf, const size_t size, const off_t offset , const struct parsedname * pn) {
    if ( OW_r_mem(buf,size,offset,pn) ) return -EINVAL ;
    return 0 ;
}

/* 2450 A/D */
static int FS_w_mem(const unsigned char *buf, const size_t size, const off_t offset , const struct parsedname * pn) {
    size_t s = size ;
    if ( size>32 )  s = 32 ;
    if ( OW_w_mem(buf,s,0,pn) ) return -EINVAL ;
    return 0 ;
}

/* 2450 A/D */
static int FS_volts(FLOAT * V , const struct parsedname * pn) {
    int element = pn->extension ;
    if ( (element<0)?OW_volts( V, (int) pn->ft->data, pn ):OW_1_volts( V, element, (int) pn->ft->data, pn ) ) return -EINVAL ;
    return 0 ;
}

static int FS_r_setvolt( FLOAT * const V, const struct parsedname * const pn ) {
    if ( OW_r_vset( V , ( (int) pn->ft->data ) >>1 , ( (int) pn->ft->data ) & 0x01 , pn ) ) return -EINVAL ;
    return 0 ;
}

static int FS_w_setvolt( const FLOAT * const V, const struct parsedname * const pn ) {
    if ( OW_w_vset( V , ( (int) pn->ft->data ) >>1 , ( (int) pn->ft->data ) & 0x01 , pn ) ) return -EINVAL ;
    return 0 ;
}

/* read page from 2450 */
static int OW_r_mem( char * const p , const unsigned int size, const int location , const struct parsedname * const pn) {
    unsigned char buf[3+8+2] = {0xAA, location&0xFF,(location>>8)&0xFF, } ;
    int thispage = 8-(location&0x07);
    int s = size ;
    int ret ;
//printf("2450 R mem data=%d size=%d location=%d\n",*p,size,location) ;

    /* First page */
    BUS_lock() ;
        ret = BUS_select(pn) || BUS_send_data(buf,3) || BUS_readin_data(&buf[3],thispage+2) || CRC16(buf,thispage+5) ;
    BUS_unlock() ;
    if ( ret ) return 1 ;
//printf("2450 R mem 1\n") ;

    if (s>thispage) s=thispage ;
    memcpy( p , &buf[3] , (size_t) s ) ;

    /* cycle through additional pages */
    for ( s=size-thispage; s>0; s-=thispage ) {
        BUS_lock() ;
            ret = BUS_readin_data(&buf[3],8+2) || CRC16(&buf[3],8+2) ;
        BUS_unlock() ;
        if ( ret ) return 1 ;

        thispage = (s>8) ? 8 : s ;
        memcpy( &p[size-s] , &buf[3] , (size_t) thispage ) ;
    }
    return 0 ;
}

/* write to 2450 */
static int OW_w_mem( const char * const p , const unsigned int size , const int location, const struct parsedname * const pn) {
    // command, address(2) , data , crc(2), databack
    unsigned char buf[] = {0x55, location&0xFF,(location>>8)&0xFF, p[0], 0xFF,0xFF, 0xFF, } ;
    int i ;
    int ret ;
//printf("2450 W mem size=%d location=%d\n",size,location) ;

    if ( size == 0 ) return 0 ;

    /* Send the first byte (handled differently) */
    BUS_lock() ;
        ret = BUS_select(pn) || BUS_sendback_data(buf,buf,7) || CRC16(buf,6) || (buf[6]!=p[0]) ;
    BUS_unlock() ;
printf("2450 byte %d return=%d\n",0,ret );
    if ( ret ) return 1 ;

    /* rest of the bytes */
    for ( i=1 ; i<size ; ++i ) {
        buf[0] = p[i] ;
        buf[1] = buf[2] = buf[3] = 0xFF ;
        BUS_lock() ;
            ret =BUS_sendback_data(buf,buf,4) || CRC16seeded(buf,3, location+i) || (buf[3]!=p[i]) ;
        BUS_unlock() ;
printf("2450 byte %d return=%d\n",i,ret );

        if ( ret ) return 1 ;
    }
    return 0 ;
}

/* Read A/D from 2450 */
/* Note: Sets 16 bits resolution and all 4 channels */
/* resolution is 1->5.10V 0->2.55V */
static int OW_volts( FLOAT * const f , const int resolution, const struct parsedname * const pn ) {
    unsigned char control[8] ;
    unsigned char data[8] ;
    int i ;
    unsigned char cont ;
    int writeback = 0 ; /* write control back? */

    /* get continuous flag -- to see if conversion can be avoided */
    if ( OW_r_mem(&cont,1,0x1C,pn) ) return 1 ;

    // Get control registers and set to A/D 16 bits
    if ( OW_r_mem( control , 8, 1<<3, pn ) ) return 1 ;
//printf("2450 control = %.2X %.2X %.2X %.2X %.2X %.2X %.2X %.2X\n",control[0],control[1],control[2],control[3],control[4],control[5],control[6],control[7]) ;
    for ( i=0; i<8 ; i++ ){ // warning, counter in incremented in loop, too
        if ( control[i] != 0x00 ) {
            control[i] = 0x00 ; // 16bit, A/D
            writeback = 1 ;
        }
        if ( (control[++i]&0x01) != (resolution&0x01) ) {
            UT_setbit(&control[i],0,resolution) ;
            writeback = 1 ;
        }
    }
//printf("2450 control = %.2X %.2X %.2X %.2X %.2X %.2X %.2X %.2X\n",control[0],control[1],control[2],control[3],control[4],control[5],control[6],control[7]) ;

    // Set control registers
    if ( writeback )
        if ( OW_w_mem( control, 8, 1<<3, pn) ) return 1 ;

    // Start A/D process if needed
    if ( writeback || cont!=0x40 )
        if ( OW_convert( cont==0x40, pn ) ) return 1 ;

    // read data
    if ( OW_r_mem( data, 8, 0<<3, pn) ) return 1 ;
//printf("2450 data = %.2X %.2X %.2X %.2X %.2X %.2X %.2X %.2X\n",data[0],data[1],data[2],data[3],data[4],data[5],data[6],data[7]) ;

    // data conversions
    f[0] = (resolution?7.8126192E-5:3.90630961E-5)*((((unsigned int)data[1])<<8)|data[0]) ;
    f[1] = (resolution?7.8126192E-5:3.90630961E-5)*((((unsigned int)data[3])<<8)|data[2]) ;
    f[2] = (resolution?7.8126192E-5:3.90630961E-5)*((((unsigned int)data[5])<<8)|data[4]) ;
    f[3] = (resolution?7.8126192E-5:3.90630961E-5)*((((unsigned int)data[7])<<8)|data[6]) ;
    return 0 ;
}


/* Read A/D from 2450 */
/* Note: Sets 16 bits resolution on a single channel */
/* resolution is 1->5.10V 0->2.55V */
static int OW_1_volts( FLOAT * const f , const int element, const int resolution , const struct parsedname * const pn ) {
    unsigned char control[2] ;
    unsigned char data[2] ;
    unsigned char cont ;
    int writeback = 0 ; /* write control back? */

    /* get continuous flag -- to see if conversion can be avoided */
    if ( OW_r_mem(&cont,1,0x1C,pn) ) return 1 ;

    // Get control registers and set to A/D 16 bits
    if ( OW_r_mem( control , 2, (1<<3)+(element<<1), pn ) ) return 1 ;
    if ( control[0] != 0x00 ) {
        control[0] = 0x00 ; // 16bit, A/D
        writeback = 1 ;
    }
    if ( (control[1]&0x01) != (resolution&0x01) ) {
        UT_setbit(&control[1],0,resolution) ;
        writeback = 1 ;
    }

    // Set control registers
    if ( writeback )
        if ( OW_w_mem( control, 2, (1<<3)+(element<<1), pn) ) return 1 ;

    // Start A/D process if needed
    if ( writeback || cont!=0x40 )
        if ( OW_convert( cont==0x40, pn ) ) return 1 ;

    // read data
    if ( OW_r_mem( data, 2, (0<<3)+(element<<1), pn) ) return 1 ;
//printf("2450 data = %.2X %.2X %.2X %.2X %.2X %.2X %.2X %.2X\n",data[0],data[1],data[2],data[3],data[4],data[5],data[6],data[7]) ;

    // data conversions
    f[0] = (resolution?7.8126192E-5:3.90630961E-5)*((((unsigned int)data[1])<<8)|data[0]) ;
    return 0 ;
}

/* send A/D conversion command */
static int OW_convert( const int continuous, const struct parsedname * const pn ) {
    unsigned char convert[] = { 0x3C , 0x0F , 0x00, 0xFF, 0xFF, } ;
    unsigned char data[5] ;
    int ret ;

    // Start conversion
    // 6 msec for 16bytex4channel (5.2)
    BUS_lock() ;
        ret = BUS_select(pn) || BUS_sendback_data( convert , data , 5 ) || memcmp( convert , data , 3 ) || CRC16(data,5) ;
        if ( ret ) {
    BUS_unlock() ;
        } else if (continuous) {
    BUS_unlock() ;
        UT_delay(6) ; /* don't need to hold line for conversion! */
        } else { /* power line for conversion */
            ret = BUS_PowerByte( 0x04, 6) ;
    BUS_unlock() ;
        }
    return ret ;
}

/* read all the pio registers */
static int OW_r_pio( int * const pio , const struct parsedname * const pn ) {
    unsigned char p[8] ;
    if ( OW_r_mem(p,8,1<<3,pn) ) return 1;
    pio[0] = ((p[0]&0xC0)==0xC0) ;
    pio[1] = ((p[2]&0xC0)==0xC0) ;
    pio[2] = ((p[4]&0xC0)==0xC0) ;
    pio[3] = ((p[6]&0xC0)==0xC0) ;
    return 0;
}

/* read one pio register */
static int OW_r_1_pio( int * const pio , const int element , const struct parsedname * const pn ) {
    unsigned char p[2] ;
    if ( OW_r_mem(p,2,(1<<3)+(element<<1),pn) ) return 1;
    pio[0] = ((p[0]&0xC0)==0xC0) ;
    return 0;
}

/* Write all the pio registers */
static int OW_w_pio( const int * const pio , const struct parsedname * const pn ) {
    unsigned char p[8] ;
    int i ;
    if ( OW_r_mem(p,8,1<<3,pn) ) return 1;
    for (i=0; i<3; ++i ){
        p[i<<1] &= 0x3F ;
        p[i<<1] |= pio[i]?0x80:0xC0 ;
    }
    return OW_w_mem(p,8,1<<3,pn) ;
}
/* write just one pio register */
static int OW_w_1_pio( const int pio , const int element, const struct parsedname * const pn ) {
    unsigned char p[2] ;
    if ( OW_r_mem(p,2,(1<<3)+(element<<1),pn) ) return 1;
    p[0] &= 0x3F ;
    p[0] |= pio?0x80:0xC0 ;
    return OW_w_mem(p,2,(1<<3)+(element<<1),pn) ;
}

static int OW_r_vset( FLOAT * const V , const int high, const int two, const struct parsedname * const pn ) {
    unsigned char p[8] ;
    if ( OW_r_mem(p,8,2<<3,pn) ) return 1;
    V[0] = (two?.01:.02) * p[0+high] ;
    V[1] = (two?.01:.02) * p[2+high] ;
    V[2] = (two?.01:.02) * p[4+high] ;
    V[3] = (two?.01:.02) * p[6+high] ;
    return 0 ;
}

static int OW_w_vset( const FLOAT * const V , const int high, const int two, const struct parsedname * const pn ) {
    unsigned char p[8] ;
    if ( OW_r_mem(p,8,2<<3,pn) ) return 1;
    p[0+high] = V[0] * (two?100.:50.) ;
    p[2+high] = V[1] * (two?100.:50.) ;
    p[4+high] = V[2] * (two?100.:50.) ;
    p[6+high] = V[3] * (two?100.:50.) ;
    if ( OW_w_mem(p,8,2<<3,pn) ) return 1;
    /* turn POR off */
    if ( OW_r_mem(p,8,1<<3,pn) ) return 1;
    UT_setbit(p,15,0) ;
    UT_setbit(p,31,0) ;
    UT_setbit(p,47,0) ;
    UT_setbit(p,63,0) ;
    return OW_w_mem(p,8,1<<3,pn) ;
}

static int OW_r_high( unsigned int * const y , const int high, const struct parsedname * const pn ) {
    unsigned char p[8] ;
    if ( OW_r_mem(p,8,1<<3,pn) ) return 1;
    y[0] = UT_getbit(p, 8+2+high) ;
    y[1] = UT_getbit(p,24+2+high) ;
    y[2] = UT_getbit(p,40+2+high) ;
    y[3] = UT_getbit(p,56+2+high) ;
    return 0 ;
}

static int OW_w_high( const unsigned int * const y , const int high, const struct parsedname * const pn ) {
    unsigned char p[8] ;
    if ( OW_r_mem(p,8,1<<3,pn) ) return 1;
    UT_setbit(p, 8+2+high,(int)y[0]&0x01) ;
    UT_setbit(p,24+2+high,(int)y[1]&0x01) ;
    UT_setbit(p,40+2+high,(int)y[2]&0x01) ;
    UT_setbit(p,56+2+high,(int)y[3]&0x01) ;
    /* turn POR off */
    UT_setbit(p,15,0) ;
    UT_setbit(p,31,0) ;
    UT_setbit(p,47,0) ;
    UT_setbit(p,63,0) ;
    return OW_w_mem(p,8,1<<3,pn) ;
}

static int OW_r_flag( unsigned int * const y , const int high, const struct parsedname * const pn ) {
    unsigned char p[8] ;
    if ( OW_r_mem(p,8,1<<3,pn) ) return 1;
    y[0] = UT_getbit(p, 8+4+high) ;
    y[1] = UT_getbit(p,24+4+high) ;
    y[2] = UT_getbit(p,40+4+high) ;
    y[3] = UT_getbit(p,56+4+high) ;
    return 0 ;
}

static int OW_w_flag( const unsigned int * const y , const int high, const struct parsedname * const pn ) {
    unsigned char p[8] ;
    if ( OW_r_mem(p,8,1<<3,pn) ) return 1;
    UT_setbit(p, 8+4+high,(int)y[0]&0x01) ;
    UT_setbit(p,24+4+high,(int)y[1]&0x01) ;
    UT_setbit(p,40+4+high,(int)y[2]&0x01) ;
    UT_setbit(p,56+4+high,(int)y[3]&0x01) ;
    return OW_w_mem(p,8,1<<3,pn) ;
}

static int OW_r_por( unsigned int * const y , const struct parsedname * const pn ) {
    unsigned char p[8] ;
    if ( OW_r_mem(p,8,1<<3,pn) ) return 1;
    y[0] = UT_getbit(p,15) || UT_getbit(p,31) || UT_getbit(p,47) || UT_getbit(p,63);
    return 0 ;
}

static int OW_w_por( const int por , const struct parsedname * const pn ) {
    unsigned char p[8] ;
    if ( OW_r_mem(p,8,1<<3,pn) ) return 1;
    UT_setbit(p,15,por) ;
    UT_setbit(p,31,por) ;
    UT_setbit(p,47,por) ;
    UT_setbit(p,63,por) ;
    return OW_w_mem(p,8,1<<3,pn) ;
}
