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
#include    <sys/time.h> /* for gettimeofday */


/* Not currently implemented -- cannot do simultaneous without a cache */
/* Some day will be able to store one path */
#ifndef OW_CACHE
    struct buspath simulpath[2] ;
    int simulpathlength ;
    time_t simulexpire ;
    #define SIMULTANEOUS_TIME 10
#endif /* OW_CACHE */


/* ------- Prototypes ----------- */
/* Statistics reporting */
 yREAD_FUNCTION( FS_r_convert ) ;
yWRITE_FUNCTION( FS_w_convert ) ;

/* -------- Structures ---------- */
struct filetype simultaneous[] = {
    {"temperature"     ,  1, NULL  , ft_yesno, ft_volatile, {y:FS_r_convert}, {y:FS_w_convert}, {i: simul_temp} , } ,
    {"voltage"         ,  1, NULL  , ft_yesno, ft_volatile, {y:FS_r_convert}, {y:FS_w_convert}, {i: simul_volt} , } ,
} ;
DeviceEntry( simultaneous, simultaneous ) ;

/* ------- Functions ------------ */
static int OW_skiprom( enum simul_type type, const struct parsedname * pn );
static int OW_setcache( enum simul_type type, const struct parsedname * pn ) ;
static int OW_getcache( enum simul_type type, const UINT msec, const struct parsedname * pn ) ;
static int OW_killcache( enum simul_type type, const struct parsedname * pn ) ;

struct internal_prop ipSimul[] = {
    {"temperature",ft_volatile},
    {"voltage",ft_volatile},
    };

/* returns 0 if valid conversion exists */
int Simul_Test( const enum simul_type type, UINT msec, const struct parsedname * pn ) {
    return OW_getcache(type,msec,pn) ;
}

int Simul_Clear( const enum simul_type type, const struct parsedname * pn ) {
    return OW_killcache(type,pn) ;
}

static int FS_w_convert(const int * y , const struct parsedname * pn) {
    if ( y[0]==0 ) {
        if ( OW_killcache((enum simul_type) pn->ft->data.i,pn) ) return -EINVAL ;
        return 0 ;
    }
    /* Since writing to /simultaneous/temperature is done recursive to all
     * adapters, we have to fake a successful write even if it's detected
     * as a bad adapter. */
    if(pn->in->Adapter == adapter_Bad) return 0;

    if ( OW_skiprom(  (enum simul_type) pn->ft->data.i, pn ) ) return -EINVAL ;
    if ( OW_setcache( (enum simul_type) pn->ft->data.i, pn ) ) return -EINVAL ;
    return 0 ;
}

static int FS_r_convert(int * y , const struct parsedname * pn) {
    y[0] = 1 ;
    if ( OW_getcache((enum simul_type) pn->ft->data.i,0,pn) ) y[0] = 0 ;
    return 0 ;
}

static int OW_setcache( enum simul_type type, const struct parsedname * pn ) {
    struct parsedname pn2 ;
    struct timeval tv ;
    memcpy( &pn2, pn , sizeof(struct parsedname)) ; // shallow copy
    FS_LoadPath(pn2.sn,&pn2) ;
    gettimeofday(&tv, NULL) ;
    return Cache_Add_Internal(&tv,sizeof(struct timeval),&ipSimul[type],&pn2) ;
}

static int OW_killcache( enum simul_type type, const struct parsedname * pn ) {
    struct parsedname pn2 ;
    memcpy( &pn2, pn , sizeof(struct parsedname)) ; // shallow copy
    FS_LoadPath(pn2.sn,&pn2) ;
    return Cache_Del_Internal(&ipSimul[type],&pn2) ;
}

static int OW_getcache( enum simul_type type ,const UINT msec, const struct parsedname * pn ) {
    struct parsedname pn2 ;
    struct timeval tv,now ;
    long int diff ;
    size_t dsize ;
    int ret ;
    memcpy( &pn2, pn , sizeof(struct parsedname)) ; // shallow copy
    FS_LoadPath(pn2.sn,&pn2) ;
    if ( (ret=Cache_Get_Internal(&tv, &dsize, &ipSimul[type],&pn2)) ) return ret ;
    gettimeofday(&now, NULL) ;
    diff =  1000*(now.tv_sec-tv.tv_sec) + (now.tv_usec-tv.tv_usec)/1000 ;
    if ( diff<(long int)msec ) UT_delay(msec-diff) ;
    return 0 ;
}

static int OW_skiprom( enum simul_type type, const struct parsedname * pn ) {
    const BYTE cmd_temp[] = { 0xCC, 0x44 } ;
    const BYTE cmd_volt[] = { 0xCC, 0x3C, 0x0F, 0x00, 0xFF, 0xFF } ;
    BYTE data[6];
    int ret = 0;
    struct parsedname pn2 ;
    memcpy( &pn2, pn, sizeof(struct parsedname)) ; /* shallow copy */
    pn2.dev = NULL ; /* only branch select done, not actual device */
    /* Make sure pn has bus-mutex initiated and it on local bus, NOT remote */
    BUSLOCK(pn);
	switch ( type ) {
	case simul_temp:
            ret = BUS_select(&pn2) || BUS_send_data( cmd_temp,2,pn ) ;
	    break ;
	case simul_volt:
            ret = BUS_select(&pn2) || BUS_sendback_data( cmd_volt,data,6,pn ) || memcmp( cmd_volt , data , 4 ) || CRC16(&data[1],5) ;
	    break ;
	}
    BUSUNLOCK(pn);
    return ret ;
}

