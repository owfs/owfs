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
#include "ow_xxxx.h"

/* ------- Functions ------------ */

int FS_type(char *buf, const size_t size, const off_t offset , const struct parsedname * pn) {
    size_t len = strlen(pn->dev->name) ;
    if ( offset ) return -EFAULT ;
    if ( len > size ) return -EMSGSIZE ;
    strcpy( buf , pn->dev->name ) ;
    return len ;
}

int FS_code(char *buf, const size_t size, const off_t offset , const struct parsedname * pn) {
    if ( offset ) return -EFAULT ;
    if ( size < 2 ) return -EMSGSIZE ;
    num2string( buf , pn->sn[0] ) ;
    return 2 ;
}

int FS_ID(char *buf, const size_t size, const off_t offset , const struct parsedname * pn) {
    if ( offset ) return -EFAULT ;
    if ( size < 12 ) return -EMSGSIZE ;
    bytes2string( buf , &(pn->sn[1]) , 6 ) ;
    return 12 ;
}

int FS_crc8(char *buf, const size_t size, const off_t offset , const struct parsedname * pn) {
    if ( offset ) return -EFAULT ;
    if ( size < 2 ) return -EMSGSIZE ;
    num2string(buf,pn->sn[7]) ;
    return 2 ;
}

int FS_address(char *buf, const size_t size, const off_t offset , const struct parsedname * pn) {
    if ( offset ) return -EFAULT ;
    if ( size < 16 ) return -EMSGSIZE ;
    bytes2string( buf , pn->sn , 8 ) ;
    return 16 ;
}

/* Check if device exists -- 0 yes, 1 no */
int CheckPresence( const struct parsedname * const pn ) {
    int ret = 0 ;
    if ( pn->type == pn_real ) {
        if ( pn->dev == DeviceSimultaneous ) return 0 ;
        BUSLOCK
            ret = BUS_normalverify(pn) ;
        BUSUNLOCK
    }
    return ret ;
}

int FS_present(int * y , const struct parsedname * pn) {
    *y = !CheckPresence(pn);
    return 0 ;
}
