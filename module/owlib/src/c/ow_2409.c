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

#include <config.h>
#include "owfs_config.h"
#include "ow_2409.h"

/* ------- Prototypes ----------- */

/* DS2409 switch */
yWRITE_FUNCTION( FS_discharge ) ;
 uREAD_FUNCTION( FS_r_control ) ;
uWRITE_FUNCTION( FS_w_control ) ;
 uREAD_FUNCTION( FS_r_sensed ) ;
 uREAD_FUNCTION( FS_r_branch ) ;
 uREAD_FUNCTION( FS_r_event ) ;

/* ------- Structures ----------- */

struct aggregate A2409 = { 2, ag_numbers, ag_aggregate, } ;
struct filetype DS2409[] = {
    F_STANDARD   ,
    {"discharge" ,     1,  NULL,    ft_yesno , fc_stable  , {v:NULL}        , {y:FS_discharge}, {v:NULL},       } ,
    {"control"   ,     1,  NULL, ft_unsigned , fc_stable  , {u:FS_r_control}, {u:FS_w_control}, {v:NULL},       } ,
    {"sensed"    ,     1,&A2409, ft_bitfield , fc_volatile, {u:FS_r_sensed} , {v:NULL}        , {v:NULL},       } ,
    {"branch"    ,     1,&A2409, ft_bitfield , fc_volatile, {u:FS_r_branch} , {v:NULL}        , {v:NULL},       } ,
    {"event"     ,     1,&A2409, ft_bitfield , fc_volatile, {u:FS_r_event}  , {v:NULL}        , {v:NULL},       } ,
    {"aux"       ,     0,  NULL, ft_directory, fc_volatile, {v:NULL}        , {v:NULL}        , {i: 1}, } ,
    {"main"      ,     0,  NULL, ft_directory, fc_volatile, {v:NULL}        , {v:NULL}        , {i: 0}, } ,
} ;
DeviceEntry( 1F, DS2409 ) ;

/* ------- Functions ------------ */

/* DS2409 */
static int OW_r_control( BYTE * data, const struct parsedname * pn ) ;

static int OW_discharge( const struct parsedname * pn ) ;
static int OW_w_control( const UINT data , const struct parsedname * pn ) ;

/* discharge 2409 lines */
static int FS_discharge(const int * y, const struct parsedname * pn) {
    if ( (*y) && OW_discharge(pn) ) return -EINVAL ;
    return 0 ;
}

/* 2409 switch -- branch pin voltage */
static int FS_r_sensed(UINT * u , const struct parsedname * pn) {
    BYTE data ;
    if ( OW_r_control(&data,pn) ) return -EINVAL ;
//    y[0] = data&0x02 ? 1 : 0 ;
//    y[1] = data&0x08 ? 1 : 0 ;
    u[0] = ((data>>1)&0x01) | ((data>>2)&0x02) ;
    return 0 ;
}

/* 2409 switch -- branch status  -- note that bit value is reversed */
static int FS_r_branch(UINT * u , const struct parsedname * pn) {
    BYTE data ;
    if ( OW_r_control(&data,pn) ) return -EINVAL ;
//    y[0] = data&0x01 ? 0 : 1 ;
//    y[1] = data&0x04 ? 0 : 1 ;
    u[0] = (((data)&0x01) | ((data>>1)&0x02)) ^ 0x03 ;
    return 0 ;
}

/* 2409 switch -- event status */
static int FS_r_event(UINT * u , const struct parsedname * pn) {
    BYTE data ;
    if ( OW_r_control(&data,pn) ) return -EINVAL ;
//    y[0] = data&0x10 ? 1 : 0 ;
//    y[1] = data&0x20 ? 1 : 0 ;
    u[0] = (data>>4)&0x03 ;
    return 0 ;
}

/* 2409 switch -- control pin state */
static int FS_r_control(UINT * u , const struct parsedname * pn) {
    BYTE data ;
    UINT control[] = { 2, 3, 0, 1, } ;
    if ( OW_r_control(&data,pn) ) return -EINVAL ;
    *u = control[data>>6] ;
    return 0 ;
}

/* 2409 switch -- control pin state */
static int FS_w_control(const UINT * u , const struct parsedname * pn) {
    if ( *u > 3 ) return -EINVAL ;
    if ( OW_w_control(*u,pn) ) return -EINVAL ;
    return 0 ;
}

static int OW_discharge( const struct parsedname * pn ) {
    BYTE dis[] = { 0xCC, } ;
    struct transaction_log t[] = {
        TRXN_START,
        { dis, NULL, 1, trxn_match } ,
        TRXN_END,
    } ;

    if ( BUS_transaction( t, pn ) ) return 1 ;

    UT_delay(100) ;

    dis[0] = 0x66 ;
    if ( BUS_transaction( t, pn ) ) return 1 ;

    return 0 ;
}

static int OW_w_control( const UINT data , const struct parsedname * pn ) {
    const BYTE d[] = { 0x20, 0xA0, 0x00, 0x40, } ;
    BYTE p[] = { 0x5A, d[data], } ;
    const BYTE r[] = { 0x80, 0xC0, 0x00, 0x40, } ;
    BYTE info ;
    struct transaction_log t[] = {
        TRXN_START,
        { p, NULL, 2, trxn_match } ,
        { NULL, &info, 1, trxn_read } ,
        TRXN_END,
    } ;

    if ( BUS_transaction( t, pn ) ) return 1 ;

    /* Check that Info corresponds */
    return (info&0xC0)==r[data] ? 0 : 1 ;
}

static int OW_r_control( BYTE * data , const struct parsedname * pn ) {
    BYTE p[] = { 0x5A, 0xFF, } ;
    struct transaction_log t[] = {
        TRXN_START,
        { p, NULL, 2, trxn_match } ,
        { NULL, data, 1, trxn_read } ,
        TRXN_END,
    } ;

    if ( BUS_transaction( t, pn ) ) return 1 ;
    return 0 ;
}
