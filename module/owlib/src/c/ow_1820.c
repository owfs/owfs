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
 uREAD_FUNCTION( FS_r_trim ) ;
uWRITE_FUNCTION( FS_w_trim ) ;
 yREAD_FUNCTION( FS_r_trimvalid ) ;
 yREAD_FUNCTION( FS_r_blanket ) ;
yWRITE_FUNCTION( FS_w_blanket ) ;

/* -------- Structures ---------- */
struct filetype DS18S20[] = {
    F_STANDARD   ,
    {"temperature",   12,  NULL, ft_float   , ft_volatile, {f:FS_10temp}     , {v:NULL}          , (void *) 1000, } ,
    {"templow",       12,  NULL, ft_float   , ft_stable  , {f:FS_r_templimit}, {f:FS_w_templimit}, (void *) 1,   } ,
    {"temphigh",      12,  NULL, ft_float   , ft_stable  , {f:FS_r_templimit}, {f:FS_w_templimit}, (void *) 0,   } ,
    {"trim",          12,  NULL, ft_unsigned, ft_volatile, {u:FS_r_trim}     , {u:FS_w_trim}     , NULL,         } ,
    {"die",            2,  NULL, ft_ascii   , ft_static  , {a:FS_r_die}      , {v:NULL}          , (void *) 1,   } ,
    {"trimvalid",      1,  NULL, ft_yesno   , ft_volatile, {y:FS_r_trimvalid}, {v:NULL}          , NULL,         } ,
    {"trimblanket",    1,  NULL, ft_yesno   , ft_volatile, {y:FS_r_blanket}  , {y:FS_w_blanket}  , NULL,         } ,
    {"power"     ,     1,  NULL, ft_yesno   , ft_volatile, {y:FS_power}      , {v:NULL}          , NULL,         } ,
}
 ;
DeviceEntryExtended( 10, DS18S20, DEV_temp )

struct filetype DS18B20[] = {
    F_STANDARD   ,
//    {"scratchpad",     8,  NULL, ft_binary, ft_volatile, FS_tempdata   , NULL, NULL, NULL,} ,
    {"temperature",   12,  NULL, ft_float   , ft_volatile, {f:FS_22temp}     , {v:NULL}          , (void *)12,  } ,
    {"fasttemp"  ,    12,  NULL, ft_float   , ft_volatile, {f:FS_22temp}     , {v:NULL}          , (void *) 9,  } ,
    {"templow",       12,  NULL, ft_float   , ft_stable  , {f:FS_r_templimit}, {f:FS_w_templimit}, (void *) 1,  } ,
    {"temphigh",      12,  NULL, ft_float   , ft_stable  , {f:FS_r_templimit}, {f:FS_w_templimit}, (void *) 0,  } ,
    {"trim",          12,  NULL, ft_unsigned, ft_volatile, {u:FS_r_trim}     , {u:FS_w_trim}     , NULL,        } ,
    {"die",            2,  NULL, ft_ascii   , ft_static  , {a:FS_r_die}      , {v:NULL}          , (void *) 2,  } ,
    {"trimvalid",      1,  NULL, ft_yesno   , ft_volatile, {y:FS_r_trimvalid}, {v:NULL}          , NULL,        } ,
    {"trimblanket",    1,  NULL, ft_yesno   , ft_volatile, {y:FS_r_blanket}  , {y:FS_w_blanket}  , NULL,        } ,
    {"power"     ,     1,  NULL, ft_yesno   , ft_volatile, {y:FS_power}      , {v:NULL}          , NULL,        } ,
} ;
DeviceEntryExtended( 28, DS18B20, DEV_temp )

struct filetype DS1822[] = {
    F_STANDARD   ,
//    {"scratchpad",     8,  NULL, ft_binary, ft_volatile, FS_tempdata   , NULL, NULL, } ,
    {"temperature",   12,  NULL, ft_float   , ft_volatile, {f:FS_22temp}     , {v:NULL}          , (void *)12,  } ,
    {"fasttemp"  ,    12,  NULL, ft_float   , ft_volatile, {f:FS_22temp}     , {v:NULL}          , (void *) 9,  } ,
    {"templow",       12,  NULL, ft_float   , ft_stable  , {f:FS_r_templimit}, {f:FS_w_templimit}, (void *) 1,  } ,
    {"temphigh",      12,  NULL, ft_float   , ft_stable  , {f:FS_r_templimit}, {f:FS_w_templimit}, (void *) 0,  } ,
    {"trim",          12,  NULL, ft_unsigned, ft_volatile, {u:FS_r_trim}     , {u:FS_w_trim}     , NULL,        } ,
    {"die",            2,  NULL, ft_ascii   , ft_static  , {a:FS_r_die}      , {v:NULL}          , (void *) 0,  } ,
    {"trimvalid",      1,  NULL, ft_yesno   , ft_volatile, {y:FS_r_trimvalid}, {v:NULL}          , NULL,        } ,
    {"trimblanket",    1,  NULL, ft_yesno   , ft_volatile, {y:FS_r_blanket}  , {y:FS_w_blanket}  , NULL,        } ,
    {"power"     ,     1,  NULL, ft_yesno   , ft_volatile, {y:FS_power}      , {v:NULL}          , NULL,        } ,
} ;
DeviceEntryExtended( 22, DS1822, DEV_temp )

/* Internal properties */
static struct internal_prop ip_resolution = {"RES",ft_stable} ;
static struct internal_prop ip_power = {"POW",ft_stable} ;

struct tempresolution {
    unsigned char config ;
    unsigned int delay ;
} ;
struct tempresolution Resolutions[] = {
//    { 0x1F,  94, } , /*  9 bit */
//    { 0x3F, 188, } , /* 10 bit */
//    { 0x5F, 375, } , /* 11 bit */
//    { 0x7F, 750, } , /* 12 bit */
    { 0x1F, 110, } , /*  9 bit */
    { 0x3F, 200, } , /* 10 bit */
    { 0x5F, 400, } , /* 11 bit */
    { 0x7F,1000, } , /* 12 bit */
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

/* Intermediate cached values  -- start with unlikely asterisk */
/* RES -- resolution
   POW -- power
*/

/* ------- Functions ------------ */

/* DS1820&2*/
static int OW_10temp(FLOAT * const temp , const struct parsedname * const pn) ;
static int OW_22temp(FLOAT * const temp , const int resolution, const struct parsedname * const pn) ;
static int OW_power(unsigned char * const data, const struct parsedname * const pn) ;
static int OW_r_templimit( FLOAT * const T, const int Tindex, const struct parsedname * const pn) ;
static int OW_w_templimit( const FLOAT T, const int Tindex, const struct parsedname * const pn) ;
static int OW_r_scratchpad(unsigned char * const data, const struct parsedname * const pn) ;
static int OW_w_scratchpad(const unsigned char * const data, const struct parsedname * const pn) ;
static int OW_r_trim(unsigned char * const trim, const struct parsedname * const pn) ;
static int OW_w_trim(const unsigned char * const trim, const struct parsedname * const pn) ;
static enum eDie OW_die( const struct parsedname * const pn ) ;

static int FS_10temp(FLOAT *T , const struct parsedname * pn) {
    if ( OW_10temp( T , pn ) ) return -EINVAL ;
    *T = Temperature(*T) ;
    return 0 ;
}

/* For DS1822 and DS18B20 -- resolution stuffed in ft->data */
static int FS_22temp(FLOAT *T , const struct parsedname * pn) {
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
    y[0] = data!=0x00 ;
    return 0 ;
}

static int FS_r_templimit(FLOAT * T , const struct parsedname * pn) {
    if ( OW_r_templimit( T , (int) pn->ft->data, pn ) ) return -EINVAL ;
    *T = Temperature(*T) ;
    return 0 ;
}

static int FS_w_templimit(const FLOAT * T, const struct parsedname * pn) {
    if ( OW_w_templimit( *T , (int) pn->ft->data, pn ) ) return -EINVAL ;
    return 0 ;
}

static int FS_r_die(char *buf, const size_t size, const off_t offset , const struct parsedname * pn) {
    const char * d ;
    switch ( OW_die(pn) ) {
    case eB6:
        d = "B6" ;
        break ;
    case eB7:
        d = "B7" ;
        break ;
    case eC2:
        d = "C2" ;
        break ;
    default:
        return -EINVAL ;
    }
    memcpy(buf,&d[offset],size) ;
    return 2 ;
}

static int FS_r_trim(unsigned int * const trim , const struct parsedname * pn) {
    unsigned char t[2] ;
    if ( OW_r_trim( t , pn ) ) return - EINVAL ;
    trim[0] = (t[1]<<8) | t[0] ;
//printf("TRIM t:%.2X %.2X trim:%u \n",t[0],t[1],trim[0]) ;
    return 0 ;
}

static int FS_w_trim(const unsigned int * const trim , const struct parsedname * pn) {
    unsigned char t[2] ;
    if ( OW_die(pn) != eB7 ) return -EINVAL ;
    t[0] = trim[0] && 0xFF ;
    t[1] = (trim[0]>>8) && 0xFF ;
    if ( OW_w_trim( t , pn ) ) return - EINVAL ;
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

/* get the temp from the scratchpad buffer after starting a conversion and waiting */
static int OW_10temp(FLOAT * const temp , const struct parsedname * const pn) {
    unsigned char data[8] ;
    unsigned char convert = 0x44 ;
    int delay = (int) pn->ft->data ;
    unsigned char pow ;
    int ret = 0 ;

    if ( OW_power( &pow, pn ) ) pow = 0x00 ; /* assume unpowered if cannot tell */
    /* Select particular device and start conversion */
    if ( ! pow ) { // unpowered, deliver power, no communication allowed
        BUSLOCK
            ret = BUS_select(pn) || BUS_PowerByte( convert, delay ) ;
        BUSUNLOCK
    } else if ( Simul_Test( simul_temp, delay, pn ) != 0 ){ // powered, so release bus immediately after issuing convert
        BUSLOCK
            ret = BUS_select(pn) || BUS_send_data( &convert, 1 ) ;
        BUSUNLOCK
        UT_delay( delay ) ;
    }
    if ( ret ) return 1 ;

    if ( OW_r_scratchpad( data , pn ) ) return 1 ;

    /* Check for error condition */
    if ( data[0]==0xAA && data[1]==0x00 && data[6]==0x0C ) {
        /* repeat the conversion (only once) */
        if ( pow ) { // powered, so release bus immediately after issuing convert
            BUSLOCK
                ret = BUS_select(pn) || BUS_send_data( &convert, 1 ) ;
            BUSUNLOCK
            UT_delay( delay ) ;
        } else { // unpowered, deliver power, no communication allowed
            BUSLOCK
                ret = BUS_select(pn) || BUS_PowerByte( convert, delay ) ;
            BUSUNLOCK
        }
        if ( ret || OW_r_scratchpad( data , pn ) ) return 1 ;
    }

    // Correction thanks to Nathan D. Holmes
    //temp[0] = (FLOAT) ((int16_t)(data[1]<<8|data[0])) * .5 ; // Main conversion
    // Further correction, using "truncation" thanks to Wim Heirman
    temp[0] = (FLOAT) ((int16_t)(data[1]<<8|data[0])>>1); // Main conversion
    if ( data[7] ) { // only if COUNT_PER_C non-zero (supposed to be!)
//        temp[0] += (FLOAT)(data[7]-data[6]) / (FLOAT)data[7] - .25 ; // additional precision
        temp[0] += .75 - (FLOAT)data[6] / (FLOAT)data[7] ; // additional precision
    }
    return 0 ;
}

static int OW_power( unsigned char * const data, const struct parsedname * const pn) {
    unsigned char b4 = 0xB4;
    int ret = 0 ;
    size_t s = sizeof(unsigned char) ;

    if ( (pn->state & pn_uncached) || Cache_Get_Internal(data,&s,&ip_power,pn) ) {
        BUSLOCK
            ret = BUS_select(pn) || BUS_send_data( &b4 , 1 ) || BUS_readin_data( data , 1 ) ;
//printf("Uncached power = %d\n",data[0]) ;
        BUSUNLOCK
        Cache_Add_Internal(data,s,&ip_power,pn) ;
//    } else {
//printf("Cached power = %d\n",data[0]) ;
    }
    return ret ;
}

static int OW_22temp(FLOAT * const temp , const int resolution, const struct parsedname * const pn) {
    unsigned char data[8] ;
    unsigned char convert = 0x44 ;
    unsigned char pow ;
    int res = Resolutions[resolution-9].config ;
    int delay = Resolutions[resolution-9].delay ;
    int oldres ;
    size_t s = sizeof(oldres) ;
    int ret = 0 ;

    /* powered? */
    if ( OW_power( &pow, pn ) ) pow = 0x00 ; /* assume unpowered if cannot tell */

    /* Resolution */
    if ( Cache_Get_Internal(&oldres,&s,&ip_resolution,pn) || oldres!=resolution ) {
        /* Get existing settings */
        if ( OW_r_scratchpad(data , pn ) ) return 1 ;
            /* Put in new settings */
            if ( data[4] != res ) {
                data[4] = res ;
            if ( OW_w_scratchpad(&data[2] , pn ) ) return 1 ;
            Cache_Add_Internal(&resolution,sizeof(int),&ip_resolution,pn) ;
        }
    }

    /* Conversion */
    if ( !pow ) { // unpowered, deliver power, no communication allowed
        BUSLOCK
            ret = BUS_select(pn) || BUS_PowerByte( convert, delay ) ;
        BUSUNLOCK
    } else if ( Simul_Test( simul_temp, delay, pn ) != 0 ) { // powered, so release bus immediately after issuing convert
        BUSLOCK
            ret = BUS_select(pn) || BUS_send_data( &convert, 1 ) ;
        BUSUNLOCK
        UT_delay( delay ) ;
    }
    if ( ret ) return 1 ;

    if ( OW_r_scratchpad( data, pn ) ) return 1 ;

//    *temp = .0625*(((char)data[1])<<8|data[0]) ;
    temp[0] = (FLOAT) ((int16_t)(data[1]<<8|data[0])) * .0625 ;
    return 0 ;
}

/* Limits Tindex=0 high 1=low */
static int OW_r_templimit( FLOAT * const T, const int Tindex, const struct parsedname * const pn) {
    unsigned char data[8] ;
    unsigned char recall = 0xB4 ;
    int ret ;

    BUSLOCK
        ret = BUS_select(pn) || BUS_send_data( &recall , 1 ) ;
    BUSUNLOCK
    if ( ret ) return 1 ;

    UT_delay(10) ;

    if ( OW_r_scratchpad( data, pn ) ) return 1 ;
//    *T = (char) data[2+Tindex] ;
    T[0] = (FLOAT) ((int8_t)data[2+Tindex]) ;
    return 0 ;
}

/* Limits Tindex=0 high 1=low */
static int OW_w_templimit( const FLOAT T, const int Tindex, const struct parsedname * const pn) {
    unsigned char data[8] ;

    if ( OW_r_scratchpad( data, pn ) ) return 1 ;
    data[2+Tindex] = T ;
    return OW_w_scratchpad( &data[2], pn ) ;
}

/* read 8 bytes, includes CRC8 which is checked */
static int OW_r_scratchpad(unsigned char * const data, const struct parsedname * const pn) {
    /* data is 8 bytes long */
    unsigned char be = 0xBE ;
    unsigned char td[9] ;
    int ret ;

    BUSLOCK
        ret = BUS_select(pn) || BUS_send_data( &be , 1 ) || BUS_readin_data( td , 9 ) || CRC8(td,9) ;
    BUSUNLOCK
    if ( ret ) return 1 ;

    memcpy( data , td , 8 ) ;
    return 0 ;
}

/* write 3 bytes (byte2,3,4 of register) */
static int OW_w_scratchpad(const unsigned char * const data, const struct parsedname * const pn) {
    /* data is 3 bytes ng */
    unsigned char d[] = { 0x4E, data[0], data[1], data[2], } ;
    int ret ;

    BUSLOCK
        ret = BUS_select(pn) || BUS_send_data( d , 4 ) || BUS_select(pn) || BUS_PowerByte( 0x48, 10 ) ;
    BUSUNLOCK
    return ret ;
}

/* Trim values -- undocumented except in AN247.pdf */
static int OW_r_trim(unsigned char * const trim, const struct parsedname * const pn) {
    unsigned char cmd[] = { 0x93, 0x68, } ;
    int ret ;

    BUSLOCK
        ret =    BUS_select(pn) || BUS_send_data( &cmd[0] , 1 ) || BUS_readin_data( &trim[0] , 1 )
              || BUS_select(pn) || BUS_send_data( &cmd[1] , 1 ) || BUS_readin_data( &trim[1] , 1 ) ;
    BUSUNLOCK
    return ret ;
}

static int OW_w_trim(const unsigned char * const trim, const struct parsedname * const pn) {
    unsigned char cmd[] = { 0x95, trim[0], 0x63, trim[1], 0x94, 0x64, } ;
    int ret ;

    BUSLOCK
        ret =    BUS_select(pn) || BUS_send_data( &cmd[0] , 2 )
              || BUS_select(pn) || BUS_send_data( &cmd[2] , 2 )
              || BUS_select(pn) || BUS_send_data( &cmd[4] , 1 )
              || BUS_select(pn) || BUS_send_data( &cmd[5] , 1 ) ;
    BUSUNLOCK

    return ret ;
}

static enum eDie OW_die( const struct parsedname * const pn ) {
    unsigned char die[6] = { pn->sn[6], pn->sn[5], pn->sn[4], pn->sn[3], pn->sn[2], pn->sn[1], } ;
    // data gives index into die matrix
    if ( memcmp(die, DIE[(int) pn->ft->data].C2 , 6 ) > 0 ) return eC2 ;
    if ( memcmp(die, DIE[(int) pn->ft->data].B7 , 6 ) > 0 ) return eB7 ;
    return eB6 ;
}
