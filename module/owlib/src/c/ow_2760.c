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

#include "owfs_config.h"
#include "ow_2760.h"

/* ------- Prototypes ----------- */

/* DS2406 switch */
 bREAD_FUNCTION( FS_r_mem ) ;
bWRITE_FUNCTION( FS_w_mem ) ;
 bREAD_FUNCTION( FS_r_page ) ;
bWRITE_FUNCTION( FS_w_page ) ;
 yREAD_FUNCTION( FS_r_pio ) ;
yWRITE_FUNCTION( FS_w_pio ) ;
 yREAD_FUNCTION( FS_r_latch ) ;
yWRITE_FUNCTION( FS_w_latch ) ;
 uREAD_FUNCTION( FS_r_s_alarm ) ;
uWRITE_FUNCTION( FS_w_s_alarm ) ;
 yREAD_FUNCTION( FS_power ) ;
 uREAD_FUNCTION( FS_channel ) ;
 yREAD_FUNCTION( FS_sense ) ;

/* ------- Structures ----------- */

struct aggregate A2760p = { 16, ag_numbers, ag_separate, } ;
struct filetype DS2760[] = {
    F_STANDARD   ,
    {"memory"    ,   256,  NULL,    ft_binary  , ft_stable  , {b:FS_r_mem}    , {b:FS_w_mem} , NULL, } ,
    {"pages"     ,     0,  NULL,    ft_subdir  , ft_volatile, {v:NULL}        , {v:NULL}     , NULL, } ,
    {"pages/page",    16,  NULL,    ft_binary  , ft_stable  , {b:FS_r_page}   , {b:FS_w_page}, NULL, } ,
    {"power"     ,     1,  NULL,    ft_yesno   , ft_volatile, {y:FS_power}    , {v:NULL}     , NULL, } ,
    {"channels"  ,     1,  NULL,    ft_unsigned, ft_stable  , {u:FS_channel}  , {v:NULL}     , NULL, } ,
    {"PIO"       ,     1,  NULL,    ft_yesno   , ft_stable  , {y:FS_r_pio}    , {y:FS_w_pio} , NULL, } ,
    {"sensed"    ,     1,  NULL,    ft_yesno   , ft_volatile, {y:FS_sense}    , {v:NULL}     , NULL, } ,
    {"latch"     ,     1,  NULL,    ft_yesno   , ft_volatile, {y:FS_r_latch}  , {y:FS_w_latch},NULL, } ,
    {"set_alarm" ,     3,  NULL,    ft_unsigned, ft_stable  , {u:FS_r_s_alarm}, {u:FS_w_s_alarm},NULL, } ,
} ;
DeviceEntry( 30, DS2760 )

/* ------- Functions ------------ */

/* DS2406 */
static int OW_r_sram( unsigned char * data , const size_t size, const size_t offset, const struct parsedname * pn ) ;
static int OW_r_mem( unsigned char * data , const size_t size, const size_t offset, const struct parsedname * pn ) ;
static int OW_w_mem( const unsigned char * data , const size_t size , const size_t offset, const struct parsedname * pn ) ;
static int OW_r_s_alarm( unsigned char * data , const struct parsedname * pn ) ;
static int OW_w_s_alarm( const unsigned char data , const struct parsedname * pn ) ;
static int OW_r_control( unsigned char * data , const struct parsedname * pn ) ;
static int OW_w_control( const unsigned char data , const struct parsedname * pn ) ;
static int OW_w_pio( const unsigned char data , const struct parsedname * pn ) ;
static int OW_access( unsigned char * data , const struct parsedname * pn ) ;
static int OW_clear( const struct parsedname * pn ) ;
static int OW_r_eeprom( const size_t page, const size_t size , const size_t offset, const struct parsedname * pn ) ;
static int OW_w_eeprom( const size_t page, const unsigned char * data , const size_t size , const size_t offset, const struct parsedname * pn ) ;

/* 2406 memory read */
static int FS_r_mem(unsigned char *buf, const size_t size, const off_t offset , const struct parsedname * pn) {
    /* read is not a "paged" endeavor, the CRC comes after a full read */
    if ( OW_r_mem( buf, size, offset, pn ) ) return -EINVAL ;
    return size ;
}

/* 2406 memory write */
static int FS_r_page(unsigned char *buf, const size_t size, const off_t offset , const struct parsedname * pn) {
//printf("2406 read size=%d, offset=%d\n",(int)size,(int)offset);
    if ( OW_r_mem( buf, size, offset+(pn->extension<<5), pn) ) return -EINVAL ;
    return size ;
}

static int FS_w_page(const unsigned char *buf, const size_t size, const off_t offset , const struct parsedname * pn) {
    if ( OW_w_mem( buf, size, offset+(pn->extension<<5), pn) ) return -EINVAL ;
    return 0 ;
}

/* Note, it's EPROM -- write once */
static int FS_w_mem(const unsigned char *buf, const size_t size, const off_t offset , const struct parsedname * pn) {
    /* write is "byte at a time" -- not paged */
    if ( OW_w_mem( buf, size, offset, pn ) ) return -EINVAL ;
    return 0 ;
}

/* 2406 switch */
static int FS_r_pio(int * y , const struct parsedname * pn) {
    unsigned char data ;
    if ( OW_access(&data,pn) ) return -EINVAL ;
//printf("RPIO data=%2X\n",data) ;
    data ^= 0xFF ; /* reverse bits */
//printf("RPIO data=%2X\n",data) ;
    y[0] = UT_getbit(&data,0) ;
    y[1] = UT_getbit(&data,1) ;
//printf("RPIO pio=%d,%d\n",y[0],y[1]) ;
    return 0 ;
}

/* 2406 switch -- is Vcc powered?*/
static int FS_power(int * y , const struct parsedname * pn) {
    unsigned char data ;
    if ( OW_access(&data,pn) ) return -EINVAL ;
    *y = UT_getbit(&data,7) ;
    return 0 ;
}

/* 2406 switch -- number of channels (actually, if Vcc powered)*/
static int FS_channel(unsigned int * u , const struct parsedname * pn) {
    unsigned char data ;
    if ( OW_access(&data,pn) ) return -EINVAL ;
    *u = (data&0x40)?2:1 ;
    return 0 ;
}

/* 2406 switch PIO sensed*/
static int FS_sense(int * y , const struct parsedname * pn) {
    unsigned char data ;
    if ( OW_access(&data,pn) ) return -EINVAL ;
    y[0] = UT_getbit(&data,2) ;
    y[1] = UT_getbit(&data,3) ;
    return 0 ;
}

/* 2406 switch activity latch*/
static int FS_r_latch(int * y , const struct parsedname * pn) {
    unsigned char data ;
    if ( OW_access(&data,pn) ) return -EINVAL ;
    y[0] = UT_getbit(&data,4) ;
    y[1] = UT_getbit(&data,5) ;
    return 0 ;
}

/* 2406 switch activity latch*/
static int FS_w_latch(const int * y , const struct parsedname * pn) {
    (void) y ;
    if ( OW_clear(pn) ) return -EINVAL ;
    return 0 ;
}

/* 2406 alarm settings*/
static int FS_r_s_alarm(unsigned int * u , const struct parsedname * pn) {
    unsigned char data ;
    if ( OW_r_control(&data,pn) ) return -EINVAL ;
    u[0] = (data & 0x01) + ((data & 0x06) >> 1) * 10 + ((data & 0x18) >> 3) * 100;
    return 0 ;
}

/* 2406 alarm settings*/
static int FS_w_s_alarm(const unsigned int * u , const struct parsedname * pn) {
    unsigned char data;
    data = ((u[0] % 10) & 0x01) | (((u[0] / 10 % 10) & 0x03) << 1) | (((u[0] / 100 % 10) & 0x03) << 3);
    if ( OW_w_control(data,pn) ) return -EINVAL ;
    return 0 ;
}

/* write 2406 switch -- 2 values*/
static int FS_w_pio(const int * y, const struct parsedname * pn) {
    unsigned char data = 0;
    UT_setbit( &data , 0 , y[0]==0 ) ;
    UT_setbit( &data , 1 , y[1]==0 ) ;
    if ( OW_w_pio(data,pn) ) return -EINVAL ;
    return 0 ;
}

static int OW_r_eeprom( const size_t page, const size_t size , const size_t offset, const struct parsedname * pn ) {
    unsigned char p[] = { 0xB8, page&0xFF, } ;
    int ret ;
    if ( offset > page+16 ) return 0 ;
    if ( offset + size <= page ) return 0 ;
    BUSLOCK
        ret = BUS_select(pn) || BUS_send_data( p , 2 ) ;
    BUSUNLOCK
    return ret ;
}

static int OW_r_mem( unsigned char * data , const size_t size , const size_t offset, const struct parsedname * pn ) {
    return OW_r_eeprom(0x20,size,offset,pn)
        || OW_r_eeprom(0x30,size,offset,pn)
        || OW_r_sram(data,size,offset,pn) ;
}

/* just read the sram -- eeprom may need to be recalled if you want it */
static int OW_r_sram( unsigned char * data , const size_t size , const size_t offset, const struct parsedname * pn ) {
    unsigned char p[] = { 0x69, offset&0xFF , } ;
    int ret ;

    BUSLOCK
        ret = BUS_select(pn) || BUS_send_data( p , 2 ) || BUS_readin_data( data, size ) ;
    BUSUNLOCK
    if ( ret ) return 1 ;

    return 0 ;
}

/* Special processing for eeprom -- page is the address of a 16 byte page (e.g. 0x20) */
static int OW_w_eeprom( const size_t page, const unsigned char * data , const size_t size , const size_t offset, const struct parsedname * pn ) {
    unsigned char p[] = { 0x48, page, } ;
    unsigned char cmp[16] ;
    size_t pagenext = page + 16 ;
    size_t eoffset = (offset<page) ? page : offset ;
    size_t enext = size+offset ;
    size_t esize ;
    int ret ;

    if ( offset >= pagenext ) return 0 ;
    if ( enext <= page ) return 0 ;
    if ( enext > pagenext ) enext = pagenext ;
    esize = enext - eoffset ;

    // reread and compare memory before writing
    ret = OW_r_sram(cmp,esize,eoffset,pn) || memcmp(cmp,&data[offset-eoffset],esize) ;
    if (ret) return ret ;
    BUSLOCK
        ret = BUS_select(pn) || BUS_send_data(p,2) ;
    BUSUNLOCK
    if (ret) return ret ;
    // delay to write eeprom -- with bus unlocked
    UT_delay(10) ;
    return 0 ;
}

static int OW_w_mem( const unsigned char * data , const size_t size , const size_t offset, const struct parsedname * pn ) {
    unsigned char p[] = { 0x6C, offset&0xFF , } ;
    int ret ;

    BUSLOCK
        ret = BUS_select(pn) || BUS_send_data(p,2) || BUS_send_data(data,size) ;
    BUSUNLOCK
    if ( ret ) return 1 ;

    /* special processing for region 0x20--0x3F which is eeprom */
    return OW_w_eeprom(0x20,data,size,offset,pn) || OW_w_eeprom(0x30,data,size,offset,pn) ;
}

/* read status byte */
static int OW_r_control( unsigned char * data , const struct parsedname * pn ) {
    unsigned char p[3+1+2] = { 0xAA, 0x07 , 0x00, } ;
    int ret ;

    BUSLOCK
        ret = BUS_select(pn) || BUS_send_data( p , 3 ) || BUS_readin_data( &p[3], 1+2 ) || CRC16(p,3+1+2) ;
    BUSUNLOCK
    if ( ret ) return 1 ;

    *data = p[3] ;
    return 0 ;
}

/* write status byte */
static int OW_w_control( const unsigned char data , const struct parsedname * pn ) {
    unsigned char p[3+1+2] = { 0x55, 0x07 , 0x00, data, } ;
    int ret ;

    BUSLOCK
        ret = BUS_select(pn) || BUS_send_data( p , 4 ) || BUS_readin_data( &p[4], 2 ) || CRC16(p,6) ;
    BUSUNLOCK
    return ret ;
}

/* read alarm settings */
static int OW_r_s_alarm( unsigned char * data , const struct parsedname * pn ) {
    if ( OW_r_control(data,pn) ) return 1;
    *data &= 0x1F;
    return 0;
}

/* write alarm settings */
static int OW_w_s_alarm( const unsigned char data , const struct parsedname * pn ) {
    unsigned char b;
    if ( OW_r_control(&b,pn) ) return 1;
    b = (b & 0x60) | (data & 0x1F);
    return OW_w_control(b,pn);
}

/* set PIO state bits: bit0=A bit1=B, value: open=1 closed=0 */
static int OW_w_pio( const unsigned char data , const struct parsedname * pn ) {
    unsigned char b;
    if ( OW_r_control(&b,pn) ) return 1;
    b = (b & 0x9F) | ((data << 5) & 0x60);
    return OW_w_control( b , pn ) ;
}

static int OW_access( unsigned char * data , const struct parsedname * pn ) {
    unsigned char p[3+2+2] = { 0xF5, 0x55 , 0xFF, } ;
    int ret ;

    BUSLOCK
         ret =BUS_select(pn) || BUS_send_data( p , 3 ) || BUS_readin_data( &p[3], 2+2 ) || CRC16(p,3+2+2) ;
    BUSUNLOCK
    if ( ret ) return 1 ;

    *data = p[3] ;
//printf("ACCESS %.2X %.2X\n",p[3],p[4]);
    return 0 ;
}

/* Clear latches */
static int OW_clear( const struct parsedname * pn ) {
    unsigned char p[3+2+2] = { 0xF5, 0xD5 , 0xFF, } ;
    int ret ;

    BUSLOCK
         ret =BUS_select(pn) || BUS_send_data( p , 3 ) || BUS_readin_data( &p[3], 2+2 ) || CRC16(p,3+2+2) ;
    BUSUNLOCK
    if ( ret ) return 1 ;

    return 0 ;
}
