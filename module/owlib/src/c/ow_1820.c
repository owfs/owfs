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
#include "ow_1820.h"

/* ------- Prototypes ----------- */
/* DS18S20&2 Temperature */
// READ_FUNCTION( FS_tempdata ) ;
 fREAD_FUNCTION( FS_10temp ) ;
 fREAD_FUNCTION( FS_22temp ) ;
 yREAD_FUNCTION( FS_22power ) ;
 fREAD_FUNCTION( FS_r_templimit ) ;
fWRITE_FUNCTION( FS_w_templimit ) ;

/* -------- Structures ---------- */
struct filetype DS18S20[] = {
    F_STANDARD   ,
    {"temperature",   12,  NULL, ft_float , ft_volatile, {f:FS_10temp}     , {v:NULL}, NULL, } ,
    {"templow",       12,  NULL, ft_float , ft_stable  , {f:FS_r_templimit},{f:FS_w_templimit}, (void *)1, } ,
    {"temphigh",      12,  NULL, ft_float , ft_stable  , {f:FS_r_templimit},{f:FS_w_templimit}, (void *)0, } ,
    {"power"     ,     1,  NULL, ft_yesno , ft_volatile, {y:FS_22power}    , {v:NULL}, NULL, } ,
}
 ;
DeviceEntry( 10, DS18S20 )

struct filetype DS18B20[] = {
    F_STANDARD   ,
//    {"scratchpad",     8,  NULL, ft_binary, ft_volatile, FS_tempdata   , NULL, NULL, } ,
    {"temperature",   12,  NULL, ft_float , ft_volatile, {f:FS_22temp}     , {v:NULL}, (void *)12, } ,
    {"fasttemp"  ,    12,  NULL, ft_float , ft_volatile, {f:FS_22temp}     , {v:NULL}, (void *) 9, } ,
    {"templow",       12,  NULL, ft_float , ft_stable  , {f:FS_r_templimit},{f:FS_w_templimit}, (void *)1, } ,
    {"temphigh",      12,  NULL, ft_float , ft_stable  , {f:FS_r_templimit},{f:FS_w_templimit}, (void *)0, } ,
    {"power"     ,     1,  NULL, ft_yesno , ft_volatile, {y:FS_22power}    , {v:NULL}, NULL, } ,
} ;
DeviceEntry( 28, DS18B20 )

struct filetype DS1822[] = {
    F_STANDARD   ,
//    {"scratchpad",     8,  NULL, ft_binary, ft_volatile, FS_tempdata   , NULL, NULL, } ,
    {"temperature",   12,  NULL, ft_float , ft_volatile, {f:FS_22temp}     , {v:NULL}, (void *)12, } ,
    {"fasttemp"  ,    12,  NULL, ft_float , ft_volatile, {f:FS_22temp}     , {v:NULL}, (void *) 9, } ,
    {"templow",       12,  NULL, ft_float , ft_stable  , {f:FS_r_templimit},{f:FS_w_templimit}, (void *)1, } ,
    {"temphigh",      12,  NULL, ft_float , ft_stable  , {f:FS_r_templimit},{f:FS_w_templimit}, (void *)0, } ,
    {"power"     ,     1,  NULL, ft_yesno , ft_volatile, {y:FS_22power}    , {v:NULL}, NULL, } ,
} ;
DeviceEntry( 22, DS1822 )

struct tempresolution {
    unsigned char config ;
	unsigned int delay ;
} ;
struct tempresolution Resolutions[] = {
    { 0x1F,  94, } , /*  9 bit */
    { 0x3F, 188, } , /* 10 bit */
    { 0x5F, 375, } , /* 11 bit */
    { 0x7F, 750, } , /* 12 bit */
} ;

/* ------- Functions ------------ */

/* DS1820&2*/
static int OW_10temp(float * temp , const struct parsedname * pn) ;
static int OW_22temp(float * temp , int resolution, const struct parsedname * pn) ;
static int OW_22power(unsigned char * data, const struct parsedname * pn) ;
static int OW_r_templimit( float * T, const int Tindex, const struct parsedname * pn) ;
static int OW_w_templimit( const float T, const int Tindex, const struct parsedname * pn) ;
static int OW_r_scratchpad(unsigned char * data, const struct parsedname * pn) ;
static int OW_w_scratchpad(const unsigned char * data, const struct parsedname * pn) ;

int FS_10temp(float *T , const struct parsedname * pn) {
    if ( OW_10temp( T , pn ) ) return -EINVAL ;
	*T = Temperature(*T) ;
	return 0 ;
}

/* For DS1822 and DS18B20 -- resolution stuffed in ft->data */
int FS_22temp(float *T , const struct parsedname * pn) {
	switch( (int) pn->ft->data ) {
	case 9:
	case 10:
	case 11:
	case 12:
		if ( OW_22temp( T, (int) pn->ft->data , pn ) ) return -EINVAL ;
    	*T = Temperature(*T) ;
		return 0 ;
	}
    return -ENODEV ;
}

int FS_22power(int * y , const struct parsedname * pn) {
    unsigned char data ;
    if ( OW_22power( &data , pn ) ) return -EINVAL ;
    *y = data ;
	return 0 ;
}

int FS_r_templimit(float * T , const struct parsedname * pn) {
    if ( OW_r_templimit( T , (int) pn->ft->data, pn ) ) return -EINVAL ;
	*T = Temperature(*T) ;
	return 0 ;
}

int FS_w_templimit(const float * T, const struct parsedname * pn) {
    if ( pn->ft->data==NULL ) return -ENODEV ;
    if ( OW_w_templimit( *T , (int) pn->ft->data, pn ) ) return -EINVAL ;
    return 0 ;
}

/*
int FS_tempdata(char *buf, const size_t size, const off_t offset , const struct parsedname * pn) {
    unsigned char data[8] ;
    if ( OW_r_scratchpad( data, pn ) ) return -EINVAL ;
    return FS_read_return( buf,size,offset,data,8 ) ;
}
*/
/* get the temp from the scratchpad buffer after starting a conversion and waiting */
static int OW_10temp(float * temp , const struct parsedname * pn) {
    unsigned char data[8] ;
    int ret ;

    /* Select particular device and start conversion */
    BUS_lock() ;
        ret = BUS_select(pn) || BUS_PowerByte( 0x44, 750 ) ;
    BUS_unlock() ;
    if ( ret ) return 1 ;

    if ( OW_r_scratchpad( data , pn ) ) return 1 ;

    /* fancy math */
    *temp = (float)((((char)data[1])<<8|data[0])>>1) + .75 - .0625*data[6] ;
    return 0 ;
}

static int OW_22power( unsigned char * data, const struct parsedname * pn) {
    unsigned char b4 = 0xB4;
    int ret ;

	BUS_lock() ;
        ret = BUS_select(pn) || BUS_send_data( &b4 , 1 ) || BUS_readin_data( data , 1 ) ;
    BUS_unlock() ;
    if ( ret ) return 1 ;
    return 0 ;
}

static int OW_22temp(float * temp , int resolution, const struct parsedname * pn) {
    unsigned char data[8] ;
	int res = Resolutions[resolution-9].config ;
    int ret ;

	/* Get existing settings */
    if ( OW_r_scratchpad(data , pn ) ) return 1 ;
	/* Put in new settings */
	if ( data[4] != res ) {
    	data[4] = res ;
        if ( OW_w_scratchpad(&data[2] , pn ) ) return 1 ;
	}

	/* Conversion */
    BUS_lock() ;
        ret =BUS_select(pn) || BUS_PowerByte( 0x44, Resolutions[resolution-9].delay ) ;
    BUS_unlock() ;
    if ( ret ) return 1 ;

    if ( OW_r_scratchpad( data, pn ) ) return 1 ;

    *temp = .0625*(((char)data[1])<<8|data[0]) ;
    return 0 ;
}

/* Limits Tindex=0 high 1=low */
static int OW_r_templimit( float * T, const int Tindex, const struct parsedname * pn) {
    unsigned char data[8] ;
	unsigned char recall = 0xB4 ;
	int ret ;

    BUS_lock() ;
        ret = BUS_select(pn) || BUS_send_data( &recall , 1 ) ;
    BUS_unlock() ;
	if ( ret ) return 1 ;

    UT_delay(10) ;

    if ( OW_r_scratchpad( data, pn ) ) return 1 ;
    *T = (char) data[2+Tindex] ;
	return 0 ;
}

/* Limits Tindex=0 high 1=low */
static int OW_w_templimit( const float T, const int Tindex, const struct parsedname * pn) {
    unsigned char data[8] ;

    if ( OW_r_scratchpad( data, pn ) ) return 1 ;
    (char) data[2+Tindex] = T ;
	return OW_w_scratchpad( &data[2], pn ) ;
}

/* read 8 bytes, includes CRC8 which is checked */
static int OW_r_scratchpad(unsigned char * data, const struct parsedname * pn) {
    /* data is 8 bytes long */
    unsigned char be = 0xBE ;
    unsigned char td[9] ;
    int ret ;

    BUS_lock() ;
        ret = BUS_select(pn) || BUS_send_data( &be , 1 ) || BUS_readin_data( td , 9 ) || CRC8(td,9) ;
    BUS_unlock() ;
    if ( ret ) return 1 ;

    memcpy( data , td , 8 ) ;
    return 0 ;
}

/* write 3 bytes (byte2,3,4 of register) */
static int OW_w_scratchpad(const unsigned char * data, const struct parsedname * pn) {
    /* data is 3 bytes ng */
    unsigned char d[] = { 0x4E, data[0], data[1], data[2], } ;
    int ret ;

    BUS_lock() ;
        ret = BUS_select(pn) || BUS_send_data( d , 4 ) || BUS_select(pn) || BUS_PowerByte( 0x48, 10 ) ;
    BUS_unlock() ;
    return ret ;
}
