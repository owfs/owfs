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
    {"temperature",   12,  NULL, ft_temperature, fc_volatile, {f:FS_10temp}     , {v:NULL}          , {i: 1000}, } ,
    {"templow",       12,  NULL, ft_temperature, fc_stable  , {f:FS_r_templimit}, {f:FS_w_templimit}, {i: 1},   } ,
    {"temphigh",      12,  NULL, ft_temperature, fc_stable  , {f:FS_r_templimit}, {f:FS_w_templimit}, {i: 0},   } ,
    {"trim",          12,  NULL, ft_unsigned   , fc_volatile, {u:FS_r_trim}     , {u:FS_w_trim}     , {v:NULL},         } ,
    {"die",            2,  NULL, ft_ascii      , fc_static  , {a:FS_r_die}      , {v:NULL}          , {i: 1},   } ,
    {"trimvalid",      1,  NULL, ft_yesno      , fc_volatile, {y:FS_r_trimvalid}, {v:NULL}          , {v:NULL},         } ,
    {"trimblanket",    1,  NULL, ft_yesno      , fc_volatile, {y:FS_r_blanket}  , {y:FS_w_blanket}  , {v:NULL},         } ,
    {"power"     ,     1,  NULL, ft_yesno      , fc_volatile, {y:FS_power}      , {v:NULL}          , {v:NULL},         } ,
}
 ;
DeviceEntryExtended( 10, DS18S20, DEV_temp | DEV_alarm ) ;

struct filetype DS18B20[] = {
    F_STANDARD   ,
//    {"scratchpad",     8,  NULL, ft_binary, fc_volatile, FS_tempdata   , NULL, NULL, NULL,} ,
    {"temperature",   12,  NULL, ft_temperature, fc_volatile, {f:FS_22temp}     , {v:NULL}          , {i:12},  } ,
    {"fasttemp"  ,    12,  NULL, ft_temperature, fc_volatile, {f:FS_22temp}     , {v:NULL}          , {i: 9},  } ,
    {"templow",       12,  NULL, ft_temperature, fc_stable  , {f:FS_r_templimit}, {f:FS_w_templimit}, {i: 1},  } ,
    {"temphigh",      12,  NULL, ft_temperature, fc_stable  , {f:FS_r_templimit}, {f:FS_w_templimit}, {i: 0},  } ,
    {"trim",          12,  NULL, ft_unsigned   , fc_volatile, {u:FS_r_trim}     , {u:FS_w_trim}     , {v:NULL},        } ,
    {"die",            2,  NULL, ft_ascii      , fc_static  , {a:FS_r_die}      , {v:NULL}          , {i: 2},  } ,
    {"trimvalid",      1,  NULL, ft_yesno      , fc_volatile, {y:FS_r_trimvalid}, {v:NULL}          , {v:NULL},        } ,
    {"trimblanket",    1,  NULL, ft_yesno      , fc_volatile, {y:FS_r_blanket}  , {y:FS_w_blanket}  , {v:NULL},        } ,
    {"power"     ,     1,  NULL, ft_yesno      , fc_volatile, {y:FS_power}      , {v:NULL}          , {v:NULL},        } ,
} ;
DeviceEntryExtended( 28, DS18B20, DEV_temp | DEV_alarm ) ;

struct filetype DS1822[] = {
    F_STANDARD   ,
//    {"scratchpad",     8,  NULL, ft_binary, fc_volatile, FS_tempdata   , NULL, NULL, } ,
    {"temperature",   12,  NULL, ft_temperature, fc_volatile, {f:FS_22temp}     , {v:NULL}          , {i:12},  } ,
    {"fasttemp"  ,    12,  NULL, ft_temperature, fc_volatile, {f:FS_22temp}     , {v:NULL}          , {i: 9},  } ,
    {"templow",       12,  NULL, ft_temperature, fc_stable  , {f:FS_r_templimit}, {f:FS_w_templimit}, {i: 1},  } ,
    {"temphigh",      12,  NULL, ft_temperature, fc_stable  , {f:FS_r_templimit}, {f:FS_w_templimit}, {i: 0},  } ,
    {"trim",          12,  NULL, ft_unsigned   , fc_volatile, {u:FS_r_trim}     , {u:FS_w_trim}     , {v:NULL},        } ,
    {"die",            2,  NULL, ft_ascii      , fc_static  , {a:FS_r_die}      , {v:NULL}          , {i: 0},  } ,
    {"trimvalid",      1,  NULL, ft_yesno      , fc_volatile, {y:FS_r_trimvalid}, {v:NULL}          , {v:NULL},        } ,
    {"trimblanket",    1,  NULL, ft_yesno      , fc_volatile, {y:FS_r_blanket}  , {y:FS_w_blanket}  , {v:NULL},        } ,
    {"power"     ,     1,  NULL, ft_yesno      , fc_volatile, {y:FS_power}      , {v:NULL}          , {v:NULL},        } ,
} ;
DeviceEntryExtended( 22, DS1822, DEV_temp | DEV_alarm ) ;

/* Internal properties */
static struct internal_prop ip_resolution = {"RES",fc_stable} ;
static struct internal_prop ip_power = {"POW",fc_stable} ;

struct tempresolution {
    BYTE config ;
    UINT delay ;
    BYTE mask ;
} ;
struct tempresolution Resolutions[] = {
    { 0x1F, 110, 0xF8 } , /*  9 bit */
    { 0x3F, 200, 0xFC } , /* 10 bit */
    { 0x5F, 400, 0xFE } , /* 11 bit */
    { 0x7F,1000, 0xFF } , /* 12 bit */
} ;

struct die_limits {
    BYTE B7[6] ;
    BYTE C2[6] ;
} ;

enum eDie { eB6, eB7, eC2, eC3, } ;

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
static int OW_10temp(FLOAT * temp , const struct parsedname * pn) ;
static int OW_22temp(FLOAT * temp , const int resolution, const struct parsedname * pn) ;
static int OW_power(BYTE * data, const struct parsedname * pn) ;
static int OW_r_templimit( FLOAT * T, const int Tindex, const struct parsedname * pn) ;
static int OW_w_templimit( const FLOAT T, const int Tindex, const struct parsedname * pn) ;
static int OW_r_scratchpad(BYTE * data, const struct parsedname * pn) ;
static int OW_w_scratchpad(const BYTE * data, const struct parsedname * pn) ;
static int OW_r_trim(BYTE * trim, const struct parsedname * pn) ;
static int OW_w_trim(const BYTE * trim, const struct parsedname * pn) ;
static enum eDie OW_die( const struct parsedname * pn ) ;

static int FS_10temp(FLOAT *T , const struct parsedname * pn) {
    if ( OW_10temp( T , pn ) ) return -EINVAL ;
    return 0 ;
}

/* For DS1822 and DS18B20 -- resolution stuffed in ft->data */
static int FS_22temp(FLOAT *T , const struct parsedname * pn) {
    switch( pn->ft->data.i ) {
    case 9:
    case 10:
    case 11:
    case 12:
        if ( OW_22temp( T, pn->ft->data.i , pn ) ) return -EINVAL ;
        return 0 ;
    }
    return -ENODEV ;
}

static int FS_power(int * y , const struct parsedname * pn) {
    BYTE data ;
    if ( OW_power( &data , pn ) ) return -EINVAL ;
    y[0] = data!=0x00 ;
    return 0 ;
}

static int FS_r_templimit(FLOAT * T , const struct parsedname * pn) {
    if ( OW_r_templimit( T , pn->ft->data.i, pn ) ) return -EINVAL ;
    return 0 ;
}

static int FS_w_templimit(const FLOAT * T, const struct parsedname * pn) {
    if ( OW_w_templimit( T[0] , pn->ft->data.i, pn ) ) return -EINVAL ;
    return 0 ;
}

static int FS_r_die(char * buf, const size_t size, const off_t offset , const struct parsedname * pn) {
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
        case eC3:
            d = "C3" ;
            break ;
        default:
            return -EINVAL ;
    }
    memcpy(buf,&d[offset],size) ;
    return 2 ;
}

static int FS_r_trim(UINT * trim , const struct parsedname * pn) {
    BYTE t[2] ;
    if ( OW_r_trim( t , pn ) ) return - EINVAL ;
    trim[0] = (t[1]<<8) | t[0] ;
//printf("TRIM t:%.2X %.2X trim:%u \n",t[0],t[1],trim[0]) ;
    return 0 ;
}

static int FS_w_trim(const UINT * trim , const struct parsedname * pn) {
    BYTE t[2] ;
    switch( OW_die(pn) ) {
        case eB7:
        case eC2:
            t[0] = trim[0] && 0xFF ;
            t[1] = (trim[0]>>8) && 0xFF ;
            if ( OW_w_trim( t , pn ) ) return - EINVAL ;
            return 0 ;
        default:
            return -EINVAL ;
    }
}

/* Are the trim values valid-looking? */
static int FS_r_trimvalid(int * y , const struct parsedname * pn) {
    BYTE trim[2] ;
    switch( OW_die(pn) ) {
        case eB7:
        case eC2:
            if ( OW_r_trim( trim , pn ) ) return -EINVAL ;
            y[0] = ( ((trim[0]&0x07)==0x05) || ((trim[0]&0x07)==0x03)) && (trim[1]==0xBB);
            break ;
        default:
            y[0] = 1 ; /* Assume true */
    }
    return 0 ;
}

/* Put in a black trim value if non-zero */
static int FS_r_blanket(int * y , const struct parsedname * pn) {
    BYTE trim[2] ;
    BYTE blanket[] = { 0x9D, 0xBB } ;
    switch( OW_die(pn) ) {
        case eB7:
        case eC2:
            if ( OW_r_trim( trim , pn ) ) return - EINVAL ;
            y[0] = ( memcmp(trim, blanket, 2) == 0 ) ;
            return 0 ;
        default:
            return -EINVAL ;
    }
}

/* Put in a black trim value if non-zero */
static int FS_w_blanket(const int * y , const struct parsedname * pn) {
    BYTE blanket[] = { 0x9D, 0xBB } ;
    switch( OW_die(pn) ) {
        case eB7:
        case eC2:
            if ( y[0] ) {
                if ( OW_w_trim( blanket , pn ) ) return -EINVAL ;
            }
            return 0 ;
        default:
            return -EINVAL ;
    }
}

/* get the temp from the scratchpad buffer after starting a conversion and waiting */
static int OW_10temp(FLOAT * temp , const struct parsedname * pn) {
    BYTE data[9] ;
    BYTE convert[] = { 0x44, } ;
    BYTE dummy ;
    UINT delay = pn->ft->data.i ;
    BYTE pow ;
    struct transaction_log tconvert[] = {
        TRXN_START ,
        { convert, &dummy, 1, trxn_power },
        TRXN_END,
    } ;

    if ( OW_power( &pow, pn ) ) pow = 0x00 ; /* assume unpowered if cannot tell */
    
    /* Select particular device and start conversion */
    if ( !pow ) { // unpowered, deliver power, no communication allowed
        tconvert[1].size = delay ;
        if ( BUS_transaction( tconvert, pn ) ) return 1 ;
    } else if ( Simul_Test( simul_temp, delay, pn ) != 0 ) { // powered, so release bus immediately after issuing convert
        tconvert[1].type = trxn_match ;
        if ( BUS_transaction( tconvert, pn ) ) return 1 ;
        UT_delay( delay ) ;
    }

    if ( OW_r_scratchpad( data, pn ) ) return 1 ;

    /* Check for error condition */
    if ( data[0]==0xAA && data[1]==0x00 && data[6]==0x0C ) {
        /* repeat the conversion (only once) */
        if ( pow ) { // powered, so release bus immediately after issuing convert
            tconvert[1].type = trxn_match ;
            if ( BUS_transaction( tconvert, pn ) ) return 1 ;
            UT_delay( delay ) ;
        } else { // unpowered, deliver power, no communication allowed
            tconvert[1].size = delay ;
            if ( BUS_transaction( tconvert, pn ) ) return 1 ;
        }
        if ( OW_r_scratchpad( data , pn ) ) return 1 ;
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

static int OW_power( BYTE * data, const struct parsedname * pn) {
    BYTE b4[] = { 0xB4, } ;
    struct transaction_log tpower[] = {
        TRXN_START ,
        { b4, NULL, 1, trxn_match },
        { NULL, data, 1, trxn_read },
        TRXN_END,
    } ;
    size_t s = sizeof(BYTE) ;
    //printf("POWER "SNformat", before check\n",SNvar(pn->sn)) ;
    if ( IsUncachedDir(pn) || Cache_Get_Internal(data,&s,&ip_power,pn) ) {
        //printf("POWER "SNformat", need to ask\n",SNvar(pn->sn)) ;
        if ( BUS_transaction( tpower, pn ) ) return 1 ;
        //printf("POWER "SNformat", asked\n",SNvar(pn->sn)) ;
        Cache_Add_Internal(data,s,&ip_power,pn) ;
        //printf("POWER "SNformat", cached\n",SNvar(pn->sn)) ;
    }
    //printf("POWER "SNformat", done\n",SNvar(pn->sn)) ;
    return 0 ;
}

static int OW_22temp(FLOAT * temp , const int resolution, const struct parsedname * pn) {
    BYTE data[9] ;
    BYTE convert[] = { 0x44, } ;
    BYTE pow ;
    int res = Resolutions[resolution-9].config ;
    UINT delay = Resolutions[resolution-9].delay ;
    BYTE mask = Resolutions[resolution-9].mask ;
    int oldres ;
    size_t s = sizeof(oldres) ;
    struct transaction_log tconvert[] = {
        TRXN_START ,
        { convert, convert, 1, trxn_power },
        TRXN_END,
    } ;

    //LEVEL_DATA("OW_22temp\n");
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
        tconvert[1].size = delay ;
        if ( BUS_transaction( tconvert, pn ) ) return 1 ;
    } else if ( Simul_Test( simul_temp, delay, pn ) != 0 ) { // powered, so release bus immediately after issuing convert
        tconvert[1].type = trxn_match ;
        if ( BUS_transaction( tconvert, pn ) ) return 1 ;
        UT_delay( delay ) ;
    }

    if ( OW_r_scratchpad( data, pn ) ) return 1 ;

    //*temp = .0625*(((char)data[1])<<8|data[0]) ;
    // Torsten Godau <tg@solarlabs.de> found a problem with 9-bit resolution
    temp[0] = (FLOAT) ((int16_t)((data[1]<<8)|data[0]|mask)) * .0625 ;
    return 0 ;
}

/* Limits Tindex=0 high 1=low */
static int OW_r_templimit( FLOAT * T, const int Tindex, const struct parsedname * pn) {
    BYTE data[9] ;
    BYTE recall[] = { 0xB4, } ;
    struct transaction_log trecall[] = {
        TRXN_START ,
        { recall, NULL, 1, trxn_match },
        TRXN_END,
    } ;

    if ( BUS_transaction( trecall, pn ) ) return 1 ;

    UT_delay(10) ;

    if ( OW_r_scratchpad( data, pn ) ) return 1 ;
    T[0] = (FLOAT) ((int8_t)data[2+Tindex]) ;
    return 0 ;
}

/* Limits Tindex=0 high 1=low */
static int OW_w_templimit( const FLOAT T, const int Tindex, const struct parsedname * pn) {
    BYTE data[9] ;

    if ( OW_r_scratchpad( data, pn ) ) return 1 ;
    data[2+Tindex] = (uint8_t) T ;
    return OW_w_scratchpad( &data[2], pn ) ;
}

/* read 8 bytes, includes CRC8 which is checked */
static int OW_r_scratchpad(BYTE * data, const struct parsedname * pn) {
    /* data is 8 bytes long */
    BYTE be[] = { 0xBE, } ;
    struct transaction_log tread[] = {
        TRXN_START ,
        { be, NULL, 1, trxn_match },
        { NULL, data, 9, trxn_read },
        TRXN_END,
    } ;
    if ( BUS_transaction( tread, pn ) ) return 1 ;
    return CRC8( data,9 ) ;
}

/* write 3 bytes (byte2,3,4 of register) */
static int OW_w_scratchpad(const BYTE * data, const struct parsedname * pn) {
    /* data is 3 bytes long */
    BYTE d[4] = { 0x4E, data[0], data[1], data[2], } ;
    BYTE pow[] = { 0x48, } ;
    struct transaction_log twrite[] = {
        TRXN_START ,
        { d, NULL, 4, trxn_match },
        TRXN_END,
    } ;
    struct transaction_log tpower[] = {
        TRXN_START ,
        { pow, pow, 10, trxn_power },
        TRXN_END,
    } ;

    /* different processing for DS18S20 and both DS19B20 and DS1822 */
    if ( pn->sn[0]==0x10 ) twrite->size = 4 ;

    if ( BUS_transaction( twrite, pn ) ) return 1 ;
    
    return BUS_transaction( tpower, pn ) ;
}

/* Trim values -- undocumented except in AN247.pdf */
static int OW_r_trim(BYTE * trim, const struct parsedname * pn) {
    BYTE cmd0[] = { 0x93, } ;
    BYTE cmd1[] = { 0x68, } ;
    struct transaction_log t0[] = {
        TRXN_START ,
        { cmd0, NULL, 1, trxn_match },
        { NULL, &trim[0], 1, trxn_read },
        TRXN_END,
    } ;
    struct transaction_log t1[] = {
        TRXN_START ,
        { cmd1, NULL, 1, trxn_match },
        { NULL, &trim[1], 1, trxn_read },
        TRXN_END,
    } ;

    if ( BUS_transaction( t0, pn ) ) return 1 ;
    
    return BUS_transaction( t1, pn ) ;
}

static int OW_w_trim(const BYTE * trim, const struct parsedname * pn) {
    BYTE cmd0[] = { 0x95, trim[0], } ;
    BYTE cmd1[] = { 0x63, trim[1], } ;
    BYTE cmd2[] = { 0x94, } ;
    BYTE cmd3[] = { 0x64, } ;
    struct transaction_log tt[] = {
        TRXN_START ,
        { cmd0, NULL, 2, trxn_match },
        TRXN_END,
    } ;
    struct transaction_log t[] = {
        TRXN_START ,
        { cmd2, NULL, 1, trxn_match },
        TRXN_END,
    } ;

    if ( BUS_transaction( tt, pn ) ) return 1 ;
    tt[1].out = cmd1 ;
    if ( BUS_transaction( tt, pn ) ) return 1 ;
    if ( BUS_transaction( t, pn ) ) return 1 ;
    
    t[1].out = cmd3 ;
    return BUS_transaction( t, pn ) ;
}

static enum eDie OW_die( const struct parsedname * pn ) {
    BYTE die[6] = { pn->sn[6], pn->sn[5], pn->sn[4], pn->sn[3], pn->sn[2], pn->sn[1], } ;
    // data gives index into die matrix
    if ( memcmp(die, DIE[pn->ft->data.i].C2 , 6 ) > 0 ) return eC2 ;
    if ( memcmp(die, DIE[pn->ft->data.i].B7 , 6 ) > 0 ) return eB7 ;
    return eB6 ;
}
