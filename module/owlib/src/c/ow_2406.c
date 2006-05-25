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

/* Changes
    7/2004 Extensive improvements based on input from Serg Oskin
*/

#include <config.h>
#include "owfs_config.h"
#include "ow_2406.h"

/* ------- Prototypes ----------- */

/* DS2406 switch */
 bREAD_FUNCTION( FS_r_mem ) ;
bWRITE_FUNCTION( FS_w_mem ) ;
 bREAD_FUNCTION( FS_r_page ) ;
bWRITE_FUNCTION( FS_w_page ) ;
 uREAD_FUNCTION( FS_r_pio ) ;
uWRITE_FUNCTION( FS_w_pio ) ;
 uREAD_FUNCTION( FS_sense ) ;
 uREAD_FUNCTION( FS_r_latch ) ;
uWRITE_FUNCTION( FS_w_latch ) ;
 uREAD_FUNCTION( FS_r_s_alarm ) ;
uWRITE_FUNCTION( FS_w_s_alarm ) ;
 yREAD_FUNCTION( FS_power ) ;
 uREAD_FUNCTION( FS_channel ) ;

/* ------- Structures ----------- */

struct aggregate A2406 = { 2, ag_letters, ag_aggregate, } ;
struct aggregate A2406p = { 4, ag_numbers, ag_separate, } ;
struct filetype DS2406[] = {
    F_STANDARD   ,
    {"memory"    ,   128,  NULL,    ft_binary  , ft_stable  , {b:FS_r_mem}    , {b:FS_w_mem} , {v:NULL}, } ,
    {"pages"     ,     0,  NULL,    ft_subdir  , ft_volatile, {v:NULL}        , {v:NULL}     , {v:NULL}, } ,
    {"pages/page",    32,  &A2406p, ft_binary  , ft_stable  , {b:FS_r_page}   , {b:FS_w_page}, {v:NULL}, } ,
    {"power"     ,     1,  NULL,    ft_yesno   , ft_volatile, {y:FS_power}    , {v:NULL}     , {v:NULL}, } ,
    {"channels"  ,     1,  NULL,    ft_unsigned, ft_stable  , {u:FS_channel}  , {v:NULL}     , {v:NULL}, } ,
    {"PIO"       ,     1,  &A2406,  ft_bitfield, ft_stable  , {u:FS_r_pio}    , {u:FS_w_pio} , {v:NULL}, } ,
    {"sensed"    ,     1,  &A2406,  ft_bitfield, ft_volatile, {u:FS_sense}    , {v:NULL}     , {v:NULL}, } ,
    {"latch"     ,     1,  &A2406,  ft_bitfield, ft_volatile, {u:FS_r_latch}  , {u:FS_w_latch},{v:NULL}, } ,
    {"set_alarm" ,     3,  NULL,    ft_unsigned, ft_stable  , {u:FS_r_s_alarm}, {u:FS_w_s_alarm},{v:NULL}, } ,
} ;
DeviceEntryExtended( 12, DS2406, DEV_alarm ) ;

/* ------- Functions ------------ */

/* DS2406 */
static int OW_r_mem( BYTE * data , const size_t size, const off_t offset, const struct parsedname * pn ) ;
static int OW_w_mem( const BYTE * data , const size_t size , const off_t offset, const struct parsedname * pn ) ;
//static int OW_r_s_alarm( BYTE * data , const struct parsedname * pn ) ;
static int OW_w_s_alarm( const BYTE data , const struct parsedname * pn ) ;
static int OW_r_control( BYTE * data , const struct parsedname * pn ) ;
static int OW_w_control( const BYTE data , const struct parsedname * pn ) ;
static int OW_w_pio( const BYTE data , const struct parsedname * pn ) ;
static int OW_access( BYTE * data , const struct parsedname * pn ) ;
static int OW_clear( const struct parsedname * pn ) ;

/* 2406 memory read */
static int FS_r_mem(BYTE *buf, const size_t size, const off_t offset , const struct parsedname * pn) {
    /* read is not a "paged" endeavor, the CRC comes after a full read */
    if ( OW_r_mem( buf, size, offset, pn ) ) return -EINVAL ;
    return size ;
}

/* 2406 memory write */
static int FS_r_page(BYTE *buf, const size_t size, const off_t offset , const struct parsedname * pn) {
//printf("2406 read size=%d, offset=%d\n",(int)size,(int)offset);
    if ( OW_r_mem( buf, size, offset+(pn->extension<<5), pn) ) return -EINVAL ;
    return size ;
}

static int FS_w_page(const BYTE *buf, const size_t size, const off_t offset , const struct parsedname * pn) {
    if ( OW_w_mem( buf, size, offset+(pn->extension<<5), pn) ) return -EINVAL ;
    return 0 ;
}

/* Note, it's EPROM -- write once */
static int FS_w_mem(const BYTE *buf, const size_t size, const off_t offset , const struct parsedname * pn) {
    /* write is "byte at a time" -- not paged */
    if ( OW_w_mem( buf, size, offset, pn ) ) return -EINVAL ;
    return 0 ;
}

/* 2406 switch */
static int FS_r_pio(UINT * u , const struct parsedname * pn) {
    BYTE data ;
    if ( OW_access(&data,pn) ) return -EINVAL ;
    u[0] = ( data^0xFF ) & 0x03 ; /* reverse bits */
    return 0 ;
}

/* 2406 switch -- is Vcc powered?*/
static int FS_power(int * y , const struct parsedname * pn) {
    BYTE data ;
    if ( OW_access(&data,pn) ) return -EINVAL ;
    *y = UT_getbit(&data,7) ;
    return 0 ;
}

/* 2406 switch -- number of channels (actually, if Vcc powered)*/
static int FS_channel(UINT * u , const struct parsedname * pn) {
    BYTE data ;
    if ( OW_access(&data,pn) ) return -EINVAL ;
    *u = (data&0x40)?2:1 ;
    return 0 ;
}

/* 2406 switch PIO sensed*/
/* bits 2 and 3 */
static int FS_sense(UINT * u , const struct parsedname * pn) {
    BYTE data ;
    if ( OW_access(&data,pn) ) return -EINVAL ;
    u[0] = ( data>>2 ) & 0x03 ;
//    y[0] = UT_getbit(&data,2) ;
//    y[1] = UT_getbit(&data,3) ;
    return 0 ;
}

/* 2406 switch activity latch*/
/* bites 4 and 5 */
static int FS_r_latch(UINT * u , const struct parsedname * pn) {
    BYTE data ;
    if ( OW_access(&data,pn) ) return -EINVAL ;
    u[0] = ( data >> 4 ) & 0x03 ;
//    y[0] = UT_getbit(&data,4) ;
//    y[1] = UT_getbit(&data,5) ;
    return 0 ;
}

/* 2406 switch activity latch*/
static int FS_w_latch(const UINT * u , const struct parsedname * pn) {
    (void) u ;
    if ( OW_clear(pn) ) return -EINVAL ;
    return 0 ;
}

/* 2406 alarm settings*/
static int FS_r_s_alarm(UINT * u , const struct parsedname * pn) {
    BYTE data ;
    if ( OW_r_control(&data,pn) ) return -EINVAL ;
    u[0] = (data & 0x01) + ((data & 0x06) >> 1) * 10 + ((data & 0x18) >> 3) * 100;
    return 0 ;
}

/* 2406 alarm settings*/
static int FS_w_s_alarm(const UINT * u , const struct parsedname * pn) {
    BYTE data;
    data = ((u[0] % 10) & 0x01) | (((u[0] / 10 % 10) & 0x03) << 1) | (((u[0] / 100 % 10) & 0x03) << 3);
    if ( OW_w_s_alarm(data,pn) ) return -EINVAL ;
    return 0 ;
}

/* write 2406 switch -- 2 values*/
static int FS_w_pio(const UINT * u, const struct parsedname * pn) {
    BYTE data = 0;
    /* reverse bits */
    data = ( u[0] ^ 0xFF ) & 0x03 ;
//    UT_setbit( &data , 0 , y[0]==0 ) ;
//    UT_setbit( &data , 1 , y[1]==0 ) ;
    if ( OW_w_pio(data,pn) ) return -EINVAL ;
    return 0 ;
}

static int OW_r_mem( BYTE * data , const size_t size , const off_t offset, const struct parsedname * pn ) {
    BYTE p[3+128+2] = { 0xF0, offset&0xFF , offset>>8, } ;
    struct transaction_log t[] = {
        TRXN_START ,
        { p, NULL, 3, trxn_match } ,
        { NULL, &p[3], 128+2-offset, trxn_read } ,
        TRXN_END,
    } ;

    if ( BUS_transaction( t, pn ) || CRC16(p,3+128+2-offset) ) return 1 ;

    memcpy( data , &p[3], size ) ;
    return 0 ;
}

static int OW_w_mem( const BYTE * data , const size_t size , const off_t offset, const struct parsedname * pn ) {
    BYTE p[6] = { 0x0F, offset&0xFF , offset>>8, data[0], } ;
    BYTE resp ;
    size_t i ;
    struct transaction_log tfirst[] = {
        TRXN_START ,
        { p, NULL, 4, trxn_match } ,
        { NULL, &p[4], 2, trxn_read } ,
        { NULL, NULL, 0, trxn_program } ,
        { NULL, &resp, 1, trxn_read } ,
        TRXN_END,
    } ;
    struct transaction_log trest[] = {
        TRXN_START ,
        { &p[1], NULL, 3, trxn_match } ,
        { NULL, &p[4], 2, trxn_read } ,
        { NULL, NULL, 0, trxn_program } ,
        { NULL, &resp, 1, trxn_read } ,
        TRXN_END,
    } ;

    if ( size==0 ) return 0 ;
    if ( BUS_transaction( tfirst, pn ) ) return 1 ;
    if ( CRC16(p,6) || (resp&~data[0]) ) return 1 ;

    for ( i=1 ; i<size ; ++i ) {
        p[3] = data[i] ;
        if ( (++p[1])==0x00 ) ++p[2] ;
        if ( BUS_transaction( trest, pn ) ) return 1 ;
        if ( CRC16(&p[1],5) || (resp&~data[i]) ) return 1 ;
    }
    return 0 ;
}

/* read status byte */
static int OW_r_control( BYTE * data , const struct parsedname * pn ) {
    BYTE p[3+1+2] = { 0xAA, 0x07 , 0x00, } ;
    struct transaction_log t[] = {
        TRXN_START ,
        { p, NULL, 3, trxn_match } ,
        { NULL, &p[3], 1+2, trxn_read } ,
        TRXN_END,
    } ;

    if ( BUS_transaction( t, pn ) ) return 1 ;
    if ( CRC16(p,3+1+2) ) return 1 ;

    *data = p[3] ;
    return 0 ;
}

/* write status byte */
static int OW_w_control( const BYTE data , const struct parsedname * pn ) {
    BYTE p[3+1+2] = { 0x55, 0x07 , 0x00, data, } ;
    struct transaction_log t[] = {
        TRXN_START ,
        { p, NULL, 4, trxn_match } ,
        { NULL, &p[4], 2, trxn_read } ,
        TRXN_END,
    } ;

    if ( BUS_transaction( t, pn ) ) return 1 ;
    if ( CRC16(p,6) ) return 1 ;

    return 0 ;
}

/* write alarm settings */
static int OW_w_s_alarm( const BYTE data , const struct parsedname * pn ) {
    BYTE b;
    if ( OW_r_control(&b,pn) ) return 1;
    b = (b & 0xE0) | (data & 0x1F);
    return OW_w_control(b,pn);
}

/* set PIO state bits: bit0=A bit1=B, value: open=1 closed=0 */
static int OW_w_pio( const BYTE data , const struct parsedname * pn ) {
    BYTE b;
    if ( OW_r_control(&b,pn) ) return 1;
    b = (b & 0x9F) | ((data << 5) & 0x60);
    return OW_w_control( b , pn ) ;
}

static int OW_access( BYTE * data , const struct parsedname * pn ) {
    BYTE p[3+2+2] = { 0xF5, 0x55 , 0xFF, } ;
    struct transaction_log t[] = {
        TRXN_START ,
        { p, NULL, 3, trxn_match } ,
        { NULL, &p[3], 2+2, trxn_read } ,
        TRXN_END,
    } ;

    if ( BUS_transaction( t, pn ) ) return 1 ;
    if ( CRC16(p,3+2+2) ) return 1 ;

    *data = p[3] ;
    return 0 ;
}

/* Clear latches */
static int OW_clear( const struct parsedname * pn ) {
    BYTE p[3+2+2] = { 0xF5, 0xD5 , 0xFF, } ;
    struct transaction_log t[] = {
        TRXN_START,
        { p, NULL, 3, trxn_match } ,
        { NULL, &p[3], 2+2, trxn_read } ,
        TRXN_END,
    } ;

    if ( BUS_transaction( t, pn ) ) return 1 ;
    if ( CRC16(p,3+2+2) ) return 1 ;

    return 0 ;
}
