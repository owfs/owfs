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
#include "ow_2405.h"

/* ------- Prototypes ----------- */

/* DS2405 */
 yREAD_FUNCTION( FS_r_2405sense ) ;
 yREAD_FUNCTION( FS_r_2405 ) ;
yWRITE_FUNCTION( FS_w_2405 ) ;

/* ------- Structures ----------- */

struct filetype DS2405[] = {
    F_STANDARD   ,
    {"PIO"       ,     1,  NULL, ft_yesno , ft_stable  , {y:FS_r_2405}     , {y:FS_w_2405} , NULL,} ,
    {"sensed"    ,     1,  NULL, ft_yesno , ft_volatile, {y:FS_r_2405sense}, {v:NULL}, NULL, } ,
} ;
DeviceEntry( 05, DS2405 )

/* ------- Functions ------------ */

static int OW_r_2405sense( int * val , const struct parsedname * pn) ;
static int OW_r_2405( int * val , const struct parsedname * pn) ;
static int OW_w_2405( int val , const struct parsedname * pn) ;

/* 2405 switch */
int FS_r_2405(int * y , const struct parsedname * pn) {
    int num ;
    if ( OW_r_2405(&num,pn) ) return -EINVAL ;
    y[0] = (num!=0) ;
    return 0 ;
}

/* 2405 switch */
int FS_r_2405sense(int * y , const struct parsedname * pn) {
    int num ;
    if ( OW_r_2405sense(&num,pn) ) return -EINVAL ;
    y[0] = (num!=0) ;
    return 0 ;
}

/* write 2405 switch */
int FS_w_2405(const int * y, const struct parsedname * pn) {
    if ( OW_w_2405(y[0],pn) ) return -EINVAL ;
    return 0 ;
}

/* read the sense of the DS2405 switch */
static int OW_r_2405sense( int * val , const struct parsedname * pn) {
    unsigned char inp ;
    int ret ;

    BUS_lock() ;
        ret = BUS_normalverify(pn) || BUS_readin_data(&inp,1) ;
    BUS_unlock() ;
    if ( ret ) return 1 ;

     *val = (inp!=0) ;
    return 0 ;
}

/* read the state of the DS2405 switch */
static int OW_r_2405( int * val , const struct parsedname * pn) {
    int ret ;

    BUS_lock() ;
        ret = BUS_alarmverify(pn) ;
    BUS_unlock() ;
    if ( ret ) {
        BUS_lock() ;
            ret = BUS_normalverify(pn) ;
        BUS_unlock() ;
        if ( ret ) return -ENOENT ;
         *val = 0 ;
    } else {
        *val = 1 ;
    }
    return 0 ;
}

/* write (set) the state of the DS2405 switch */
static int OW_w_2405( const int val , const struct parsedname * pn) {
    int current ;
    int ret = 0 ;
    
    if ( OW_r_2405(&current,pn) ) return 1 ;
    
    BUS_lock() ;
    if ( current != val ) {
        ret = BUS_select(pn) ;
    }
    ret |= BUS_reset() ;
 //printf("2405write current=%d new=%d\n",current,val) ;
    BUS_unlock() ;
    return ret ;
}


