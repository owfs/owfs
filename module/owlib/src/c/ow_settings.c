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

/* Stats are a pseudo-device -- they are a file-system entry and handled as such,
     but have a different caching type to distiguish their handling */

#include "owfs_config.h"
#include "ow_settings.h"

/* ------- Prototypes ----------- */
/* Statistics reporting */
 yREAD_FUNCTION( FS_r_enable ) ;
yWRITE_FUNCTION( FS_w_enable ) ;
 iREAD_FUNCTION( FS_r_timeout ) ;
iWRITE_FUNCTION( FS_w_timeout ) ;

/* -------- Structures ---------- */

struct filetype set_cache[] = {
    {"enabled"         ,  1, NULL  , ft_yesno,    ft_static, {y:FS_r_enable},  {y:FS_w_enable},    NULL          , } ,
    {"volatile"        , 15, NULL  , ft_unsigned, ft_static, {i:FS_r_timeout}, {i:FS_w_timeout}, & timeout.vol   , } ,
    {"stable"          , 15, NULL  , ft_unsigned, ft_static, {i:FS_r_timeout}, {i:FS_w_timeout}, & timeout.stable, } ,
    {"directory"       , 15, NULL  , ft_unsigned, ft_static, {i:FS_r_timeout}, {i:FS_w_timeout}, & timeout.dir   , } ,
}
 ;
struct device d_set_cache = { "cache", "cache", pn_settings, NFT(set_cache), set_cache } ;

/* ------- Functions ------------ */

static int FS_r_enable(int * y , const struct parsedname * pn) {
    CACHELOCK
        y[0] =  cacheenabled ;
    CACHEUNLOCK
    return 0 ;
}

static int FS_w_enable(const int * y , const struct parsedname * pn) {
    if ( cacheavailable==0 ) return -EINVAL ;
    CACHELOCK
        cacheenabled = y[0] ;
    CACHEUNLOCK
    if ( cacheenabled==0 ) Cache_Clear() ;
    return 0 ;
}

static int FS_r_timeout(int * i , const struct parsedname * pn) {
    CACHELOCK
        i[0] = ((unsigned int *)pn->ft->data)[0] ;
    CACHEUNLOCK
    return 0 ;
}

static int FS_w_timeout(const int * i , const struct parsedname * pn) {
    int previous ;
    CACHELOCK
        previous = ((unsigned int *)pn->ft->data)[0] ;
        ((unsigned int *)pn->ft->data)[0] = i[0] ;
    CACHEUNLOCK
    if ( previous > i[0] ) Cache_Clear() ;
    return 0 ;
}
