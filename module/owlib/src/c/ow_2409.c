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

#include "owfs_config.h"
#include "ow_2409.h"

/* ------- Prototypes ----------- */

/* DS2409 switch */
yWRITE_FUNCTION( FS_discharge ) ;
 uREAD_FUNCTION( FS_r_control ) ;
uWRITE_FUNCTION( FS_w_control ) ;
 yREAD_FUNCTION( FS_r_sensed ) ;
 yREAD_FUNCTION( FS_r_branch ) ;
 yREAD_FUNCTION( FS_r_event ) ;

/* ------- Structures ----------- */

struct aggregate A2409 = { 2, ag_numbers, ag_aggregate, } ;
struct filetype DS2409[] = {
    F_STANDARD   ,
    {"discharge" ,     1,  NULL,    ft_yesno , ft_stable  , {v:NULL}        , {y:FS_discharge}, NULL, } ,
    {"control"   ,     1,  NULL, ft_unsigned , ft_stable  , {u:FS_r_control}, {u:FS_w_control}, NULL, } ,
    {"sensed"    ,     1,&A2409,    ft_yesno , ft_volatile, {y:FS_r_sensed} , {v:NULL}        , NULL, } ,
    {"branch"    ,     1,&A2409,    ft_yesno , ft_volatile, {y:FS_r_branch} , {v:NULL}        , NULL, } ,
    {"event"     ,     1,&A2409,    ft_yesno , ft_volatile, {y:FS_r_event}  , {v:NULL}        , NULL, } ,
    {"aux"       ,     0,  NULL, ft_directory, ft_volatile, {v:NULL}        , {v:NULL}        , (void *) 1, } ,
    {"main"      ,     0,  NULL, ft_directory, ft_volatile, {v:NULL}        , {v:NULL}        , (void *) 0, } ,
} ;
DeviceEntry( 1F, DS2409 )

/* ------- Functions ------------ */

/* DS2409 */
static int OW_r_control( unsigned char * const data, const struct parsedname * const pn ) ;

static int OW_discharge( const struct parsedname * const pn ) ;
static int OW_w_control( const unsigned int data , const struct parsedname * const pn ) ;

/* discharge 2409 lines */
int FS_discharge(const int * y, const struct parsedname * pn) {
    if ( (*y) && OW_discharge(pn) ) return -EINVAL ;
    return 0 ;
}

/* 2409 switch -- branch pin voltage */
int FS_r_sensed(int * y , const struct parsedname * pn) {
    unsigned char data ;
    if ( OW_r_control(&data,pn) ) return -EINVAL ;
    y[0] = data&0x02 ? 1 : 0 ;
    y[1] = data&0x08 ? 1 : 0 ;
    return 0 ;
}

/* 2409 switch -- branch status  -- note that bit value is reversed */
int FS_r_branch(int * y , const struct parsedname * pn) {
    unsigned char data ;
    if ( OW_r_control(&data,pn) ) return -EINVAL ;
    y[0] = data&0x01 ? 0 : 1 ;
    y[1] = data&0x04 ? 0 : 1 ;
    return 0 ;
}

/* 2409 switch -- event status */
int FS_r_event(int * y , const struct parsedname * pn) {
    unsigned char data ;
    if ( OW_r_control(&data,pn) ) return -EINVAL ;
    y[0] = data&0x10 ? 1 : 0 ;
    y[1] = data&0x20 ? 1 : 0 ;
    return 0 ;
}

/* 2409 switch -- control pin state */
int FS_r_control(unsigned int * u , const struct parsedname * pn) {
    unsigned char data ;
    unsigned int control[] = { 2, 3, 0, 1, } ;
    if ( OW_r_control(&data,pn) ) return -EINVAL ;
    *u = control[data>>6] ;
    return 0 ;
}

/* 2409 switch -- control pin state */
int FS_w_control(const unsigned int * u , const struct parsedname * pn) {
    if ( *u > 3 ) return -EINVAL ;
    if ( OW_w_control(*u,pn) ) return -EINVAL ;
    return 0 ;
}

static int OW_discharge( const struct parsedname * const pn ) {
    unsigned char dis = 0xCC ;
    unsigned char all = 0x66 ;
    int ret ;

    BUS_lock() ;
        ret = BUS_select(pn) || BUS_send_data( &dis , 1 ) ;
    BUS_unlock() ;
    if (ret) return ret ;

    UT_delay(100) ;

    BUS_lock() ;
        ret = BUS_select(pn) || BUS_send_data( &all ,1 ) ;
    BUS_unlock() ;
    return ret ;
}

static int OW_w_control( const unsigned int data , const struct parsedname * const pn ) {
    const unsigned char d[] = { 0x20, 0xA0, 0x00, 0x40, } ;
    unsigned char p[] = { 0x5A, d[data], } ;
    const unsigned char r[] = { 0x80, 0xC0, 0x00, 0x40, } ;
    unsigned char info ;
    int ret ;

    BUS_lock() ;
        (ret=BUS_select(pn)) || (ret=BUS_send_data(p,2)) || (ret=BUS_readin_data(&info,1)) ;
    BUS_unlock() ;
    if (ret) return ret ;
    /* Check that Info corresponds */
    return (info&0xC0)==r[data] ? 0 : 1 ;
}

static int OW_r_control( unsigned char * const data , const struct parsedname * const pn ) {
    unsigned char p[] = { 0x5A, 0xFF, } ;
    int ret ;

    BUS_lock() ;
        (ret=BUS_select(pn)) || (ret=BUS_send_data(p,2)) || (ret=BUS_readin_data(data,1)) ;
    BUS_unlock() ;
    return ret ;
}
