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
#include "ow_system.h"

/* ------- Prototypes ----------- */
/* Statistics reporting */
 aREAD_FUNCTION( FS_ascii ) ;
 uREAD_FUNCTION( FS_uint ) ;
 aREAD_FUNCTION( FS_detail ) ;

/* -------- Structures ---------- */
struct filetype sys_adapter[] = {
    {"name"       , -fl_adap_name, NULL , ft_ascii,   ft_static, {a:FS_ascii} , {v:NULL}, & adapter_name , } ,
    {"port"       , -fl_adap_port, NULL , ft_ascii,   ft_static, {a:FS_ascii} , {v:NULL}, & devport      , } ,
    {"version"    ,            12, NULL , ft_unsigned,ft_static, {u:FS_uint}  , {v:NULL}, & Adapter      , } ,
    {"detail"     ,            16, NULL , ft_ascii,   ft_static, {a:FS_detail}, {v:NULL},   NULL         , } ,
} ;
struct device d_sys_adapter = { "adapter", "adapter", pn_system, NFT(sys_adapter), sys_adapter } ;

struct filetype sys_process[] = {
    {"pidfile"    , -fl_pidfile  , NULL , ft_ascii,   ft_static, {a:FS_ascii} , {v:NULL}, & pid_file     , } ,
    {"pid"        ,            12, NULL , ft_unsigned,ft_static, {u:FS_uint}  , {v:NULL}, & pid_num      , } ,
} ;
struct device d_sys_process = { "process", "process", pn_system, NFT(sys_process), sys_process } ;

/* special entry -- picked off by parsing before filetypes tried */
struct device d_sys_structure = { "structure", "structure", pn_system, 0, NULL } ;

/* ------- Functions ------------ */

static int FS_ascii(char *buf, const size_t size, const off_t offset , const struct parsedname * pn) {
    if( (char**)(pn->ft->data) == NULL ) return -ENODEV ;
    strncpy( buf, ((char **)(pn->ft->data))[offset], size ) ;
    return size ;
}

static int FS_uint(unsigned int * u, const struct parsedname * pn) {
    u[0] = (((unsigned int *)pn->ft->data))[0] ;
    return 0 ;
}

static int FS_detail(char *buf, const size_t size, const off_t offset , const struct parsedname * pn) {
    switch(Adapter) {
    case adapter_LINK:
    case adapter_LINK_Multi:
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
    default:
        return 0 ;
    }
}


