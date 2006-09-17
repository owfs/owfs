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
#include "ow_2890.h"

/* ------- Prototypes ----------- */

/* DS2890 Digital Potentiometer */
 yREAD_FUNCTION( FS_r_cp ) ;
yWRITE_FUNCTION( FS_w_cp ) ;
 uREAD_FUNCTION( FS_r_wiper ) ;
uWRITE_FUNCTION( FS_w_wiper ) ;

/* ------- Structures ----------- */

struct filetype DS2890[] = {
    F_STANDARD   ,
    {"chargepump",     1,  NULL, ft_yesno  , fc_stable  , {y:FS_r_cp}       , {y:FS_w_cp}   , {v:NULL}, } ,
    {"wiper"     ,     3,  NULL,ft_unsigned, fc_stable  , {u:FS_r_wiper}    , {u:FS_w_wiper}, {v:NULL}, } ,
} ;
DeviceEntryExtended( 2C, DS2890, DEV_alarm | DEV_resume | DEV_ovdr ) ;

/* ------- Functions ------------ */

/* DS2890 */
static int OW_r_wiper(UINT *val, const struct parsedname * pn) ;
static int OW_w_wiper(const UINT val, const struct parsedname * pn) ;
static int OW_r_cp(int * val , const struct parsedname * pn) ;
static int OW_w_cp(const int val, const struct parsedname * pn) ;

/* Wiper */
static int FS_w_wiper(const UINT * i , const struct parsedname * pn) {
    UINT num=i[0] ;
    if ( num>255 ) num=255 ;

    if ( OW_w_wiper(num,pn) ) return -EINVAL ;
    return 0 ;
}

/* write Charge Pump */
static int FS_w_cp(const int * y , const struct parsedname * pn) {
    if ( OW_w_cp(y[0],pn) ) return -EINVAL ;
    return 0 ;
}

/* read Wiper */
static int FS_r_wiper(UINT * w , const struct parsedname * pn) {
    if ( OW_r_wiper( w , pn ) ) return -EINVAL ;
    return 0 ;
}

/* Charge Pump */
static int FS_r_cp(int * y , const struct parsedname * pn) {
    if ( OW_r_cp(y,pn) ) return -EINVAL ;
    return 0 ;
}

/* write Wiper */
static int OW_w_wiper(const UINT val, const struct parsedname * pn) {
    BYTE resp[1] ;
    BYTE cmd[] = { 0x0F , (BYTE) val, } ;
    BYTE ns[] = { 0x96, } ;
    struct transaction_log t[] = {
        TRXN_START,
        { cmd, NULL, 2, trxn_match, } ,
        { NULL, resp, 1, trxn_read, } ,
        { ns, NULL, 1, trxn_match, } ,
        TRXN_END
    } ;

    return BUS_transaction(t,pn) || resp[0]!=val ;
}

/* read Wiper */
static int OW_r_wiper(UINT *val, const struct parsedname * pn) {
    BYTE fo[]={0xF0,} ;
    BYTE resp[2] ;
    struct transaction_log t[] = {
        TRXN_START,
        { fo, NULL, 1, trxn_match, } ,
        { NULL, resp, 2, trxn_read, } ,
        TRXN_END
    } ;

    if ( BUS_transaction(t,pn) ) return 1 ;

    *val = resp[1] ;
    return 0 ;
}

/* write Charge Pump */
static int OW_w_cp(const int val, const struct parsedname * pn) {
    BYTE resp[1] ;
    BYTE cmd[] = { 0x55 , (val)?0x4C:0x0C } ;
    BYTE ns[] = { 0x96, } ;
    struct transaction_log t[] = {
        TRXN_START,
        { cmd, NULL, 2, trxn_match, } ,
        { NULL, resp, 1, trxn_read, } ,
        { ns, NULL, 1, trxn_match, } ,
        TRXN_END
    } ;

    return BUS_transaction(t,pn) || resp[0]!=cmd[1] ;
}

/* read Charge Pump */
static int OW_r_cp(int * val , const struct parsedname * pn) {
    BYTE aa[] = { 0xAA, } ;
    BYTE resp[2] ;
    struct transaction_log t[] = {
        TRXN_START,
        { aa, NULL, 1, trxn_match, } ,
        { NULL, resp, 2, trxn_read, } ,
        TRXN_END
    } ;

    if ( BUS_transaction(t,pn) ) return 1 ;

    *val = resp[1] & 0x40 ;
    return 0 ;
}
