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

/* Simultaneous is a trigger to do a mass conversion on all the devices in the specified path */

#include "owfs_config.h"
#include "ow_simultaneous.h"

#ifndef OW_CACHE
    struct buspath simulpath[2] ;
    int simulpathlength ;
    time_t simulexpire ;
#endif /* OW_CACHE */


/* ------- Prototypes ----------- */
/* Statistics reporting */
 yREAD_FUNCTION( FS_r_convert ) ;
yWRITE_FUNCTION( FS_w_convert ) ;

/* -------- Structures ---------- */
struct filetype simultaneous[] = {
    {"temperature"     ,  1, NULL  , ft_yesno, ft_volatile, {y:FS_r_convert}, {y:FS_w_convert}, (void *)simul_temp , } ,
    {"voltage"         ,  1, NULL  , ft_yesno, ft_volatile, {y:FS_r_convert}, {y:FS_w_convert}, (void *)simul_volt , } ,
} ;
DeviceEntry( simultaneous, simultaneous )

/* ------- Functions ------------ */
static int OW_skiprom( unsigned char cmd, const struct parsedname * const pn );
static int OW_setcache( const struct parsedname * const pn ) ;
static int OW_getcache( const unsigned int msec, const struct parsedname * const pn ) ;
static int OW_killcache( const struct parsedname * const pn ) ;

int Simul_Test( const enum simul_type type, unsigned int msec, const struct parsedname * pn ) ;
int Simul_Clear( const enum simul_type type, const struct parsedname * pn ) { return 0 ;}

int Simul_Test( const enum simul_type type, unsigned int msec, const struct parsedname * pn ) {
    return OW_getcache(msec,pn) ;
}


static int FS_w_convert(const int * y , const struct parsedname * pn) {
    int ret;
    if ( y[0]==0 ) {
        if ( OW_killcache(pn) ) return -EINVAL ;
        return 0 ;
    }
    switch( (enum simul_type) pn->ft->data ) {
    case simul_temp:
        ret = OW_skiprom(0x44,pn) ;
        break ;
    case simul_volt:
        ret = OW_skiprom(0x3C,pn) ;
        break ;
    }
    if ( ret || OW_setcache( pn ) ) return -EINVAL ;
    return 0 ;
}

static int FS_r_convert(int * y , const struct parsedname * pn) {
    y[0] = 1 ;
    if ( OW_getcache(0,pn) ) y[0] = 0 ;
    return 0 ;
}

static int OW_setcache( const struct parsedname * const pn ) {
    struct parsedname pn2 ;
    struct timeval tv ;
    memcpy( &pn2, pn , sizeof(struct parsedname)) ; // shallow copy
    FS_LoadPath(&pn2) ;
    gettimeofday(&tv,0) ;
    return Cache_Add(&tv,sizeof(struct timeval),&pn2) ;
}

static int OW_killcache( const struct parsedname * const pn ) {
    struct parsedname pn2 ;
    memcpy( &pn2, pn , sizeof(struct parsedname)) ; // shallow copy
    FS_LoadPath(&pn2) ;
    return Cache_Del(&pn2) ;
}

static int OW_getcache( const unsigned int msec, const struct parsedname * const pn ) {
    struct parsedname pn2 ;
    struct timeval tv,now ;
    long int diff ;
    int dsize ;
    int ret ;
    memcpy( &pn2, pn , sizeof(struct parsedname)) ; // shallow copy
    FS_LoadPath(&pn2) ;
    if ( (ret=Cache_Get(&tv, &dsize, &pn2)) ) return ret ;
    gettimeofday(&now,0) ;
    diff =  1000*(now.tv_sec-tv.tv_sec) + (now.tv_usec-tv.tv_usec)/1000 ;
    if ( diff<msec ) UT_delay(msec-diff) ;
    return 0 ;
}

static int OW_skiprom( unsigned char cmd, const struct parsedname * const pn ) {
    const unsigned char p[] = { 0xCC, cmd } ;
    int ret ;
    struct parsedname pn2 ;
    memcpy( &pn2, pn, sizeof(struct parsedname)) ; /* shallow copy */
    pn2.dev = NULL ; /* only branch select done, not actual device */
    BUSLOCK
        ret = BUS_select(&pn2) || BUS_send_data( p , 2 ) ;
    BUSUNLOCK
    return ret ;
}

