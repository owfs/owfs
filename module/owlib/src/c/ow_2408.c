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

/* LCD drivers, two designs
   Maxim / AAG uses 7 PIO pins
   based on Public domain code from Application Note 3286

   Hobby-Boards by Eric Vickery
   Paul,

Go right ahead and use it for whatever you want. I just provide it as an
example for people who are using the LCD Driver.

It originally came from an application that I was working on (and may
again) but that particular code is in the public domain now.

Let me know if you have any other questions.

Eric
*/

#include <config.h>
#include "owfs_config.h"
#include "ow_2408.h"

/* ------- Prototypes ----------- */

/* DS2408 switch */
 yREAD_FUNCTION( FS_r_strobe ) ;
yWRITE_FUNCTION( FS_w_strobe ) ;
 uREAD_FUNCTION( FS_r_pio ) ;
uWRITE_FUNCTION( FS_w_pio ) ;
 uREAD_FUNCTION( FS_sense ) ;
 yREAD_FUNCTION( FS_power ) ;
 uREAD_FUNCTION( FS_r_latch ) ;
uWRITE_FUNCTION( FS_w_latch ) ;
 uREAD_FUNCTION( FS_r_s_alarm ) ;
uWRITE_FUNCTION( FS_w_s_alarm ) ;
 yREAD_FUNCTION( FS_r_por ) ;
 yWRITE_FUNCTION( FS_w_por ) ;
 yWRITE_FUNCTION( FS_Mclear ) ;
 yWRITE_FUNCTION( FS_Mhome ) ;
 aWRITE_FUNCTION( FS_Mscreen ) ;
 aWRITE_FUNCTION( FS_Mmessage ) ;
 yWRITE_FUNCTION( FS_Hclear ) ;
 yWRITE_FUNCTION( FS_Hhome ) ;
 aWRITE_FUNCTION( FS_Hscreen ) ;
 aWRITE_FUNCTION( FS_Hmessage ) ;

/* ------- Structures ----------- */

struct aggregate A2408 = { 8, ag_numbers, ag_aggregate, } ;
struct filetype DS2408[] = {
    F_STANDARD   ,
    {"power"     ,     1,  NULL,    ft_yesno   , fc_volatile, {y:FS_power}    , {v:NULL},        {v:NULL}, } ,
    {"PIO"       ,     1,  &A2408,  ft_bitfield, fc_stable  , {u:FS_r_pio}    , {u:FS_w_pio},    {v:NULL}, } ,
    {"sensed"    ,     1,  &A2408,  ft_bitfield, fc_volatile, {u:FS_sense}    , {v:NULL},        {v:NULL}, } ,
    {"latch"     ,     1,  &A2408,  ft_bitfield, fc_volatile, {u:FS_r_latch}  , {u:FS_w_latch},  {v:NULL}, } ,
    {"strobe"    ,     1,  NULL,    ft_yesno   , fc_stable  , {y:FS_r_strobe} , {y:FS_w_strobe}, {v:NULL}, } ,
    {"set_alarm" ,     9,  NULL,    ft_unsigned, fc_stable  , {u:FS_r_s_alarm}, {u:FS_w_s_alarm},{v:NULL}, } ,
    {"por"       ,     1,  NULL,    ft_yesno   , fc_stable  , {y:FS_r_por}    , {y:FS_w_por},    {v:NULL}, } ,
    {"LCD_M"     ,     0,  NULL,    ft_subdir  , fc_stable  , {v:NULL}        , {v:NULL}    ,    {v:NULL}, } ,
    {"LCD_M/clear",    1,  NULL,    ft_yesno   , fc_stable  , {v:NULL}        , {y:FS_Mclear},   {v:NULL}, } ,
    {"LCD_M/home",     1,  NULL,    ft_yesno   , fc_stable  , {v:NULL}        , {y:FS_Mhome},    {v:NULL}, } ,
    {"LCD_M/screen", 128,  NULL,    ft_ascii   , fc_stable  , {v:NULL}        , {a:FS_Mscreen},  {v:NULL}, } ,
    {"LCD_M/message",128,  NULL,    ft_ascii   , fc_stable  , {v:NULL}        , {a:FS_Mmessage}, {v:NULL}, } ,
    {"LCD_H"     ,     0,  NULL,    ft_subdir  , fc_stable  , {v:NULL}        , {v:NULL}    ,    {v:NULL}, } ,
    {"LCD_H/clear",    1,  NULL,    ft_yesno   , fc_stable  , {v:NULL}        , {y:FS_Hclear},   {v:NULL}, } ,
    {"LCD_H/home",     1,  NULL,    ft_yesno   , fc_stable  , {v:NULL}        , {y:FS_Hhome},    {v:NULL}, } ,
    {"LCD_H/screen", 128,  NULL,    ft_ascii   , fc_stable  , {v:NULL}        , {a:FS_Hscreen},  {v:NULL}, } ,
    {"LCD_H/message",128,  NULL,    ft_ascii   , fc_stable  , {v:NULL}        , {a:FS_Hmessage}, {v:NULL}, } ,
} ;
DeviceEntryExtended( 29, DS2408, DEV_alarm | DEV_resume | DEV_ovdr ) ;

/* Internal properties */
static struct internal_prop ip_init = {"INI",fc_stable} ;

/* ------- Functions ------------ */

/* DS2408 */
static int OW_w_control( const BYTE data , const struct parsedname * pn ) ;
static int OW_c_latch( const struct parsedname * pn ) ;
static int OW_w_pio( const BYTE data,  const struct parsedname * pn ) ;
static int OW_r_reg( BYTE * data , const struct parsedname * pn ) ;
static int OW_w_s_alarm( const BYTE *data , const struct parsedname * pn ) ;
static int OW_w_pios( const BYTE * data, const size_t size, const struct parsedname * pn ) ;

/* 2408 switch */
/* 2408 switch -- is Vcc powered?*/
static int FS_power(int * y , const struct parsedname * pn) {
    BYTE data[6] ;
    if ( OW_r_reg(data,pn) ) return -EINVAL ;
    *y = UT_getbit(&data[5],7) ;
    return 0 ;
}

static int FS_r_strobe(int * y , const struct parsedname * pn) {
    BYTE data[6] ;
    if ( OW_r_reg(data,pn) ) return -EINVAL ;
    *y = UT_getbit(&data[5],2) ;
    return 0 ;
}

static int FS_w_strobe(const int * y, const struct parsedname * pn) {
    BYTE data[6] ;
    if ( OW_r_reg(data,pn) ) return -EINVAL ;
    UT_setbit( &data[5], 2, y[0] ) ;
    return OW_w_control( data[5] , pn ) ? -EINVAL : 0 ;
}

static int FS_Mclear(const int * y, const struct parsedname * pn) {
    int init = 1 ;
    size_t s = sizeof(init) ;

    if ( Cache_Get_Internal(&init,&s,&ip_init,pn) ) {
        int one = 1 ;
        if ( FS_r_strobe(&one,pn)  // set reset pin to strobe mode
            || OW_w_pio(0x30,pn) ) return -EINVAL ;
        UT_delay(100) ;
        // init
        if ( OW_w_pio(0x38,pn) ) return -EINVAL ;
        UT_delay(10) ;
        // Enable Display, Cursor, and Blinking
         // Entry-mode: auto-increment, no shift
        if ( OW_w_pio(0x0F,pn) || OW_w_pio(0x06,pn) ) return -EINVAL ;
        Cache_Add_Internal(&init,sizeof(init),&ip_init,pn) ;
    }
    // clear
    if ( OW_w_pio(0x01,pn) ) return -EINVAL ;
    UT_delay(2) ;
    return FS_Mhome(y,pn) ;
}

static int FS_Mhome(const int * y, const struct parsedname * pn) {
    // home
    (void) y ;
    if ( OW_w_pio(0x02,pn) ) return -EINVAL ;
    UT_delay(2) ;
    return 0 ;
}

static int FS_Mscreen(const char *buf, const size_t size, const off_t offset , const struct parsedname * pn ) {
    BYTE data[size] ;
    size_t i ;
    (void) offset ;
    for ( i = 0 ; i < size ; ++i ) {
        if ( buf[i] & 0x80 ) return -EINVAL ;
        data[i] = buf[i] | 0x80 ;
    }
    return OW_w_pios( data, size, pn ) ;
}

static int FS_Mmessage(const char *buf, const size_t size, const off_t offset , const struct parsedname * pn ) {
    int y = 1 ;
    if ( FS_Mclear(&y,pn) ) return -EINVAL ;
    return FS_Mscreen(buf,size,offset,pn) ;
}

/* 2408 switch PIO sensed*/
/* From register 0x88 */
static int FS_sense(UINT * u, const struct parsedname * pn) {
    BYTE data[6] ;
    if ( OW_r_reg(data,pn) ) return -EINVAL ;
    u[0] = data[0] ;
    return 0 ;
}

/* 2408 switch PIO set*/
/* From register 0x89 */
static int FS_r_pio(UINT * u , const struct parsedname * pn) {
    BYTE data[6] ;
    if ( OW_r_reg(data,pn) ) return -EINVAL ;
    u[0] = data[1] ^ 0xFF ; /* reverse bits */
    return 0 ;
}

/* 2408 switch PIO change*/
static int FS_w_pio(const UINT * u , const struct parsedname * pn) {
    /* reverse bits */
    if ( OW_w_pio((u[0]&0xFF)^0xFF,pn) ) return -EINVAL ;
    return 0 ;
}

/* 2408 read activity latch */
/* From register 0x8A */
static int FS_r_latch(UINT * u , const struct parsedname * pn) {
    BYTE data[6] ;
    if ( OW_r_reg(data,pn) ) return -EINVAL ;
    u[0] = data[2] ;
    return 0 ;
}

/* 2408 write activity latch */
/* Actually resets them all */
static int FS_w_latch(const UINT * u, const struct parsedname * pn) {
    (void) u ;
    if ( OW_c_latch(pn) ) return -EINVAL ;
    return 0 ;
}

/* 2408 alarm settings*/
/* From registers 0x8B-0x8D */
static int FS_r_s_alarm(UINT * u , const struct parsedname * pn) {
    BYTE d[6] ;
    int i, p ;
    if ( OW_r_reg(d,pn) ) return -EINVAL ;
    /* register 0x8D */
    u[0] = ( d[5] & 0x03 ) * 100000000 ;
    /* registers 0x8B and 0x8C */
    for ( i=0, p=1 ; i<8 ; ++i, p*=10 )
        u[0] += UT_getbit(&d[3],i) | ( UT_getbit(&d[4],i) << 1 ) * p ;
    return 0 ;
}

/* 2408 alarm settings*/
/* First digit source and logic data[2] */
/* next 8 channels */
/* data[1] polarity */
/* data[0] selection  */
static int FS_w_s_alarm(const UINT * u , const struct parsedname * pn) {
    BYTE data[3];
    int i ;
    UINT p ;
    for ( i=0, p=1 ; i<8 ; ++i, p*=10 ) {
        UT_setbit(&data[1],i,((int)(u[0] / p) % 10) & 0x01) ;
        UT_setbit(&data[0],i,(((int)(u[0] / p) % 10) & 0x02) >> 1) ;
    }
    data[2] = ((u[0] / 100000000) % 10) & 0x03 ;
    if ( OW_w_s_alarm(data,pn) ) return -EINVAL ;
    return 0 ;
}

static int FS_r_por(int * y , const struct parsedname * pn) {
    BYTE data[6] ;
    if ( OW_r_reg(data,pn) ) return -EINVAL ;
    *y = UT_getbit(&data[5],3) ;
    return 0 ;
}

static int FS_w_por(const int * y, const struct parsedname * pn) {
    BYTE data[6] ;
    if ( OW_r_reg(data,pn) ) return -EINVAL ;
    UT_setbit( &data[5], 3, y[0] ) ;
    return OW_w_control( data[5] , pn ) ? -EINVAL : 0 ;
}

static int FS_Hclear(const int * y, const struct parsedname * pn) {
    int init = 1 ;
    size_t s = sizeof(init) ;
    // clear, display on, mode
    BYTE clear[6] = { 0x00, 0x10, 0x00, 0xC0, 0x00, 0x60 } ; 
    (void) y ;

    if ( Cache_Get_Internal(&init,&s,&ip_init,pn) ) {
        BYTE data[6] ;
        BYTE setup[] = { 0x30, 0x30, 0x20, 0x20, 0x80 } ;
        if (
            OW_w_control( 0x04 , pn ) // strobe
        || OW_r_reg(data,pn) 
        || ( data[5] != 0x84 )       // not powered
        || OW_c_latch(pn)            // clear PIOs
        || OW_w_pio( 0x30, pn )
        ) return -EINVAL ;
        UT_delay(5) ;
        if ( OW_w_pios(setup, 5, pn ) ) return -EINVAL ;
        Cache_Add_Internal(&init,sizeof(init),&ip_init,pn) ;
    }
    return OW_w_pios( clear, 6 , pn ) ? -EINVAL : 0 ;
}

static int FS_Hhome(const int * y, const struct parsedname * pn) {
    BYTE home[] = { 0x80, 0x00 } ;
    // home
    (void) y ;
    if ( OW_w_pios(home,2,pn) ) return -EINVAL ;
    return 0 ;
}

static int FS_Hscreen(const char *buf, const size_t size, const off_t offset , const struct parsedname * pn ) {
    BYTE data[2*size] ;
    size_t i, j = 0 ;
    (void) offset ;
    for ( i = 0 ; i < size ; ++i ) {
        data[j++] = ( buf[i] & 0xF0 ) | 0x08 ;
        data[j++] = ( (buf[i]<<4) & 0xF0 ) | 0x08 ;
    }
    return OW_w_pios( data, j , pn ) ? -EINVAL : 0 ;
}

static int FS_Hmessage(const char *buf, const size_t size, const off_t offset , const struct parsedname * pn ) {
    int y = 1 ;
    if ( FS_Hclear(&y,pn) || FS_Hhome(&y,pn) || FS_Hscreen(buf,size,offset,pn) ) return -EINVAL ;
    return 0 ;
}

/* Read 6 bytes --
   0x88 PIO logic State
   0x89 PIO output Latch state
   0x8A PIO Activity Latch
   0x8B Conditional Ch Mask
   0x8C Londitional Ch Polarity
   0x8D Control/Status
   plus 2 more bytes to get to the end of the page and qualify for a CRC16 checksum
*/
static int OW_r_reg( BYTE * data , const struct parsedname * pn ) {
    BYTE p[3+8+2] = { 0xF0, 0x88 , 0x00, } ;
    struct transaction_log t[] = {
        TRXN_START,
        { p, NULL, 3, trxn_match } ,
        { NULL, &p[3], 8+2, trxn_read } ,
        TRXN_END,
    } ;

    //printf( "R_REG read attempt\n");
    if ( BUS_transaction( t, pn ) ) return 1 ;
    //printf( "R_REG read ok\n");
    if ( CRC16(p,3+8+2) ) return 1 ;
    //printf( "R_REG CRC16 ok\n");

    memcpy( data , &p[3], 6 ) ;
    return 0 ;
}

static int OW_w_pio( const BYTE data,  const struct parsedname * pn ) {
    BYTE p[] = { 0x5A, data , ~data, } ;
    BYTE r[2] ;
    struct transaction_log t[] = {
        TRXN_START,
        { p, NULL, 3, trxn_match } ,
        { NULL, r, 2, trxn_read } ,
        TRXN_END,
    } ;

    //printf( "W_PIO attempt\n");
    if ( BUS_transaction( t, pn ) ) return 1 ;
    //printf( "W_PIO attempt\n");
    //printf("wPIO data = %2X %2X %2X %2X %2X\n",p[0],p[1],p[2],r[0],r[1]) ;
    if ( r[0]!=0xAA ) return 1 ;
    //printf( "W_PIO 0xAA ok\n");
    /* Ignore byte 5 r[1] the PIO status byte */
    return 0 ;
}

static int OW_w_pios( const BYTE * data, const size_t size, const struct parsedname * pn ) {
    BYTE p[] = { 0x5A, 0 , 0, } ;
    BYTE r[2] ;
    struct transaction_log t[] = {
        TRXN_START,
        { p, NULL, 3, trxn_match } ,
        { NULL, r, 2, trxn_read } ,
        TRXN_END,
    } ;
    size_t i ;

    for ( i = 0 ; i < size ; ++i ) {
        p[1] = data[i] ;
        p[2] = ~p[1] ;
        //printf( "W_PIO attempt\n");
        if ( BUS_transaction( &t[i<1], pn ) ) return 1 ;
        //printf( "W_PIO attempt\n");
        //printf("wPIO data = %2X %2X %2X %2X %2X\n",p[0],p[1],p[2],r[0],r[1]) ;
        if ( r[0]!=0xAA ) return 1 ;
        //printf( "W_PIO 0xAA ok\n");
        /* Ignore byte 5 r[1] the PIO status byte */
    }
    return 0 ;
}

/* Reset activity latch */
static int OW_c_latch(const struct parsedname * pn) {
    BYTE p[] = { 0xC3, } ;
    BYTE r ;
    struct transaction_log t[] = {
        TRXN_START,
        { p, NULL, 1, trxn_match } ,
        { NULL, &r, 1, trxn_read } ,
        TRXN_END,
    } ;

    //printf( "C_LATCH attempt\n");
    if ( BUS_transaction( t, pn ) ) return 1 ;
    //printf( "C_LATCH transact\n");
    if ( r!=0xAA ) return 1 ;
    //printf( "C_LATCH 0xAA ok\n");

    return 0 ;
}

/* Write control/status */
static int OW_w_control( const BYTE data , const struct parsedname * pn ) {
    BYTE d[6] ; /* register read */
    BYTE p[] = { 0xCC, 0x8D, 0x00, data, } ;
    struct transaction_log t[] = {
        TRXN_START,
        { p, NULL, 4, trxn_match } ,
        TRXN_END,
    } ;

    //printf( "W_CONTROL attempt\n");
    if ( BUS_transaction( t, pn ) ) return 1 ;
    //printf( "W_CONTROL ok, now check\n");

    /* Read registers */
    if ( OW_r_reg(d,pn) ) return -EINVAL ;

    return ( (data & 0x0F) != (d[5] & 0x0F) ) ;
}

/* write alarm settings */
static int OW_w_s_alarm( const BYTE *data , const struct parsedname * pn ) {
    BYTE d[6], cr ;
    BYTE a[] = { 0xCC, 0x8D, 0x00, } ;
    struct transaction_log t[] = {
        TRXN_START,
        { a, NULL, 3, trxn_match } ,
        { data, NULL, 2, trxn_match } ,
        { &cr, NULL, 1, trxn_match } ,
        TRXN_END,
    } ;

    // get the existing register contents
    if ( OW_r_reg(d,pn) ) return -EINVAL ;

    //printf("S_ALARM 0x8B... = %.2X %.2X %.2X \n",data[0],data[1],data[2]) ;
    cr = (data[2] & 0x03) | (d[5] & 0x0C) ;
    //printf("S_ALARM adjusted 0x8B... = %.2X %.2X %.2X \n",data[0],data[1],cr) ;

    if ( BUS_transaction( t, pn ) ) return 1 ;

    /* Re-Read registers */
    if ( OW_r_reg(d,pn) ) return 1 ;
    //printf("S_ALARM back 0x8B... = %.2X %.2X %.2X \n",d[3],d[4],d[5]) ;

    return  ( data[0] != d[3] ) || ( data[1] != d[4] ) || ( cr != (d[5] & 0x0F) ) ;
}
