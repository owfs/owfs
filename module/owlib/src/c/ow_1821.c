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
 fREAD_FUNCTION( FS_temperature ) ;
 yREAD_FUNCTION( FS_r_polarity ) ;
yWRITE_FUNCTION( FS_w_polarity ) ;
 fREAD_FUNCTION( FS_r_templimit ) ;
fWRITE_FUNCTION( FS_w_templimit ) ;

/* -------- Structures ---------- */
struct filetype DS1821[] = {
    F_type,
    {"temperature",   12,  NULL, ft_temperature, ft_volatile, {f:FS_temperature}, {v:NULL}          , {v:NULL},         } ,
    {"polarity",       1,  NULL, ft_yesno      , ft_stable,   {y:FS_r_polarity} , {y:FS_w_polarity} , {v:NULL},         } ,
    {"templow",       12,  NULL, ft_temperature, ft_stable  , {f:FS_r_templimit}, {f:FS_w_templimit}, {i: 1},   } ,
    {"temphigh",      12,  NULL, ft_temperature, ft_stable  , {f:FS_r_templimit}, {f:FS_w_templimit}, {i: 0},   } ,
}
 ;
DeviceEntry( thermostat, DS1821 ) ;

/* ------- Functions ------------ */

/* DS1821*/
static int OW_temperature( FLOAT * temp , const struct parsedname * pn ) ;
static int OW_current_temperature( FLOAT * temp , const struct parsedname * pn ) ;
static int OW_r_status( BYTE * data, const struct parsedname * pn) ;
static int OW_w_status( BYTE * data, const struct parsedname * pn) ;
static int OW_r_templimit( FLOAT * T, const int Tindex, const struct parsedname * pn) ;
static int OW_w_templimit( const FLOAT * T, const int Tindex, const struct parsedname * pn) ;

/* Internal properties */
static struct internal_prop ip_continuous = {"CON",ft_stable} ;

static int FS_temperature(FLOAT *T , const struct parsedname * pn) {
    if ( OW_temperature( T , pn ) ) return -EINVAL ;
    return 0 ;
}

static int FS_r_polarity( int * y , const struct parsedname * pn) {
    BYTE data ;

    if ( OW_r_status( &data, pn ) ) return -EINVAL ;
    y[0] = (data & 0x02)>>1 ;
    return 0 ;
}

static int FS_w_polarity( const int * y , const struct parsedname * pn) {
    BYTE data ;

    if ( OW_r_status( &data, pn ) ) return -EINVAL ;
    UT_setbit(&data,1,y[0]) ;
    if ( OW_w_status( &data, pn ) ) return -EINVAL ;
    return 0 ;
}


static int FS_r_templimit(FLOAT * T , const struct parsedname * pn) {
    if ( OW_r_templimit( T , pn->ft->data.i, pn ) ) return -EINVAL ;
    return 0 ;
}

static int FS_w_templimit(const FLOAT * T, const struct parsedname * pn) {
    if ( OW_w_templimit( T , pn->ft->data.i, pn ) ) return -EINVAL ;
    return 0 ;
}

static int OW_r_status( BYTE * data, const struct parsedname * pn) {
    BYTE p[] = { 0xAC, } ;
    int ret ;
    BUSLOCK(pn) ;
        ret = BUS_select(pn) || BUS_send_data( p,1,pn ) || BUS_readin_data( data,1,pn ) ;
    BUSUNLOCK(pn) ;
    return ret ;
}

static int OW_w_status( BYTE * data, const struct parsedname * pn) {
    BYTE p[] = { 0x0C, data[0] } ;
    int ret ;
    BUSLOCK(pn) ;
        ret = BUS_select(pn) || BUS_send_data( p,2,pn ) ;
    BUSUNLOCK(pn) ;
    return ret ;
}

static int OW_temperature( FLOAT * temp , const struct parsedname * pn ) {
    BYTE c[] = { 0xEE, } ;
    BYTE status ;
    int continuous ;
    int need_to_trigger = 0 ;
    size_t s = sizeof(continuous) ;
    int ret = OW_r_status( &status, pn ) ;

    if ( ret ) return ret ;

    if ( status & 0x01 ) { /* 1-shot, convert and wait 1 second */
        need_to_trigger = 1 ;
    } else { /* continuous conversion mode */
        if ( Cache_Get_Internal( &continuous,&s,&ip_continuous,pn ) || continuous==0 ) {
            need_to_trigger = 1 ;
        }
    }
    
    if ( need_to_trigger ) {
        BUSLOCK(pn) ;
        ret = BUS_select(pn) || BUS_send_data( c,1,pn) ;
        BUSUNLOCK(pn) ;
        if (ret) return ret ;
        UT_delay(1000) ;
        if ( (status & 0x01) == 0 ) { /* continuous mode, just triggered */
            continuous = 1 ;
            Cache_Add_Internal( &continuous, sizeof(int), &ip_continuous, pn ) ; /* Flag as continuous */
        }
    }
    return OW_current_temperature(temp,pn) ;
}

static int OW_current_temperature( FLOAT * temp , const struct parsedname * pn ) {
    BYTE rt[] = { 0xAA, } ;
    BYTE rc[] = { 0xA0, } ;
    BYTE lc[] = { 0x41, } ;
    BYTE temp_read ;
    BYTE count_per_c ;
    BYTE count_remain ;
    int ret ;

    BUSLOCK(pn) ;
    ret =
            BUS_select(pn) || BUS_send_data( rt,1,pn) || BUS_readin_data( &temp_read,1,pn )
        ||  BUS_select(pn) || BUS_send_data( rc,1,pn) || BUS_readin_data( &count_remain,1,pn )
        ||  BUS_select(pn) || BUS_send_data( lc,1,pn)
        ||  BUS_select(pn) || BUS_send_data( rc,1,pn) || BUS_readin_data( &count_per_c,1,pn ) ;
    
    BUSUNLOCK(pn) ;
    if ( ret ) return ret ;
    if ( count_per_c ) {
        temp[0] = (FLOAT) ((int8_t)temp_read) + .5 - ((FLOAT)count_remain)/((FLOAT)count_per_c) ;
    } else { /* Bad count_per_c -- use lower resolution */
        temp[0] = (FLOAT) ((int8_t)temp_read) ;
    }
    return 0 ;
}

/* Limits Tindex=0 high 1=low */
static int OW_r_templimit( FLOAT * T, const int Tindex, const struct parsedname * pn) {
    BYTE p[] = { 0xA1, 0xA2, } ;
    BYTE data ;
    int ret ;

    BUSLOCK(pn) ;
    ret = BUS_select(pn) || BUS_send_data( &p[Tindex],1,pn ) || BUS_readin_data( &data,1,pn ) ;
    BUSUNLOCK(pn) ;
    T[0] = (FLOAT) ((int8_t)data) ;
    return ret ;
}

/* Limits Tindex=0 high 1=low */
static int OW_w_templimit( const FLOAT * T, const int Tindex, const struct parsedname * pn) {
    BYTE p[] = { 0x01, 0x02, } ;
    signed char data = (int) (T[0]+.49);
    int ret ;

    BUSLOCK(pn) ;
    ret = BUS_select(pn) || BUS_send_data( &p[Tindex],1,pn ) || BUS_send_data( &data,1,pn ) ;
    BUSUNLOCK(pn) ;
    return ret ;
}
