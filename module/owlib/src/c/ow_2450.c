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
 yREAD_FUNCTION( FS_r_PIO ) ;
yWRITE_FUNCTION( FS_w_PIO ) ;

/* ------- Structures ----------- */

struct aggregate A2450  = { 4, ag_numbers, ag_separate, } ;
struct aggregate A2450v = { 4, ag_letters, ag_aggregate, } ;
struct filetype DS2450[] = {
    F_STANDARD   ,
    {"pages"     ,     0,  NULL,    ft_subdir, ft_volatile, {v:NULL}        , {v:NULL}     , NULL, } ,
    {"pages/page",     8,  &A2450,  ft_binary, ft_stable  , {b:FS_r_page}   , {b:FS_w_page}, NULL, } ,
    {"continuous",     1,  NULL,    ft_yesno , ft_stable  , {b:FS_r_cont}   , {b:FS_w_cont}, NULL, } ,
    {"memory"    ,    32,  NULL,    ft_binary, ft_stable  , {b:FS_r_mem}    , {b:FS_w_mem} , NULL, } ,
    {"PIO"       ,     1,  &A2450v, ft_yesno , ft_stable  , {y:FS_r_PIO}    , {y:FS_w_PIO} , NULL, } ,
    {"volt"      ,    12,  &A2450v, ft_float , ft_volatile, {f:FS_volts}    , {v:NULL}     , (void *) 1, } ,
    {"volt2"     ,    12,  &A2450v, ft_float , ft_volatile, {f:FS_volts}    , {v:NULL}     , (void *) 0, } ,
} ;
DeviceEntry( 20, DS2450 )

/* ------- Functions ------------ */

/* DS2450 */
static int OW_r_mem( char * p , const int size, const int location , const struct parsedname * pn) ;
static int OW_w_mem( const char * p , const int size , const int location , const struct parsedname * pn) ;
static int OW_volts( FLOAT * f , const int resolution , const struct parsedname * pn ) ;
static int OW_r_pio( int * pio , const struct parsedname * pn ) ;
static int OW_r_1_pio( int * pio , const int element , const struct parsedname * pn ) ;
static int OW_w_pio( const int * pio , const struct parsedname * pn ) ;
static int OW_w_1_pio( const int pio , const int element , const struct parsedname * pn ) ;

/* 2450 A/D */
static int FS_r_page(unsigned char *buf, const size_t size, const off_t offset , const struct parsedname * pn) {
    unsigned char p[8] ;
    if ( size+offset>8 ) return -ERANGE ;
    if ( OW_r_mem(p,8,((pn->extension)<<3)+offset,pn) ) return -EINVAL ;
    return FS_read_return(buf,size,offset,p,8) ;
}

/* 2450 A/D */
static int FS_w_page(const unsigned char *buf, const size_t size, const off_t offset , const struct parsedname * pn) {
    if ( size+offset>8 ) return -ERANGE ;
    if ( OW_w_mem(buf,size,((pn->extension)<<3)+offset,pn) ) return -EINVAL ;
    return 0 ;
}

/* 2450 A/D */
static int FS_r_cont(int *y, const struct parsedname * pn) {
    unsigned char p ;
    if ( OW_r_mem(&p,1,0x1C,pn) ) return -EINVAL ;
//printf("Cont %d\n",p) ;
    y[0] = (p==0x40) ;
    return 0 ;
}

/* 2450 A/D */
static int FS_w_cont(const int *y, const struct parsedname * pn) {
    unsigned char p = 0x40 ;
    unsigned char q = 0x00 ;
    if ( OW_w_mem(y[0]?&p:&q,1,0x1C,pn) ) return -EINVAL ;
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
    unsigned char p[8] ;
//printf("2450 r mem: size %d offset %d \n ",size,offset);
    if ( size+offset>4*8 ) return -ERANGE ;
    if ( OW_r_mem(p,size,offset,pn) ) return -EINVAL ;
    return FS_read_return(buf,size,offset,p,8) ;
}

/* 2450 A/D */
static int FS_w_mem(const unsigned char *buf, const size_t size, const off_t offset , const struct parsedname * pn) {
//printf("2450 w mem: size %d offset %d \n ",size,offset);
    if ( size+offset>4*8 ) return -ERANGE ;
    if ( OW_w_mem(buf,size,offset,pn) ) return -EINVAL ;
    return 0 ;
}

/* 2450 A/D */
static int FS_volts(FLOAT * V , const struct parsedname * pn) {
    if ( OW_volts( V , (int) pn->ft->data , pn ) ) return -EINVAL ;
    return 0 ;
}

/* read page from 2450 */
static int OW_r_mem( char * p , const int size, const int location , const struct parsedname * pn) {
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
static int OW_w_mem( const char * p , const int size , const int location, const struct parsedname * pn) {
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
    if ( ret ) return 1 ;

    /* rest of the bytes */
    for ( i=1 ; i<size ; ++i ) {
        buf[0] = p[i] ;
        buf[1] = buf[2] = buf[3] = 0xFF ;
        BUS_lock() ;
            ret =BUS_sendback_data(buf,buf,4) || CRC16seeded(buf,3, location+i) || (buf[3]!=p[i]) ;
        BUS_unlock() ;

        if ( ret ) return 1 ;
    }
    return 0 ;
}

/* Read A/D from 2450 */
/* Note: Sets 16 bits resolution and all 4 channels */
/* resolution is 1->5.10V 0->2.55V */
static int OW_volts( FLOAT * f , const int resolution, const struct parsedname * pn ) {
    unsigned char control[8] ;
    unsigned char data[8] ;
    int i ;
    unsigned char convert[] = { 0x3C , 0x0F , 0x00, 0xFF, 0xFF, } ;
    int ret ;
    // Get control registers and set to A2D 16 bits
    if ( OW_r_mem( control , 8, 1<<3, pn ) ) return 1 ;
//printf("2450 control = %.2X %.2X %.2X %.2X %.2X %.2X %.2X %.2X\n",control[0],control[1],control[2],control[3],control[4],control[5],control[6],control[7]) ;
    for ( i=0; i<8 ; i++ ){
        control[i] = 0x00 ; // 16bit, A/D
        control[++i] &= 0x7F ; // Reset -- leave input range alone    control[0] = 0x00 ; // 16bit, A/D
        control[i] |= (unsigned char) resolution ;
    }
//printf("2450 control = %.2X %.2X %.2X %.2X %.2X %.2X %.2X %.2X\n",control[0],control[1],control[2],control[3],control[4],control[5],control[6],control[7]) ;

    // Set control registers
    if ( OW_w_mem( control, 8, 1<<3, pn) ) return 1 ;

    // Start conversion
	// 6 msec for 16bytex4channel (5.2)
    BUS_lock() ;
        ret = BUS_select(pn) || BUS_sendback_data( convert , data , 5 ) || memcmp( convert , data , 3 ) || CRC16(data,5) || BUS_PowerByte(0x04, 6) ;
    BUS_unlock() ;
    if ( ret ) return 1 ;

    // read data
    BUS_lock() ;
        ret = BUS_readin_data(data,1) || (data[0]!=0xFF) ;
    BUS_unlock() ;
    if ( ret ) return 1 ;

    if ( OW_r_mem( data, 8, 0<<3, pn) ) return 1 ;
//printf("2450 data = %.2X %.2X %.2X %.2X %.2X %.2X %.2X %.2X\n",data[0],data[1],data[2],data[3],data[4],data[5],data[6],data[7]) ;

    // data conversions
    f[0] = (resolution?7.8126192E-5:3.90630961E-5)*((((unsigned int)data[1])<<8)|data[0]) ;
    f[1] = (resolution?7.8126192E-5:3.90630961E-5)*((((unsigned int)data[3])<<8)|data[2]) ;
    f[2] = (resolution?7.8126192E-5:3.90630961E-5)*((((unsigned int)data[5])<<8)|data[4]) ;
    f[3] = (resolution?7.8126192E-5:3.90630961E-5)*((((unsigned int)data[7])<<8)|data[6]) ;
    return 0 ;
}

/* read all the pio registers */
static int OW_r_pio( int * pio , const struct parsedname * pn ) {
    unsigned char p[8] ;
    if ( OW_r_mem(p,8,1<<3,pn) ) return 1;
    pio[0] = ((p[0]&0xC0)==0xC0) ;
    pio[1] = ((p[2]&0xC0)==0xC0) ;
    pio[2] = ((p[4]&0xC0)==0xC0) ;
    pio[3] = ((p[6]&0xC0)==0xC0) ;
}

/* write one pio register */
static int OW_r_1_pio( int * pio , const int element , const struct parsedname * pn ) {
    unsigned char p ;
    if ( OW_r_mem(&p,1,(1<<3)+(element<<1),pn) ) return 1;
    pio[0] = ((p&0xC0)==0xC0) ;
}

/* Write all the pio registers */
static int OW_w_pio( const int * pio , const struct parsedname * pn ) {
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
static int OW_w_1_pio( const int pio , const int element, const struct parsedname * pn ) {
    unsigned char p ;
    if ( OW_r_mem(&p,1,(1<<3)+(element<<1),pn) ) return 1;
    p &= 0x3F ;
    p |= pio?0x80:0xC0 ;
    return OW_w_mem(p,1,(1<<3)+(element<<1),pn) ;
}
