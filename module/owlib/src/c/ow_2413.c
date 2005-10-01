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
#include "ow_2413.h"

/* ------- Prototypes ----------- */

/* DS2406 switch */
 uREAD_FUNCTION( FS_r_pio ) ;
uWRITE_FUNCTION( FS_w_pio ) ;
 uREAD_FUNCTION( FS_sense ) ;
 uREAD_FUNCTION( FS_r_latch ) ;

/* ------- Structures ----------- */

struct aggregate A2413 = { 2, ag_letters, ag_aggregate, } ;
struct filetype DS2413[] = {
    F_STANDARD   ,
    {"PIO"       ,     1,  &A2413,  ft_bitfield, ft_volatile, {u:FS_r_pio}    , {u:FS_w_pio} , NULL, } ,
    {"sensed"    ,     1,  &A2413,  ft_bitfield, ft_volatile, {u:FS_sense}    , {v:NULL}     , NULL, } ,
    {"latch"     ,     1,  &A2413,  ft_bitfield, ft_volatile, {u:FS_r_latch}  , {v:NULL}     , NULL, } ,
} ;
DeviceEntryExtended( 3A, DS2413, DEV_resume | DEV_ovdr ) ;

/* ------- Functions ------------ */

/* DS2413 */
static int OW_write( const unsigned char data , const struct parsedname * pn ) ;
static int OW_read( unsigned char * data , const struct parsedname * pn ) ;

/* 2413 switch */
static int FS_r_pio(unsigned int * u , const struct parsedname * pn) {
    unsigned char data ;
    unsigned char uu[] = { 0x03, 0x02, 0x03, 0x02, 0x01, 0x00, 0x01, 0x00, } ;
    if ( OW_read(&data,pn) ) return -EINVAL ;
    u[0] = uu[data&0x07] ; /* look up reversed bits */
    return 0 ;
}

/* 2413 switch PIO sensed*/
/* bits 2 and 3 */
static int FS_sense(unsigned int * u , const struct parsedname * pn) {
    if ( FS_r_pio(u,pn) ) return -EINVAL ;
    u[0] ^= 0x03 ;
    return 0 ;
}

/* 2413 switch activity latch*/
/* bites 4 and 5 */
static int FS_r_latch(unsigned int * u , const struct parsedname * pn) {
    unsigned char data ;
    unsigned char uu[] = { 0x00, 0x01, 0x00, 0x01, 0x02, 0x03, 0x02, 0x03, } ;
    if ( OW_read(&data,pn) ) return -EINVAL ;
    u[0] = uu[(data>>1)&0x07] ;
    return 0 ;
}

/* write 2413 switch -- 2 values*/
static int FS_w_pio(const unsigned int * u, const struct parsedname * pn) {
    unsigned char data = 0;
    /* reverse bits */
    data = ~u[0] ;
//    UT_setbit( &data , 0 , y[0]==0 ) ;
//    UT_setbit( &data , 1 , y[1]==0 ) ;
    if ( OW_write(data,pn) ) return -EINVAL ;
    return 0 ;
}

/* read status byte */
static int OW_read( unsigned char * data , const struct parsedname * pn ) {
    unsigned char p[] = { 0xF5, } ;
    int ret ;

    BUSLOCK(pn);
    BUSUNLOCK(pn);
    return ret ;
}

/* write status byte */
static int OW_write( const unsigned char data , const struct parsedname * pn ) {
    unsigned char p[] = { 0x5A, data|0xFC , ~data&0x03, } ;
    unsigned char q[2] ;
    int ret ;

    BUSLOCK(pn);
        ret = BUS_select(pn) || BUS_send_data( p , 3,pn ) || BUS_readin_data( q, 2,pn ) || q[0]!=0xAA ;
    BUSUNLOCK(pn);
    return ret ;
}

