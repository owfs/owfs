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
#include "ow_xxxx.h"
#include "ow_counters.h"
#include "ow_connection.h"

/* ------- Prototypes ------------ */

/* ------- Functions ------------ */

/*
 * Funcation to calculated maximum delay between writing a
 * command to the bus, and reading the answer.
 * This will probably not work with simultanious reading...
 */
void update_max_delay(const struct parsedname * pn) {
    long sec, usec;
    struct timeval *r, *w;
    struct timeval last_delay;
    if(!pn || !pn->in) return;
    gettimeofday( &(pn->in->bus_read_time) , NULL );
    r = &pn->in->bus_read_time;
    w = &pn->in->bus_write_time;

    sec = r->tv_sec - w->tv_sec;
    if((sec >= 0) && (sec <= 5)) {
        usec = r->tv_usec - w->tv_usec;
        last_delay.tv_sec = sec;
        last_delay.tv_usec = usec;

        while( last_delay.tv_usec >= 1000000 ) {
            last_delay.tv_usec -= 1000000 ;
            last_delay.tv_sec++;
        }
        if (
            (last_delay.tv_sec > max_delay.tv_sec)
            || ((last_delay.tv_sec >= max_delay.tv_sec) && (last_delay.tv_usec > max_delay.tv_usec))
           ) {
            STATLOCK;
                max_delay.tv_sec =  last_delay.tv_sec;
                max_delay.tv_usec =  last_delay.tv_usec;
            STATUNLOCK;
        }
    }
    /* DS9097(1410)_send_and_get() call this function many times, therefore
     * I reset bus_write_time after every calls */
    gettimeofday( &(pn->in->bus_write_time) , NULL );
    return;
}

int FS_type(char *buf, size_t size, off_t offset , const struct parsedname * pn) {
    size_t len = strlen(pn->dev->name) ;
//printf("TYPE len=%d, size=%d\n",len,size);
    if ( offset ) return -EFAULT ;
    if ( len > size ) return -EMSGSIZE ;
    strcpy( buf , pn->dev->name ) ;
    return len ;
}

int FS_code(char *buf, size_t size, off_t offset , const struct parsedname * pn) {
    if ( offset ) return -EFAULT ;
    if ( size < 2 ) return -EMSGSIZE ;
    num2string( buf , pn->sn[0] ) ;
    return 2 ;
}

int FS_ID(char *buf, size_t size, off_t offset , const struct parsedname * pn) {
    size_t i ;
    size_t siz = size>>1 ;
    size_t off = offset>>1 ;
    for ( i= 0 ; i < siz ; ++i ) num2string( buf+2*i+offset, pn->sn[i+off+1] ) ;
    return size ;
}

int FS_r_ID(char *buf, size_t size, off_t offset , const struct parsedname * pn) {
    size_t i ;
    size_t siz = size>>1 ;
    size_t off = offset>>1 ;
    for ( i= 0 ; i < siz ; ++i ) num2string( buf+2*i+offset, pn->sn[6-i-off] ) ;
    return size ;
}

int FS_crc8(char *buf, size_t size, off_t offset , const struct parsedname * pn) {
    (void) size ;
    (void) offset ;
    num2string(buf,pn->sn[7]) ;
    return 2 ;
}

int FS_address(char *buf, size_t size, off_t offset , const struct parsedname * pn) {
    size_t i ;
    size_t siz = size>>1 ;
    size_t off = offset>>1 ;
    for ( i= 0 ; i < siz ; ++i ) num2string( buf+2*i+offset, pn->sn[i+off] ) ;
    return size ;
}

int FS_r_address(char *buf, size_t size, off_t offset , const struct parsedname * pn) {
    size_t i ;
    size_t siz = size>>1 ;
    size_t off = offset>>1 ;
    for ( i= 0 ; i < siz ; ++i ) num2string( buf+2*i+offset, pn->sn[7-i-off] ) ;
    return size ;
}
