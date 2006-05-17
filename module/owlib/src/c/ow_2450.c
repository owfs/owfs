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
#include "ow_2450.h"

/* ------- Prototypes ----------- */

/* DS2450 A/D */
 bREAD_FUNCTION( FS_r_page ) ;
bWRITE_FUNCTION( FS_w_page ) ;
 bREAD_FUNCTION( FS_r_mem ) ;
bWRITE_FUNCTION( FS_w_mem ) ;
 fREAD_FUNCTION( FS_volts ) ;
 yREAD_FUNCTION( FS_r_power ) ;
yWRITE_FUNCTION( FS_w_power ) ;
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
    {"pages"              ,     0,  NULL,    ft_subdir, ft_volatile, {v:NULL}        , {v:NULL}        , {v:NULL}, } ,
    {"pages/page"         ,     8,  &A2450,  ft_binary, ft_stable  , {b:FS_r_page}   , {b:FS_w_page}   , {v:NULL}, } ,
    {"power"              ,     1,  NULL,    ft_yesno , ft_stable  , {y:FS_r_power}  , {y:FS_w_power}  , {v:NULL}, } ,
    {"memory"             ,    32,  NULL,    ft_binary, ft_stable  , {b:FS_r_mem}    , {b:FS_w_mem}    , {v:NULL}, } ,
    {"PIO"                ,     1,  &A2450m, ft_yesno , ft_stable  , {y:FS_r_PIO}    , {y:FS_w_PIO}    , {i: 0}, } ,
    {"volt"               ,    12,  &A2450m, ft_float , ft_volatile, {f:FS_volts}    , {v:NULL}        , {i: 1}, } ,
    {"volt2"              ,    12,  &A2450m, ft_float , ft_volatile, {f:FS_volts}    , {v:NULL}        , {i: 0}, } ,
    {"set_alarm"          ,     0,  NULL,    ft_subdir, ft_volatile, {v:NULL}        , {v:NULL}        , {v:NULL}, } ,
    {"set_alarm/volthigh" ,    12,  &A2450v, ft_float , ft_stable  , {f:FS_r_setvolt}, {f:FS_w_setvolt}, {i: 3}, } ,
    {"set_alarm/volt2high",    12,  &A2450v, ft_float , ft_stable  , {f:FS_r_setvolt}, {f:FS_w_setvolt}, {i: 2}, } ,
    {"set_alarm/voltlow"  ,    12,  &A2450v, ft_float , ft_stable  , {f:FS_r_setvolt}, {f:FS_w_setvolt}, {i: 1}, } ,
    {"set_alarm/volt2low" ,    12,  &A2450v, ft_float , ft_stable  , {f:FS_r_setvolt}, {f:FS_w_setvolt}, {i: 0}, } ,
    {"set_alarm/high"     ,     1,  &A2450v, ft_yesno , ft_stable  , {y:FS_r_high}   , {y:FS_w_high}   , {i: 1}, } ,
    {"set_alarm/low"      ,     1,  &A2450v, ft_yesno , ft_stable  , {y:FS_r_high}   , {y:FS_w_high}   , {i: 0}, } ,
    {"set_alarm/unset"    ,     1,  NULL,    ft_yesno , ft_stable  , {y:FS_r_por}    , {y:FS_w_por}    , {v:NULL}, } ,
    {"alarm"              ,     0,  NULL,    ft_subdir, ft_volatile, {v:NULL}        , {v:NULL}        , {v:NULL}, } ,
    {"alarm/high"         ,     1,  &A2450v, ft_yesno , ft_volatile, {y:FS_r_flag}   , {y:FS_w_flag}   , {i: 1}, } ,
    {"alarm/low"          ,     1,  &A2450v, ft_yesno , ft_volatile, {y:FS_r_flag}   , {y:FS_w_flag}   , {i: 0}, } ,
} ;
DeviceEntryExtended( 20, DS2450, DEV_volt | DEV_alarm | DEV_ovdr ) ;

/* ------- Functions ------------ */

/* DS2450 */
static int OW_r_mem( BYTE * p , const size_t size, const size_t offset , const struct parsedname * pn) ;
static int OW_w_mem( const BYTE * p , const size_t size , const size_t offset , const struct parsedname * pn) ;
static int OW_volts( FLOAT * f , const int resolution , const struct parsedname * pn ) ;
static int OW_1_volts( FLOAT * f , const int element, const int resolution , const struct parsedname * pn ) ;
static int OW_convert( const struct parsedname * pn ) ;
static int OW_r_pio( int * pio , const struct parsedname * pn ) ;
static int OW_r_1_pio( int * pio , const int element , const struct parsedname * pn ) ;
static int OW_w_pio( const int * pio , const struct parsedname * pn ) ;
static int OW_w_1_pio( const int pio , const int element , const struct parsedname * pn ) ;
static int OW_r_vset( FLOAT * V , const int high, const int resolution, const struct parsedname * pn ) ;
static int OW_w_vset( const FLOAT * V , const int high, const int resolution, const struct parsedname * pn ) ;
static int OW_r_high( UINT * y , const int high, const struct parsedname * pn ) ;
static int OW_w_high( const UINT * y , const int high, const struct parsedname * pn ) ;
static int OW_r_flag( UINT * y , const int high, const struct parsedname * pn ) ;
static int OW_w_flag( const UINT * y , const int high, const struct parsedname * pn ) ;
static int OW_r_por( UINT * y , const struct parsedname * pn ) ;
static int OW_w_por( const int por , const struct parsedname * pn ) ;

/* read a page of memory (8 bytes) */
static int FS_r_page(BYTE *buf, const size_t size, const off_t offset , const struct parsedname * pn) {
    if ( OW_r_mem(buf,size,(int)(((pn->extension)<<3)+offset),pn) ) return -EINVAL ;
    return size ;
}

/* write an 8-byte page */
static int FS_w_page(const BYTE *buf, const size_t size, const off_t offset , const struct parsedname * pn) {
    if ( OW_w_mem(buf,size,(int)(((pn->extension)<<3)+offset),pn) ) return -EINVAL ;
    return 0 ;
}

/* read powered flag */
static int FS_r_power(int *y, const struct parsedname * pn) {
    BYTE p ;
    if ( OW_r_mem(&p,1,0x1C,pn) ) return -EINVAL ;
//printf("Cont %d\n",p) ;
    y[0] = (p==0x40) ;
    return 0 ;
}

/* write powered flag */
static int FS_w_power(const int *y, const struct parsedname * pn) {
    BYTE p = 0x40 ; /* powered */
    BYTE q = 0x00 ; /* parasitic */
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
    if ( OW_r_high(y,pn->ft->data.i&0x01,pn) ) return -EINVAL ;
    return 0 ;
}

/* write high/low voltage enable alarm flags */
static int FS_w_high(const int *y, const struct parsedname * pn) {
    if ( OW_w_high(y,pn->ft->data.i&0x01,pn) ) return -EINVAL ;
    return 0 ;
}

/* read high/low voltage triggered state alarm flags */
static int FS_r_flag(int *y, const struct parsedname * pn) {
    if ( OW_r_flag(y,pn->ft->data.i&0x01,pn) ) return -EINVAL ;
    return 0 ;
}

/* write high/low voltage triggered state alarm flags */
static int FS_w_flag(const int *y, const struct parsedname * pn) {
    if ( OW_w_flag(y,pn->ft->data.i&0x01,pn) ) return -EINVAL ;
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
static int FS_r_mem(BYTE *buf, const size_t size, const off_t offset , const struct parsedname * pn) {
    if ( OW_read_paged( buf, size, offset, pn, 8, OW_r_mem) ) return -EINVAL ;
    return size ;
}

/* 2450 A/D */
static int FS_w_mem(const BYTE *buf, const size_t size, const off_t offset , const struct parsedname * pn) {
    if ( OW_w_mem(buf,size,(int)offset,pn) ) return -EINVAL ;
    return 0 ;
}

/* 2450 A/D */
static int FS_volts(FLOAT * V , const struct parsedname * pn) {
    int element = pn->extension ;
    if ( (element<0)?OW_volts( V, pn->ft->data.i, pn ):OW_1_volts( V, element, pn->ft->data.i, pn ) ) return -EINVAL ;
    return 0 ;
}

static int FS_r_setvolt( FLOAT * V, const struct parsedname * pn ) {
    if ( OW_r_vset( V , ( pn->ft->data.i ) >>1 , ( pn->ft->data.i ) & 0x01 , pn ) ) return -EINVAL ;
    return 0 ;
}

static int FS_w_setvolt( const FLOAT * V, const struct parsedname * pn ) {
    if ( OW_w_vset( V , ( pn->ft->data.i ) >>1 , ( pn->ft->data.i ) & 0x01 , pn ) ) return -EINVAL ;
    return 0 ;
}

/* read up to 1 page from 2450 */
/* cannot span page boundary */
static int OW_r_mem( BYTE * p , const size_t size, const size_t offset , const struct parsedname * pn) {
    BYTE buf[3+8+2] = {0xAA, offset&0xFF,(offset>>8)&0xFF, } ;
    struct transaction_log t[] = {
        TRXN_START ,
        { buf, NULL, 3, trxn_match } ,
        { NULL, &buf[3], size, trxn_read } ,
        TRXN_END,
    } ;

    if ( ((offset+size)&0x07) == 0 ) t[2].size += 2 ; // to end of page
    if ( BUS_transaction( t, pn ) ) return 1 ;
    if ( ((offset+size)&0x07) == 0 && CRC16(buf,size+5) ) return 1 ; // to end of page
    
    memcpy( p , &buf[3] , size ) ;

    return 0 ;
}

/* write to 2450 */
static int OW_w_mem( const BYTE * p , const size_t size , const size_t offset, const struct parsedname * pn) {
    // command, address(2) , data , crc(2), databack
    BYTE buf[7] = {0x55, offset&0xFF,(offset>>8)&0xFF, p[0], } ;
    size_t i ;
    struct transaction_log tfirst[] = {
        TRXN_START ,
        { buf, NULL, 4, trxn_match } ,
        { NULL, &buf[4], 3, trxn_read },
        TRXN_END,
    } ;
    struct transaction_log trest[] = { // note no TRXN_START
        { buf, NULL, 1, trxn_match } ,
        { NULL, &buf[1], 3, trxn_read },
        TRXN_END,
    } ;
//printf("2450 W mem size=%d location=%d\n",size,location) ;

    if ( size == 0 ) return 0 ;

    /* Send the first byte (handled differently) */
    if ( BUS_transaction( tfirst, pn ) || CRC16(buf,6) || (buf[6]!=p[0]) ) return 1 ;
    /* rest of the bytes */
    for ( i=1 ; i<size ; ++i ) {
        buf[0] = p[i] ;
        if ( BUS_transaction( trest, pn ) || CRC16seeded(buf,3,offset+i) || (buf[3]!=p[i]) ) return 1 ;
    }
    return 0 ;
}

/* Read A/D from 2450 */
/* Note: Sets 16 bits resolution and all 4 channels */
/* resolution is 1->5.10V 0->2.55V */
static int OW_volts( FLOAT * f , const int resolution, const struct parsedname * pn ) {
    BYTE control[8] ;
    BYTE data[8] ;
    int i ;
    int writeback = 0 ; /* write control back? */
    //printf("Volts res=%d\n",resolution);
    // Get control registers and set to A/D 16 bits
    if ( OW_r_mem( control , 8, 1<<3, pn ) ) return 1 ;
    //printf("2450 control = %.2X %.2X %.2X %.2X %.2X %.2X %.2X %.2X\n",control[0],control[1],control[2],control[3],control[4],control[5],control[6],control[7]) ;
    for ( i=0; i<8 ; i++ ){ // warning, counter in incremented in loop, too
        if ( control[i]&0x0F ) {
            control[i] &= 0xF0 ; // 16bit, A/D
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
    //printf("writeback=%d\n",writeback);
    // Start A/D process if needed
    if ( OW_convert( pn ) ) {
      LEVEL_DEFAULT("OW_volts: Failed to start conversion\n");
      return 1 ;
    }

    // read data
    if ( OW_r_mem( data, 8, 0<<3, pn) ) {
      LEVEL_DEFAULT("OW_volts: OW_r_mem Failed\n");
      return 1 ;
    }
    //printf("2450 data = %.2X %.2X %.2X %.2X %.2X %.2X %.2X %.2X\n",data[0],data[1],data[2],data[3],data[4],data[5],data[6],data[7]) ;

    // data conversions
    f[0] = (resolution?7.8126192E-5:3.90630961E-5)*((((UINT)data[1])<<8)|data[0]) ;
    f[1] = (resolution?7.8126192E-5:3.90630961E-5)*((((UINT)data[3])<<8)|data[2]) ;
    f[2] = (resolution?7.8126192E-5:3.90630961E-5)*((((UINT)data[5])<<8)|data[4]) ;
    f[3] = (resolution?7.8126192E-5:3.90630961E-5)*((((UINT)data[7])<<8)|data[6]) ;
    return 0 ;
}


/* Read A/D from 2450 */
/* Note: Sets 16 bits resolution on a single channel */
/* resolution is 1->5.10V 0->2.55V */
static int OW_1_volts( FLOAT * f , const int element, const int resolution , const struct parsedname * pn ) {
    BYTE control[2] ;
    BYTE data[2] ;
    int writeback = 0 ; /* write control back? */

    // Get control registers and set to A/D 16 bits
    if ( OW_r_mem( control , 2, (1<<3)+(element<<1), pn ) ) return 1 ;
    if ( control[0]&0x0F ) {
        control[0] &= 0xF0 ; // 16bit, A/D
        writeback = 1 ;
    }
    if ( (control[1]&0x01) != (resolution&0x01) ) {
        UT_setbit(&control[1],0,resolution) ;
        writeback = 1 ;
    }

    // Set control registers
    if ( writeback )
        if ( OW_w_mem( control, 2, (1<<3)+(element<<1), pn) ) return 1 ;

    // Start A/D process
    if ( OW_convert( pn ) ) {
      LEVEL_DEFAULT("OW_1_volts: Failed to start conversion\n");
      return 1 ;
    }

    // read data
    if ( OW_r_mem( data, 2, (0<<3)+(element<<1), pn) ) {
      LEVEL_DEFAULT("OW_volts: OW_r_mem failed\n");
      return 1 ;
    }
//printf("2450 data = %.2X %.2X %.2X %.2X %.2X %.2X %.2X %.2X\n",data[0],data[1],data[2],data[3],data[4],data[5],data[6],data[7]) ;

    // data conversions
    f[0] = (resolution?7.8126192E-5:3.90630961E-5)*((((UINT)data[1])<<8)|data[0]) ;
    return 0 ;
}

/* send A/D conversion command */
static int OW_convert( const struct parsedname * pn ) {
    BYTE convert[] = { 0x3C , 0x0F , 0x00, 0xFF, 0xFF, } ;
    BYTE power ;
    struct transaction_log tpower[] = {
        TRXN_START ,
        { convert, NULL, 3, trxn_match } ,
        { NULL, &convert[3], 2, trxn_read } ,
        TRXN_END,
    } ;
    struct transaction_log tdead[] = {
        TRXN_START ,
        { convert, NULL, 3, trxn_match } ,
        { NULL, &convert[3], 1, trxn_read } ,
        { &convert[4], &convert[4], 6, trxn_power } ,
        TRXN_END,
    } ;
    
    /* get power flag -- to see if pullup can be avoided */
    if ( OW_r_mem(&power,1,0x1C,pn) ) return 1 ;
    
    /* See if a conversion was globally triggered */
    if ( power==0x40 && Simul_Test( simul_volt, 6, pn )==0 ) return 0 ;

    // Start conversion
    // 6 msec for 16bytex4channel (5.2)
    if (power==0x40) { //powered
        if ( BUS_transaction( tpower, pn ) || CRC16(convert,5) ) return 1 ;
        UT_delay(6) ; /* don't need to hold line for conversion! */
    } else {
        if ( BUS_transaction( tdead, pn ) || CRC16(convert,5) ) return 1 ;
    }
    return 0 ;
}

/* read all the pio registers */
static int OW_r_pio( int * pio , const struct parsedname * pn ) {
    BYTE p[8] ;
    if ( OW_r_mem(p,8,1<<3,pn) ) return 1;
    pio[0] = ((p[0]&0xC0)!=0x80) ;
    pio[1] = ((p[2]&0xC0)!=0x80) ;
    pio[2] = ((p[4]&0xC0)!=0x80) ;
    pio[3] = ((p[6]&0xC0)!=0x80) ;
    return 0;
}

/* read one pio register */
static int OW_r_1_pio( int * pio , const int element , const struct parsedname * pn ) {
    BYTE p[2] ;
    if ( OW_r_mem(p,2,(1<<3)+(element<<1),pn) ) return 1;
    pio[0] = ((p[0]&0xC0)!=0x80) ;
    return 0;
}

/* Write all the pio registers */
static int OW_w_pio( const int * pio , const struct parsedname * pn ) {
    BYTE p[8] ;
    int i ;
    if ( OW_r_mem(p,8,1<<3,pn) ) return 1;
    for (i=0; i<=3; ++i ){
        p[i<<1] &= 0x3F ;
        p[i<<1] |= pio[i]?0xC0:0x80 ;
    }
    return OW_w_mem(p,8,1<<3,pn) ;
}
/* write just one pio register */
static int OW_w_1_pio( const int pio , const int element, const struct parsedname * pn ) {
    BYTE p[2] ;
    if ( OW_r_mem(p,2,(1<<3)+(element<<1),pn) ) return 1;
    p[0] &= 0x3F ;
    p[0] |= pio?0xC0:0x80 ;
    return OW_w_mem(p,2,(1<<3)+(element<<1),pn) ;
}

static int OW_r_vset( FLOAT * V , const int high, const int resolution, const struct parsedname * pn ) {
    BYTE p[8] ;
    if ( OW_r_mem(p,8,2<<3,pn) ) return 1;
    V[0] = (resolution?.02:.01) * p[0+high] ;
    V[1] = (resolution?.02:.01) * p[2+high] ;
    V[2] = (resolution?.02:.01) * p[4+high] ;
    V[3] = (resolution?.02:.01) * p[6+high] ;
    return 0 ;
}

static int OW_w_vset( const FLOAT * V , const int high, const int resolution, const struct parsedname * pn ) {
    BYTE p[8] ;
    if ( OW_r_mem(p,8,2<<3,pn) ) return 1;
    p[0+high] = V[0] * (resolution?50.:100.) ;
    p[2+high] = V[1] * (resolution?50.:100.) ;
    p[4+high] = V[2] * (resolution?50.:100.) ;
    p[6+high] = V[3] * (resolution?50.:100.) ;
    if ( OW_w_mem(p,8,2<<3,pn) ) return 1;
    /* turn POR off */
    if ( OW_r_mem(p,8,1<<3,pn) ) return 1;
    UT_setbit(p,15,0) ;
    UT_setbit(p,31,0) ;
    UT_setbit(p,47,0) ;
    UT_setbit(p,63,0) ;
    return OW_w_mem(p,8,1<<3,pn) ;
}

static int OW_r_high( UINT * y , const int high, const struct parsedname * pn ) {
    BYTE p[8] ;
    if ( OW_r_mem(p,8,1<<3,pn) ) return 1;
    y[0] = UT_getbit(p, 8+2+high) ;
    y[1] = UT_getbit(p,24+2+high) ;
    y[2] = UT_getbit(p,40+2+high) ;
    y[3] = UT_getbit(p,56+2+high) ;
    return 0 ;
}

static int OW_w_high( const UINT * y , const int high, const struct parsedname * pn ) {
    BYTE p[8] ;
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

static int OW_r_flag( UINT * y , const int high, const struct parsedname * pn ) {
    BYTE p[8] ;
    if ( OW_r_mem(p,8,1<<3,pn) ) return 1;
    y[0] = UT_getbit(p, 8+4+high) ;
    y[1] = UT_getbit(p,24+4+high) ;
    y[2] = UT_getbit(p,40+4+high) ;
    y[3] = UT_getbit(p,56+4+high) ;
    return 0 ;
}

static int OW_w_flag( const UINT * y , const int high, const struct parsedname * pn ) {
    BYTE p[8] ;
    if ( OW_r_mem(p,8,1<<3,pn) ) return 1;
    UT_setbit(p, 8+4+high,(int)y[0]&0x01) ;
    UT_setbit(p,24+4+high,(int)y[1]&0x01) ;
    UT_setbit(p,40+4+high,(int)y[2]&0x01) ;
    UT_setbit(p,56+4+high,(int)y[3]&0x01) ;
    return OW_w_mem(p,8,1<<3,pn) ;
}

static int OW_r_por( UINT * y , const struct parsedname * pn ) {
    BYTE p[8] ;
    if ( OW_r_mem(p,8,1<<3,pn) ) return 1;
    y[0] = UT_getbit(p,15) || UT_getbit(p,31) || UT_getbit(p,47) || UT_getbit(p,63);
    return 0 ;
}

static int OW_w_por( const int por , const struct parsedname * pn ) {
    BYTE p[8] ;
    if ( OW_r_mem(p,8,1<<3,pn) ) return 1;
    UT_setbit(p,15,por) ;
    UT_setbit(p,31,por) ;
    UT_setbit(p,47,por) ;
    UT_setbit(p,63,por) ;
    return OW_w_mem(p,8,1<<3,pn) ;
}
