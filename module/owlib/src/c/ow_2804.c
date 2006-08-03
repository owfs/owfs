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
#include "ow_2804.h"

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
 yREAD_FUNCTION( FS_r_por ) ;
 yREAD_FUNCTION( FS_polarity ) ;
yWRITE_FUNCTION( FS_w_por ) ;

/* ------- Structures ----------- */

struct aggregate A2804 = { 2, ag_numbers, ag_aggregate, } ;
struct aggregate A2804p = { 17, ag_numbers, ag_separate, } ;
struct filetype DS28E04[] = {
    F_STANDARD   ,
    {"memory"    ,   550,  NULL,    ft_binary  , fc_stable  , {b:FS_r_mem}    , {b:FS_w_mem} , {v:NULL}, } ,
    {"pages"     ,     0,  NULL,    ft_subdir  , fc_volatile, {v:NULL}        , {v:NULL}     , {v:NULL}, } ,
    {"pages/page",    32,  &A2804p, ft_binary  , fc_stable  , {b:FS_r_page}   , {b:FS_w_page}, {v:NULL}, } ,
    {"polarity"  ,     1,  NULL,    ft_yesno   , fc_volatile, {y:FS_polarity} , {v:NULL}     , {v:NULL}, } ,
    {"power"     ,     1,  NULL,    ft_yesno   , fc_volatile, {y:FS_power}    , {v:NULL}     , {v:NULL}, } ,
    {"por"       ,     1,  NULL,    ft_yesno   , fc_volatile, {y:FS_r_por}    , {y:FS_w_por} , {v:NULL}, } ,
    {"PIO"       ,     1,  &A2804,  ft_bitfield, fc_stable  , {u:FS_r_pio}    , {u:FS_w_pio} , {v:NULL}, } ,
    {"sensed"    ,     1,  &A2804,  ft_bitfield, fc_volatile, {u:FS_sense}    , {v:NULL}     , {v:NULL}, } ,
    {"latch"     ,     1,  &A2804,  ft_bitfield, fc_volatile, {u:FS_r_latch}  , {u:FS_w_latch},{v:NULL}, } ,
    {"set_alarm" ,     3,  NULL,    ft_unsigned, fc_stable  , {u:FS_r_s_alarm}, {u:FS_w_s_alarm},{v:NULL}, } ,
} ;
DeviceEntryExtended( 1C, DS28E04, DEV_alarm | DEV_resume | DEV_ovdr ) ;

/* ------- Functions ------------ */

/* DS2804 */
static int OW_w_mem( const BYTE * data , const size_t size , const off_t offset, const struct parsedname * pn ) ;
static int OW_w_scratch( const BYTE * data , const size_t size , const off_t offset, const struct parsedname * pn ) ;
static int OW_w_pio( const BYTE data , const struct parsedname * pn ) ;
//static int OW_access( BYTE * data , const struct parsedname * pn ) ;
static int OW_clear( const struct parsedname * pn ) ;
static int OW_w_reg( const BYTE * data, const size_t size, const off_t offset, const struct parsedname * pn ) ;

/* 2804 memory read */
static int FS_r_mem(BYTE *buf, const size_t size, const off_t offset , const struct parsedname * pn) {
    /* read is not a "paged" endeavor, the CRC comes after a full read */
    if ( OW_r_mem_simple( buf, size, offset, pn ) ) return -EINVAL ;
    return size ;
}

/* Note, it's EPROM -- write once */
static int FS_w_mem(const BYTE *buf, const size_t size, const off_t offset , const struct parsedname * pn) {
    /* write is "byte at a time" -- not paged */
    if ( OW_write_paged( buf, size, offset, pn, 32, OW_w_mem ) ) return -EINVAL ;
    return 0 ;
}

/* 2406 memory write */
static int FS_r_page(BYTE *buf, const size_t size, const off_t offset , const struct parsedname * pn) {
//printf("2406 read size=%d, offset=%d\n",(int)size,(int)offset);
    if ( OW_r_mem_simple( buf, size, offset+(pn->extension<<5), pn) ) return -EINVAL ;
    return size ;
}

static int FS_w_page(const BYTE *buf, const size_t size, const off_t offset , const struct parsedname * pn) {
    if ( OW_w_mem( buf, size, offset+(pn->extension<<5), pn) ) return -EINVAL ;
    return 0 ;
}

/* 2406 switch */
static int FS_r_pio(UINT * u , const struct parsedname * pn) {
    BYTE data ;
    if ( OW_r_mem_simple(&data,1,0x0221,pn) ) return -EINVAL ;
    u[0] = ( data^0xFF ) & 0x03 ; /* reverse bits */
    return 0 ;
}

/* write 2406 switch -- 2 values*/
static int FS_w_pio(const UINT * u, const struct parsedname * pn) {
    BYTE data = 0;
    /* reverse bits */
    data = ( u[0] ^ 0xFF ) | 0xFC ; /* Set bits 2-7 to "1" */
    if ( OW_w_pio(data,pn) ) return -EINVAL ;
    return 0 ;
}

/* 2406 switch -- is Vcc powered?*/
static int FS_power(int * y , const struct parsedname * pn) {
    BYTE data ;
    if ( OW_r_mem_simple(&data,1,0x0225,pn) ) return -EINVAL ;
    y[0] = UT_getbit(&data,7) ;
    return 0 ;
}

/* 2406 switch -- power-on status of polrity pin */
static int FS_polarity(int * y , const struct parsedname * pn) {
    BYTE data ;
    if ( OW_r_mem_simple(&data,1,0x0225,pn) ) return -EINVAL ;
    y[0] = UT_getbit(&data,6) ;
    return 0 ;
}

/* 2406 switch -- power-on status of polrity pin */
static int FS_r_por(int * y , const struct parsedname * pn) {
    BYTE data ;
    if ( OW_r_mem_simple(&data,1,0x0225,pn) ) return -EINVAL ;
    y[0] = UT_getbit(&data,3) ;
    return 0 ;
}

/* 2406 switch -- power-on status of polrity pin */
static int FS_w_por(const int * y , const struct parsedname * pn) {
    BYTE data ;
    (void) y ;
    if ( OW_r_mem_simple(&data,1,0x0225,pn) ) return -EINVAL ; /* get current register */
    if ( UT_getbit(&data,3) ) { /* needs resetting? bit3==1 */
        int yy ;
        data ^= 0x08 ; /* flip bit 3 */
        if ( OW_w_reg(&data,1,0x0225,pn) ) return -EINVAL ; /* reset */
        if ( FS_r_por(&yy,pn) ) return -EINVAL ; /* reread */
        if ( yy ) return -EINVAL ; /* not reset despite our try */
    }
    return 0 ; /* good */
}

/* 2406 switch PIO sensed*/
static int FS_sense(UINT * u , const struct parsedname * pn) {
    BYTE data ;
    if ( OW_r_mem_simple(&data,1,0x0220,pn) ) return -EINVAL ;
    u[0] = ( data ) & 0x03 ;
    return 0 ;
}

/* 2406 switch activity latch*/
static int FS_r_latch(UINT * u , const struct parsedname * pn) {
    BYTE data ;
    if ( OW_r_mem_simple(&data,1,0x0222,pn) ) return -EINVAL ;
    u[0] = data & 0x03 ;
    return 0 ;
}

/* 2406 switch activity latch*/
static int FS_w_latch(const UINT * u , const struct parsedname * pn) {
    (void) u ;
    if ( OW_clear(pn) ) return -EINVAL ;
    return 0 ;
}

/* 2804 alarm settings*/
static int FS_r_s_alarm(UINT * u , const struct parsedname * pn) {
    BYTE data[3] ;
    if ( OW_r_mem_simple(data,3,0x0223,pn) ) return -EINVAL ;
    u[0] = ( data[2] & 0x03 ) * 100 ;
    u[0] += UT_getbit(&data[1],0) | ( UT_getbit(&data[0],0) << 1 ) ;
    u[0] += UT_getbit(&data[1],1) | ( UT_getbit(&data[0],1) << 1 ) * 10 ;
    return 0 ;
}

/* 2804 alarm settings*/
static int FS_w_s_alarm(const UINT * u , const struct parsedname * pn) {
    BYTE data[3] = { 0, 0, 0, } ;
    if ( OW_r_mem_simple(&data[2],1,0x0225,pn) ) return -EINVAL ;
    data[2] |= (u[0] / 100 % 10) & 0x03 ;
    UT_setbit(&data[1],0,(int)(u[0] % 10) & 0x01) ;
    UT_setbit(&data[1],1,(int)(u[0] / 10 % 10) & 0x01) ;
    UT_setbit(&data[0],0,((int)(u[0] % 10) & 0x02) >> 1) ;
    UT_setbit(&data[0],1,((int)(u[0] / 10 % 10) & 0x02) >> 1) ;
    if ( OW_w_reg(data,3,0x0223,pn) ) return -EINVAL ;
    return 0 ;
}

static int OW_w_scratch( const BYTE * data , const size_t size , const off_t offset, const struct parsedname * pn ) {
    BYTE p[3+32+2] = { 0x0F, offset&0xFF , (offset>>8)&0xFF, } ;
    struct transaction_log t[] = {
        TRXN_START ,
        { p, NULL, 3+size, trxn_match, } ,
        TRXN_END ,
    } ;
    struct transaction_log tcrc[] = {
        TRXN_START ,
        { p, NULL, 3+size, trxn_match, } ,
        { &p[3+size], NULL, 2, trxn_read, } ,
        TRXN_END ,
    } ;

    memcpy( &p[3] , data, size ) ;
    if ( ((size+offset)&0x1F)==0 ) { /* Check CRC if write is to end of page */
        return BUS_transaction(tcrc,pn) || CRC16(p,3+size+2) ;
    } else {
        return BUS_transaction(t,pn) ;
    }
}

/* pre-paged */
static int OW_w_mem( const BYTE * data , const size_t size , const off_t offset, const struct parsedname * pn ) {
    BYTE p[4+32+2] = { 0xAA, offset&0xFF , (offset>>8)&0xFF, } ;
    struct transaction_log tread[] = {
        TRXN_START ,
        { p, NULL, 1, trxn_match, } ,
        { &p[1], NULL, 1+size+2, trxn_read, } ,
        TRXN_END ,
    } ;
    struct transaction_log tcopy[] = {
        TRXN_START ,
        { p, NULL, 3, trxn_match, } ,
        { &p[3], &p[3], 10, trxn_power, } ,
        TRXN_END ,
    } ;

    if (  OW_w_scratch(data,size,offset,pn)
            || BUS_transaction(tread,pn)
            || CRC16(p,4+size+2)
            || memcmp(data,&p[4],size)
        ) return 1 ;
    p[0] = 0x55 ;
    return BUS_transaction(tcopy,pn) ;
}

//* write status byte */
static int OW_w_reg( const BYTE * data, const size_t size, const off_t offset, const struct parsedname * pn ) {
    BYTE p[3] = { 0xCC, offset&0xFF, (offset>>8)&0xFF, } ;
    struct transaction_log t[] = {
        TRXN_START ,
        { p, NULL, 3, trxn_match, } ,
        { data, NULL, size, trxn_match, } ,
        TRXN_END ,
    } ;
    return BUS_transaction(t,pn) ;
}

/* set PIO state bits: bit0=A bit1=B, value: open=1 closed=0 */
static int OW_w_pio( const BYTE data , const struct parsedname * pn ) {
    BYTE p[3] = { 0x5A, data&0xFF, (data&0xFF)^0xFF, } ;
    BYTE resp[1] ;
    struct transaction_log t[] = {
        TRXN_START ,
        { p, NULL, 3, trxn_match, } ,
        { NULL, resp, 1, trxn_read, } ,
        TRXN_END ,
    } ;

    return BUS_transaction(t,pn) || resp[0]!=0xAA ;
}

/* Clear latches */
static int OW_clear( const struct parsedname * pn ) {
    BYTE p[2] = { 0xC3, 0xFF } ;
    struct transaction_log t[] = {
        TRXN_START ,
        { p, p, 2, trxn_read, } ,
        TRXN_END ,
    } ;

    return BUS_transaction(t,pn) || p[0]!=0xC3 || p[1]!=0xAA ;
}
