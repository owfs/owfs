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
//int FS_port(char *buf, const size_t size, const off_t offset , const struct parsedname * pn) ;
//int FS_version(char *buf, const size_t size, const off_t offset , const struct parsedname * pn) ;
//int FS_detail(char *buf, const size_t size, const off_t offset , const struct parsedname * pn) ;

/* ------- Structures ----------- */

struct filetype DS9097[] = {
    F_present ,
    {"port"      , PATH_MAX,NULL,ft_ascii  , ft_static  , {a:FS_port}       , {v:NULL}, NULL, } ,
    {"version"   ,    32,  NULL, ft_integer, ft_static  , {i:FS_version}    , {v:NULL}, NULL, } ,
} ;
struct device d_DS9097 = { "DS9097", "DS9097", dev_interface, NFT(DS9097), DS9097 } ;

struct filetype DS1410[] = {
    F_present ,
    {"port"      , PATH_MAX,NULL,ft_ascii  , ft_static  , {a:FS_port}       , {v:NULL}, NULL, } ,
} ;
struct device d_DS1410 = { "DS1410", "DS1410", dev_interface, NFT(DS1410), DS1410 } ;

struct filetype DS9097U[] = {
    F_present ,
    {"port"      , PATH_MAX,NULL,ft_ascii  , ft_static  , {a:FS_port}       , {v:NULL}, NULL, } ,
    {"version"   ,    32,  NULL, ft_integer, ft_static  , {i:FS_version}    , {v:NULL}, NULL, } ,
} ;
struct device d_DS9097U = { "DS9097U", "DS9097U", dev_interface, NFT(DS9097U), DS9097U } ;

struct filetype iButtonLink[] = {
    F_present ,
    {"detail"    ,   128,  NULL, ft_ascii  , ft_stable  , {a:FS_detail}     , {v:NULL}, NULL, } ,
    {"port"      , PATH_MAX,NULL,ft_ascii  , ft_static  , {a:FS_port}       , {v:NULL}, NULL, } ,
    {"version"   ,    32,  NULL, ft_integer, ft_static  , {i:FS_version}    , {v:NULL}, NULL, } ,
} ;
struct device d_iButtonLink = { "LINK_Multiport", "iButtonLink_Multiport", dev_interface, NFT(iButtonLink), iButtonLink } ;
struct device d_iButtonLink_Multiport = { "LINK", "iButtonLink", dev_interface, NFT(iButtonLink), iButtonLink } ;

struct filetype DS9490[] = {
    F_present ,
    {"port"      , PATH_MAX,NULL,ft_ascii  , ft_static  , {a:FS_port}       , {v:NULL}, NULL, } ,
} ;
struct device d_DS9490 = { "DS9490", "DS9490", dev_interface, NFT(DS9490), DS9490 } ;

/* ------- Functions ------------ */

static int FS_version(int * v , const struct parsedname * pn) {
    /* Unused */
    (void) pn ;
	*v = Version2480 ;
	return 0 ;
}

static int FS_detail(char *buf, const size_t size, const off_t offset , const struct parsedname * pn) {
    /* Unused */
    (void) pn ;

    if ( LI_reset() || BUS_write(" ",1) ) {
        return -ENODEV ;
    } else {
         char v[127] ;
         BUS_read(v,24) ; // ignore return value
         COM_flush() ;
         return FS_read_return( buf, size, offset , v , strlen(v) ) ;
    }
}

static int FS_port(char *buf, const size_t size, const off_t offset , const struct parsedname * pn) {
    /* Unused */
    (void) pn ;

    return FS_read_return( buf, size, offset, devport, strlen(devport) ) ;
}
