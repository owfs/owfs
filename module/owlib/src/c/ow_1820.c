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
 yREAD_FUNCTION( FS_power ) ;
 fREAD_FUNCTION( FS_r_templimit ) ;
fWRITE_FUNCTION( FS_w_templimit ) ;
 aREAD_FUNCTION( FS_r_die ) ;
 bREAD_FUNCTION( FS_r_trim ) ;
bWRITE_FUNCTION( FS_w_trim ) ;
 yREAD_FUNCTION( FS_r_trimvalid ) ;
 yREAD_FUNCTION( FS_r_blanket ) ;
yWRITE_FUNCTION( FS_w_blanket ) ;

/* -------- Structures ---------- */
struct filetype DS18S20[] = {
    F_STANDARD   ,
    {"temperature",   12,  NULL, ft_float , ft_volatile, {f:FS_10temp}     , {v:NULL}          , (void *) 1000, } ,
    {"templow",       12,  NULL, ft_float , ft_stable  , {f:FS_r_templimit}, {f:FS_w_templimit}, (void *) 1,   } ,
    {"temphigh",      12,  NULL, ft_float , ft_stable  , {f:FS_r_templimit}, {f:FS_w_templimit}, (void *) 0,   } ,
    {"trim",           2,  NULL, ft_binary, ft_volatile, {b:FS_r_trim}     , {b:FS_w_trim}     , NULL,         } ,
    {"die",            2,  NULL, ft_ascii , ft_static  , {a:FS_r_die}      , {v:NULL}          , (void *) 1,   } ,
    {"trimvalid",      1,  NULL, ft_yesno , ft_volatile, {y:FS_r_trimvalid}, {v:NULL}          , NULL,         } ,
    {"trimblanket",    1,  NULL, ft_yesno , ft_volatile, {y:FS_r_blanket}  , {y:FS_w_blanket}  , NULL,         } ,
    {"power"     ,     1,  NULL, ft_yesno , ft_volatile, {y:FS_power}      , {v:NULL}          , NULL,         } ,
}
 ;
DeviceEntry( 10, DS18S20 )

struct filetype DS18B20[] = {
    F_STANDARD   ,
//    {"scratchpad",     8,  NULL, ft_binary, ft_volatile, FS_tempdata   , NULL, NULL, NULL,} ,
    {"temperature",   12,  NULL, ft_float , ft_volatile, {f:FS_22temp}     , {v:NULL}          , (void *)12,  } ,
    {"fasttemp"  ,    12,  NULL, ft_float , ft_volatile, {f:FS_22temp}     , {v:NULL}          , (void *) 9,  } ,
    {"templow",       12,  NULL, ft_float , ft_stable  , {f:FS_r_templimit}, {f:FS_w_templimit}, (void *) 1,  } ,
    {"temphigh",      12,  NULL, ft_float , ft_stable  , {f:FS_r_templimit}, {f:FS_w_templimit}, (void *) 0,  } ,
    {"trim",           2,  NULL, ft_binary, ft_volatile, {b:FS_r_trim}     , {b:FS_w_trim}     , NULL,        } ,
    {"die",            2,  NULL, ft_ascii , ft_static  , {a:FS_r_die}      , {v:NULL}          , (void *) 2,  } ,
    {"trimvalid",      1,  NULL, ft_yesno , ft_volatile, {y:FS_r_trimvalid}, {v:NULL}          , NULL,        } ,
    {"trimblanket",    1,  NULL, ft_yesno , ft_volatile, {y:FS_r_blanket}  , {y:FS_w_blanket}  , NULL,        } ,
    {"power"     ,     1,  NULL, ft_yesno , ft_volatile, {y:FS_power}      , {v:NULL}          , NULL,        } ,
} ;
DeviceEntry( 28, DS18B20 )

struct filetype DS1822[] = {
    F_STANDARD   ,
//    {"scratchpad",     8,  NULL, ft_binary, ft_volatile, FS_tempdata   , NULL, NULL, } ,
    {"temperature",   12,  NULL, ft_float , ft_volatile, {f:FS_22temp}     , {v:NULL}          , (void *)12,  } ,
    {"fasttemp"  ,    12,  NULL, ft_float , ft_volatile, {f:FS_22temp}     , {v:NULL}          , (void *) 9,  } ,
    {"templow",       12,  NULL, ft_float , ft_stable  , {f:FS_r_templimit}, {f:FS_w_templimit}, (void *) 1,  } ,
    {"temphigh",      12,  NULL, ft_float , ft_stable  , {f:FS_r_templimit}, {f:FS_w_templimit}, (void *) 0,  } ,
    {"trim",           2,  NULL, ft_binary, ft_volatile, {b:FS_r_trim}     , {b:FS_w_trim}     , NULL,        } ,
    {"die",            2,  NULL, ft_ascii , ft_static  , {a:FS_r_die}      , {v:NULL}          , (void *) 0,  } ,
    {"trimvalid",      1,  NULL, ft_yesno , ft_volatile, {y:FS_r_trimvalid}, {v:NULL}          , NULL,        } ,
    {"trimblanket",    1,  NULL, ft_yesno , ft_volatile, {y:FS_r_blanket}  , {y:FS_w_blanket}  , NULL,        } ,
    {"power"     ,     1,  NULL, ft_yesno , ft_volatile, {y:FS_power}      , {v:NULL}          , NULL,        } ,
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

struct die_limits {
    unsigned char B7[6] ;
    unsigned char C2[6] ;
} ;

enum eDie { eB6, eB7, eC2 } ;

// ID ranges for the different chip dies
struct die_limits DIE[] = {
    {   // DS1822 Family code 22
        { 0x00, 0x00, 0x00, 0x08, 0x97, 0x8A } ,
        { 0x00, 0x00, 0x00, 0x0C, 0xB8, 0x1A } ,
    } ,
    {   // DS18S20 Family code 10
        { 0x00, 0x08, 0x00, 0x59, 0x1D, 0x20 } ,
        { 0x00, 0x08, 0x00, 0x80, 0x88, 0x60 } ,
    } ,
    {   // DS18B20 Family code 28
        { 0x00, 0x00, 0x00, 0x54, 0x50, 0x10 } ,
        { 0x00, 0x00, 0x00, 0x66, 0x2B, 0x50 } ,
    } ,
} ;            

/* ------- Functions ------------ */

/* DS1820&2*/
static int OW_10temp(float * temp , const struct parsedname * pn) ;
static int OW_22temp(float * temp , int resolution, const struct parsedname * pn) ;
static int OW_power(unsigned char * data, const struct parsedname * pn) ;
static int OW_r_templimit( float * T, const int Tindex, const struct parsedname * pn) ;
static int OW_w_templimit( const float T, const int Tindex, const struct parsedname * pn) ;
static int OW_r_scratchpad(unsigned char * data, const struct parsedname * pn) ;
static int OW_w_scratchpad(const unsigned char * data, const struct parsedname * pn) ;
static int OW_r_trim(unsigned char * const trim, const struct parsedname * pn) ;
static int OW_w_trim(const unsigned char * const trim, const struct parsedname * pn) ;
static enum eDie OW_die( const struct parsedname * const pn ) ;

static int FS_10temp(float *T , const struct parsedname * pn) {
    if ( OW_10temp( T , pn ) ) return -EINVAL ;
    *T = Temperature(*T) ;
    return 0 ;
}

/* For DS1822 and DS18B20 -- resolution stuffed in ft->data */
static int FS_22temp(float *T , const struct parsedname * pn) {
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

static int FS_power(int * y , const struct parsedname * pn) {
    unsigned char data ;
    if ( OW_power( &data , pn ) ) return -EINVAL ;
    *y = data ;
    return 0 ;
}

static int FS_r_templimit(float * T , const struct parsedname * pn) {
    if ( OW_r_templimit( T , (int) pn->ft->data, pn ) ) return -EINVAL ;
    *T = Temperature(*T) ;
    return 0 ;
}

static int FS_w_templimit(const float * T, const struct parsedname * pn) {
    if ( OW_w_templimit( *T , (int) pn->ft->data, pn ) ) return -EINVAL ;
    return 0 ;
}

static int FS_r_die(char *buf, const size_t size, const off_t offset , const struct parsedname * pn) {
    switch ( OW_die(pn) ) {
    case eB6:
        return FS_read_return( buf, size, offset, "B6", 2 ) ;
    case eB7:
        return FS_read_return( buf, size, offset, "B7", 2 ) ;
    case eC2:
        return FS_read_return( buf, size, offset, "C2", 2 ) ;
    }
    return -EINVAL ;
}


static int FS_r_trim(unsigned char * T, const size_t size, const off_t offset , const struct parsedname * pn) {
    if ( offset ) return -EINVAL ;
    if ( OW_r_trim( T , pn ) ) return - EINVAL ;
    return 2 ;
}

static int FS_w_trim(const unsigned char * T, const size_t size, const off_t offset , const struct parsedname * pn) {
    if ( OW_die(pn) != eB7 ) return -EINVAL ;
    if ( offset ) return -EINVAL ;
    if ( OW_w_trim( T , pn ) ) return - EINVAL ;
    return 0 ;
}

/* Are the trim values valid-looking? */
static int FS_r_trimvalid(int * y , const struct parsedname * pn) {
    if ( OW_die(pn) == eB7 ) {
        unsigned char trim[2] ;
        if ( OW_r_trim( trim , pn ) ) return -EINVAL ;
        y[0] = ( ((trim[0]&0x07)==0x05) || ((trim[0]&0x07)==0x03)) && (trim[1]==0xBB);
    } else {
        y[0] = 1 ; /* Assume true */
    }
    return 0 ;
}

/* Put in a black trim value if non-zero */
static int FS_r_blanket(int * y , const struct parsedname * pn) {
    unsigned char trim[2] ;
    unsigned char blanket[] = { 0x9D, 0xBB } ;
    if ( OW_die(pn) != eB7 ) return -EINVAL ;
    if ( OW_r_trim( trim , pn ) ) return - EINVAL ;
    y[0] = ( memcmp(trim, blanket, 2) == 0 ) ;
    return 0 ;
}

/* Put in a black trim value if non-zero */
static int FS_w_blanket(const int * y , const struct parsedname * pn) {
    unsigned char blanket[] = { 0x9D, 0xBB } ;
    if ( OW_die(pn) != eB7 ) return -EINVAL ;
    if ( y[0] ) {
        if ( OW_w_trim( blanket , pn ) ) return -EINVAL ;
    }
    return 0 ;
}

/*
static int FS_tempdata(char *buf, const size_t size, const off_t offset , const struct parsedname * pn) {
    unsigned char data[8] ;
    if ( OW_r_scratchpad( data, pn ) ) return -EINVAL ;
    return FS_read_return( buf,size,offset,data,8 ) ;
}
*/
/* get the temp from the scratchpad buffer after starting a conversion and waiting */
static int OW_10temp(float * temp , const struct parsedname * pn) {
    unsigned char data[8] ;
    int delay = 750 ;
    int ret ;
    if ( pn->ft->data ) delay = (int) pn->ft->data ;
    /* Select particular device and start conversion */
    BUS_lock() ;
        ret = BUS_select(pn) || BUS_PowerByte( 0x44, delay ) ;
    BUS_unlock() ;
    if ( ret ) return 1 ;

    if ( OW_r_scratchpad( data , pn ) ) return 1 ;

    /* Check for error condition */
    if ( data[0]==0xAA && data[1]==0x00 && data[6]==0x0C ) {
        /* repeat the conversion (only once) */
        BUS_lock() ;
            ret = BUS_select(pn) || BUS_PowerByte( 0x44, delay ) ;
        BUS_unlock() ;
        if ( ret || OW_r_scratchpad( data , pn ) ) return 1 ;
    }

    /* fancy math */
    *temp = (float)((((char)data[1])<<8|data[0])>>1) + .75 - .0625*data[6] ;
    return 0 ;
}

static int OW_power( unsigned char * data, const struct parsedname * pn) {
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

/* Trim values -- undocumented except in AN247.pdf */
static int OW_r_trim(unsigned char * const trim, const struct parsedname * pn) {
    unsigned char cmd[] = { 0x93, 0x68, } ;
    int ret ;

    BUS_lock() ;
        ret =    BUS_select(pn) || BUS_send_data( &cmd[0] , 1 ) || BUS_readin_data( &trim[0] , 1 )
              || BUS_select(pn) || BUS_send_data( &cmd[1] , 1 ) || BUS_readin_data( &trim[1] , 1 ) ;
    BUS_unlock() ;
    return ret ;
}

static int OW_w_trim(const unsigned char * const trim, const struct parsedname * pn) {
    unsigned char cmd[] = { 0x95, trim[0], 0x63, trim[1], 0x94, 0x64, } ;
    int ret ;

    BUS_lock() ;
        ret =    BUS_select(pn) || BUS_send_data( &cmd[0] , 2 )
              || BUS_select(pn) || BUS_send_data( &cmd[2] , 1 )
              || BUS_select(pn) || BUS_send_data( &cmd[3] , 2 )
              || BUS_select(pn) || BUS_send_data( &cmd[5] , 1 ) ;
    BUS_unlock() ;

    return ret ;
}

static enum eDie OW_die( const struct parsedname * const pn ) {
    unsigned char die[6] = { pn->sn[6], pn->sn[5], pn->sn[4], pn->sn[3], pn->sn[2], pn->sn[1], } ;
    // data gives index into die matrix
    if ( memcmp(die, DIE[(int) pn->ft->data].C2 , 6 ) > 0 ) return eC2 ;
    if ( memcmp(die, DIE[(int) pn->ft->data].B7 , 6 ) > 0 ) return eB7 ;
    return eB6 ;
}
