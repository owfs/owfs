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
#include "ow_link.h"

/* ------- Prototypes ----------- */

 aREAD_FUNCTION( FS_port) ;
 aREAD_FUNCTION( FS_detail) ;
 iREAD_FUNCTION( FS_version) ;
 yREAD_FUNCTION( FS_apresent) ;
//int FS_port(char *buf, const size_t size, const off_t offset , const struct parsedname * pn) ;
//int FS_version(char *buf, const size_t size, const off_t offset , const struct parsedname * pn) ;
//int FS_detail(char *buf, const size_t size, const off_t offset , const struct parsedname * pn) ;

/* ------- Structures ----------- */

struct filetype DS9097[] = {
    {"present"   ,     1,  NULL, ft_yesno  , ft_static  , {y:FS_apresent}    , {v:NULL}, NULL, } ,
    {"port"      , PATH_MAX,NULL,ft_ascii  , ft_static  , {a:FS_port}       , {v:NULL}, NULL, } ,
    {"version"   ,    32,  NULL, ft_integer, ft_static  , {i:FS_version}    , {v:NULL}, NULL, } ,
} ;
struct device d_DS9097 = { "DS9097", "DS9097", dev_interface, NFT(DS9097), DS9097 } ;

struct filetype DS1410[] = {
    {"present"   ,     1,  NULL, ft_yesno  , ft_static  , {y:FS_apresent}    , {v:NULL}, NULL, } ,
    {"port"      , PATH_MAX,NULL,ft_ascii  , ft_static  , {a:FS_port}       , {v:NULL}, NULL, } ,
} ;
struct device d_DS1410 = { "DS1410", "DS1410", dev_interface, NFT(DS1410), DS1410 } ;

struct filetype DS9097U[] = {
    {"present"   ,     1,  NULL, ft_yesno  , ft_static  , {y:FS_apresent}    , {v:NULL}, NULL, } ,
    {"port"      , PATH_MAX,NULL,ft_ascii  , ft_static  , {a:FS_port}       , {v:NULL}, NULL, } ,
    {"version"   ,    32,  NULL, ft_integer, ft_static  , {i:FS_version}    , {v:NULL}, NULL, } ,
} ;
struct device d_DS9097U = { "DS9097U", "DS9097U", dev_interface, NFT(DS9097U), DS9097U } ;

struct filetype iButtonLink[] = {
    {"present"   ,     1,  NULL, ft_yesno  , ft_static  , {y:FS_apresent}    , {v:NULL}, NULL, } ,
    {"detail"    ,   128,  NULL, ft_ascii  , ft_stable  , {a:FS_detail}     , {v:NULL}, NULL, } ,
    {"port"      , PATH_MAX,NULL,ft_ascii  , ft_static  , {a:FS_port}       , {v:NULL}, NULL, } ,
    {"version"   ,    32,  NULL, ft_integer, ft_static  , {i:FS_version}    , {v:NULL}, NULL, } ,
} ;
struct device d_iButtonLink = { "LINK_Multiport", "iButtonLink_Multiport", dev_interface, NFT(iButtonLink), iButtonLink } ;
struct device d_iButtonLink_Multiport = { "LINK", "iButtonLink", dev_interface, NFT(iButtonLink), iButtonLink } ;

struct filetype DS9490[] = {
    {"present"   ,     1,  NULL, ft_yesno  , ft_static  , {y:FS_apresent}    , {v:NULL}, NULL, } ,
    {"port"      , PATH_MAX,NULL,ft_ascii  , ft_static  , {a:FS_port}       , {v:NULL}, NULL, } ,
} ;
struct device d_DS9490 = { "DS9490", "DS9490", dev_interface, NFT(DS9490), DS9490 } ;

static int CheckAdapterPresence( const struct parsedname * const pn ) ;

/* ------- Functions ------------ */

static int FS_version(int * v , const struct parsedname * pn) {
    (void) pn ;
    *v = Adapter ;
    return 0 ;
}

static int FS_detail(char *buf, const size_t size, const off_t offset , const struct parsedname * pn) {
    (void) pn ;

    if ( LI_reset(pn) || BUS_write(" ",1) ) {
        return -ENODEV ;
    } else if (offset) {
        return -EADDRNOTAVAIL ;
    } else {
         memset(buf,0,size) ;
         BUS_read(buf,size) ; // ignore return value -- will time out, probably
         COM_flush() ;
         return size ;
    }
}

static int FS_port(char *buf, const size_t size, const off_t offset , const struct parsedname * pn) {
    (void) pn ;
    if ( offset> strlen(devport) ) return -EMSGSIZE ;
    strncpy(buf,&devport[offset],size) ;
    return size ;
}

/* Check if device exists -- 0 yes, 1 no */
static int CheckAdapterPresence( const struct parsedname * const pn ) {
    switch ( Adapter ) {
    case adapter_DS9097:
        return strcmp( "DS9097", pn->dev->code ) ;
    case adapter_DS1410:
        return strcmp( "DS1410", pn->dev->code ) ;
    case adapter_DS9097U:
        return strcmp( "DS9097U", pn->dev->code ) ;
    case adapter_LINK_Multi:
        return strcmp( "LINK_Multiport", pn->dev->code ) ;
    case adapter_LINK:
        return strcmp( "LINK", pn->dev->code ) ;
    case adapter_DS9490:
        return strcmp( "DS9490", pn->dev->code ) ;
    }
}

static int FS_apresent( int * y , const struct parsedname * pn ) {
    y[0] = ! CheckAdapterPresence(pn) ;
    return 0 ;
}

																							