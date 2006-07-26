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

#define TAI8570 /* AAG barometer */

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
    {"memory"    ,   128,  NULL,    ft_binary  , fc_stable  , {b:FS_r_mem}    , {b:FS_w_mem} , {v:NULL}, } ,
    {"pages"     ,     0,  NULL,    ft_subdir  , fc_volatile, {v:NULL}        , {v:NULL}     , {v:NULL}, } ,
    {"pages/page",    32,  &A2406p, ft_binary  , fc_stable  , {b:FS_r_page}   , {b:FS_w_page}, {v:NULL}, } ,
    {"power"     ,     1,  NULL,    ft_yesno   , fc_volatile, {y:FS_power}    , {v:NULL}     , {v:NULL}, } ,
    {"channels"  ,     1,  NULL,    ft_unsigned, fc_stable  , {u:FS_channel}  , {v:NULL}     , {v:NULL}, } ,
    {"PIO"       ,     1,  &A2406,  ft_bitfield, fc_stable  , {u:FS_r_pio}    , {u:FS_w_pio} , {v:NULL}, } ,
    {"sensed"    ,     1,  &A2406,  ft_bitfield, fc_volatile, {u:FS_sense}    , {v:NULL}     , {v:NULL}, } ,
    {"latch"     ,     1,  &A2406,  ft_bitfield, fc_volatile, {u:FS_r_latch}  , {u:FS_w_latch},{v:NULL}, } ,
    {"set_alarm" ,     3,  NULL,    ft_unsigned, fc_stable  , {u:FS_r_s_alarm}, {u:FS_w_s_alarm},{v:NULL}, } ,
#ifdef TAI8570
    {"TAI8570"   ,     0,  NULL,    ft_subdir  , fc_volatile, {v:NULL}        , {v:NULL}     , {v:NULL}, } ,
    {"TAI8570/temperature", 12,  NULL, ft_temperature, fc_volatile, {v:NULL}        , {v:NULL}     , {v:NULL}, } ,
    {"TAI8570/pressure"   , 12,  NULL,    ft_float   , fc_volatile, {v:NULL}        , {v:NULL}     , {v:NULL}, } ,
    {"TAI8570/calibration",  8,  NULL,    ft_binary  , fc_stable  , {v:NULL}        , {v:NULL}     , {v:NULL}, } ,
    {"TAI8570/mate"       , 16,  NULL,    ft_ascii   , fc_stable  , {v:NULL}        , {v:NULL}     , {v:NULL}, } ,
#endif /* TAI8570 */
} ;
DeviceEntryExtended( 12, DS2406, DEV_alarm ) ;


#ifdef TAI8570
struct s_TAI8570 {
    BYTE reader[8] ;
    BYTE writer[8] ;
    UINT C[6] ;
} ;
/* Internal files */
static struct internal_prop ip_bar = { "BAR", fc_persistent } ;

// Updated by Simon Melhuish, with ref. to AAG C++ code
static BYTE SEC_READW4[] = { 0x0E, 0x0E, 0x0E, 0x04, 0x0E, 0x0E, 0x04, 0x0E, 0x04, 0x04, 0x04, 0x04, 0x00 } ;

static BYTE SEC_READW2[] = { 0x0E, 0x0E, 0x0E, 0x04, 0x0E, 0x04, 0x0E, 0x0E, 0x04, 0x04, 0x04, 0x04, 0x00 } ;
static BYTE SEC_READW1[] = { 0x0E, 0x0E, 0x0E, 0x04, 0x0E, 0x04, 0x0E, 0x04, 0x0E, 0x04, 0x04, 0x04, 0x00 } ;
static BYTE SEC_READW3[] = { 0x0E, 0x0E, 0x0E, 0x04, 0x0E, 0x0E, 0x04, 0x04, 0x0E, 0x04, 0x04, 0x04, 0x00 } ;

static BYTE SEC_READD1[] = { 0x0E, 0x0E, 0x0E, 0x0E, 0x04, 0x0E, 0x04, 0x04, 0x04, 0x04, 0x04, 0x00 } ;
static BYTE SEC_READD2[] = { 0x0E, 0x0E, 0x0E, 0x0E, 0x04, 0x04, 0x0E, 0x04, 0x04, 0x04, 0x04, 0x00 } ;
static BYTE SEC_RESET[]  = { 0x0E, 0x04, 0x0E, 0x04, 0x0E, 0x04, 0x0E, 0x04, 0x0E, 0x04, 0x0E, 0x04, 0x0E, 0x04, 0x0E, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x00 } ;

static BYTE CFG_READ = 0xEC; // '11101100'   Configuraci� de lectura para DS2407
static BYTE CFG_WRITE = 0x8C;    // '10001100'  Configuraci� de Escritura para DS2407
static BYTE CFG_READPULSE = 0xC8;    // '11001000'  Configuraci� de lectura de Pulso de conversion para DS2407

#endif /* TAI8570 */


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
static int OW_full_access( BYTE * data , const struct parsedname * pn ) ;

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
    BYTE d[2] = { 0x55, 0xFF, } ;
    if ( OW_full_access( d, pn ) ) return 1 ;
    data[0] = d[0] ;
    return 0 ;
}

/* Clear latches */
static int OW_clear( const struct parsedname * pn ) {
    BYTE data[2] = { 0xD5, 0xFF, } ;
    return OW_full_access(data, pn ) ; ;
}

// write both control bytes, and read both back
static int OW_full_access( BYTE * data , const struct parsedname * pn ) {
    BYTE p[3+2+2] = { 0xF5, data[0] , data[1], } ;
    struct transaction_log t[] = {
        TRXN_START ,
        { p, NULL, 3, trxn_match } ,
        { NULL, &p[3], 2+2, trxn_read } ,
        TRXN_END,
    } ;

    if ( BUS_transaction( t, pn ) ) return 1 ;
    if ( CRC16(p,3+2+2) ) return 1 ;

    data[0] = p[3] ;
    data[1] = p[4] ;
    return 0 ;
}

static int ReadTmexPage( BYTE * data, size_t size, int page, const struct parsedname * pn ) ;
static int TAI8570_Calibration (UINT * cal, const struct s_TAI8570 * tai, const struct parsedname * pn) ;
static int testTAI8570( struct s_TAI8570 * tai, const struct parsedname * pn ) ;
static int TAI8570_Write( BYTE * cmd, const struct s_TAI8570 * tai, const struct parsedname * pn ) ;
static int TAI8570_Read( UINT * u, const struct s_TAI8570 * tai, const struct parsedname * pn ) ;
static int TAI8570_Reset( const struct s_TAI8570 * tai, const struct parsedname * pn ) ;
static int TAI8570_ClockPulse( const struct s_TAI8570 * tai, const struct parsedname * pn ) ;
static int TAI8570_DataPulse( const struct s_TAI8570 * tai, const struct parsedname * pn ) ;
static int TAI8570_CalValue (UINT * cal, BYTE * cmd, const struct s_TAI8570 * tai, const struct parsedname * pn) ;
static int TAI8570_A( const struct parsedname * pn ) ;
static int TAI8570_B( const struct parsedname * pn ) ;

// Read a page and confirm its a valid tmax page
static int ReadTmexPage( BYTE * data, size_t size, int page, const struct parsedname * pn ) {
    if ( OW_r_mem( data, size, size*page, pn) ) return 1 ; // read page
    if ( data[0] > size ) return 1 ; // check length
    return CRC16seeded( data, data[0]+3, page ) ;
}

static int TAI8570_config( BYTE cfg, BYTE * sn, const struct parsedname * pn ) {
    BYTE data[] = { cfg, 0xFF, } ;
    struct parsedname pn2 ;
    memcpy( &pn2, pn, sizeof(struct parsedname) ) ; // shallow copy
    memcpy( pn2.sn, sn, 8 ) ;
    return OW_full_access( data, &pn2 ) ;
}

static int TAI8570_A( const struct parsedname * pn ) {
    BYTE b = 0xFF ;
    int i ;
    for ( i=0 ; i<5 ; ++i ) {
        if ( OW_access( &b, pn ) ) return 1 ;
        if ( OW_w_control( b|0x20, pn ) ) return 1 ;
        if ( OW_access( &b, pn ) ) return 1 ;
        if ( b&0xDF ) return 0 ;
    }
    return 1 ;
}

static int TAI8570_B( const struct parsedname * pn ) {
    BYTE b = 0xFF ;
    int i ;
    for ( i=0 ; i<5 ; ++i ) {
        if ( OW_access( &b, pn ) ) return 1 ;
        if ( OW_w_control( b|0x40, pn ) ) return 1 ;
        if ( OW_access( &b, pn ) ) return 1 ;
        if ( b&0xBF ) return 0 ;
    }
    return 1 ;
}

static int TAI8570_ClockPulse( const struct s_TAI8570 * tai, const struct parsedname * pn ) {
    struct parsedname pn2 ;
    memcpy( &pn2, pn, sizeof(struct parsedname) ) ; // shallow copy
    memcpy( pn2.sn, tai->reader, 8 ) ;
    if ( TAI8570_A( &pn2 ) ) return 1 ;
    memcpy( pn2.sn, tai->writer, 8 ) ;
    return TAI8570_A( &pn2 ) ;
}

static int TAI8570_DataPulse( const struct s_TAI8570 * tai, const struct parsedname * pn ) {
    struct parsedname pn2 ;
    memcpy( &pn2, pn, sizeof(struct parsedname) ) ; // shallow copy
    memcpy( pn2.sn, tai->reader, 8 ) ;
    if ( TAI8570_B( &pn2 ) ) return 1 ;
    memcpy( pn2.sn, tai->writer, 8 ) ;
    return TAI8570_B( &pn2 ) ;
}

static int TAI8570_Reset( const struct s_TAI8570 * tai, const struct parsedname * pn ) {
    if ( TAI8570_ClockPulse( tai, pn ) ) return 1 ;
    if ( TAI8570_config( CFG_WRITE, tai->writer, pn ) ) return 1 ; // config write
    return TAI8570_Write( SEC_RESET, tai, pn ) ;
}

static int TAI8570_Write( BYTE * cmd, const struct s_TAI8570 * tai, const struct parsedname * pn ) {
    size_t len = strlen( cmd ) ;
    BYTE zero = 0x04 ;
    struct transaction_log t[] = {
        { cmd, NULL, len, trxn_match, } ,
        { &zero, NULL, 1, trxn_match, } ,
        TRXN_END ,
    } ;
    if ( TAI8570_config( CFG_READ, tai->reader, pn ) ) return 1 ; // config write
    return BUS_transaction( t, pn ) ;
}

static int TAI8570_Read( UINT * u, const struct s_TAI8570 * tai, const struct parsedname * pn ) {
    size_t i,j ;
    BYTE data[32] ;
    UINT U = 0 ;
    struct transaction_log t[] = {
        { data, data, 32, trxn_read, } ,
        TRXN_END ,
    } ;
    if ( TAI8570_config( CFG_WRITE, tai->writer, pn ) ) return 1 ; // config write
    for ( i=j=0 ; i<16; ++i ) {
        data[j++] = 0xFF ;
        data[j++] = 0xFA ;
    }
    if ( BUS_transaction( t, pn ) ) return 1 ;
    for ( j=0 ; j<32; j += 2 ) {
        U = U << 1 ;
        if ( data[j]&0x80 ) ++U ;
    }
    u[0] = U ;
    return 0 ;
}

static int TAI8570_CalValue (UINT * cal, BYTE * cmd, const struct s_TAI8570 * tai, const struct parsedname * pn) {
    if ( TAI8570_ClockPulse(tai,pn) ) return 1 ;
    if ( TAI8570_Write( cmd, tai, pn ) ) return 1 ;
    if ( TAI8570_ClockPulse(tai,pn) ) return 1 ;
    if ( TAI8570_Read( cal, tai, pn ) ) return 1 ;
    TAI8570_DataPulse(tai,pn) ;
    return 0 ;
}
/* read calibration constants and put in Cache, too
   return 0 on successful aquisition
 */
static int TAI8570_Calibration (UINT * cal, const struct s_TAI8570 * tai, const struct parsedname * pn) {
    UINT oldcal[4] = { 0, 0, 0, 0, } ;
    int rep ;
    
    for ( rep=0 ; rep<5 ; ++rep ) {
        if (TAI8570_Reset(tai,pn)) return 1 ;
        TAI8570_CalValue(&cal[0], SEC_READW1, tai, pn);
        TAI8570_CalValue(&cal[1], SEC_READW2, tai, pn);
        TAI8570_CalValue(&cal[2], SEC_READW3, tai, pn);
        TAI8570_CalValue(&cal[3], SEC_READW4, tai, pn);
        if ( memcmp(cal, oldcal, sizeof(oldcal) )==0 ) break ;
        memcpy(oldcal,cal,sizeof(oldcal) ) ;
    }
    return rep >= 5 ; // couldn't get the same answer twice in a row
}

static int testTAI8570( struct s_TAI8570 * tai, const struct parsedname * pn ) {
    size_t s = sizeof( struct s_TAI8570 ) ;
    int pow ;
    BYTE data[32] ;
    UINT cal[4] ;
    // See if already cached
    if ( Cache_Get_Internal( (void *) tai, &s, &ip_bar, pn ) == 0 ) return 0 ;
    // read page 0
    if ( ReadTmexPage(data,32,0,pn) ) return 1 ; // read page
    if ( memcmp("8570",&data[8],4) ) return 1 ; // check dir entry
    // See if page 1 is readable
    if ( ReadTmexPage(data,32,1,pn) ) return 1 ; // read page
    // see which DS2406 is powered
    if ( FS_power( &pow, pn) ) return 1 ;
    if ( pow ) {
        memcpy( tai->writer, pn->sn  , 8 ) ;
        memcpy( tai->reader, &data[1], 8 ) ;
    } else {
        memcpy( tai->reader, pn->sn  , 8 ) ;
        memcpy( tai->reader, &data[1], 8 ) ;
    }

    if ( TAI8570_Calibration(cal,tai,pn) ) return 1 ;
    tai->C[0] = (((unsigned int)cal[1])<<8)     +  (((unsigned int) cal[0])>>1) ;
    tai->C[1] = (((unsigned int)cal[5]) & 0x3F) + ((((unsigned int) cal[7]) & 0x3F)<<6) ;
    tai->C[2] = (((unsigned int)cal[7])<<8)     +  (((unsigned int) cal[6])>>6) ;
    tai->C[3] = (((unsigned int)cal[5])<<8)     +  (((unsigned int) cal[4])>>6) ;
    tai->C[4] = (((unsigned int)cal[3])<<8)     +  (((unsigned int) cal[2])>>6) + ((((unsigned int) cal[0])&0x01)<<10) ;
    tai->C[5] = (((unsigned int)cal[3]) & 0x3F) ;
    
    return Cache_Add_Internal( (const void *) tai, s, &ip_bar, pn ) ;
}
