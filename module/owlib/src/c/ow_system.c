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
    {"name"       , -fl_adap_name, NULL , ft_ascii,   ft_static, {a:FS_ascii} , {v:NULL}, (void *) 0 , } ,
    {"port"       , -fl_adap_port, NULL , ft_ascii,   ft_static, {a:FS_ascii} , {v:NULL}, (void *) 1 , } ,
    {"version"    ,            12, NULL , ft_unsigned,ft_static, {u:FS_uint}  , {v:NULL}, (void *) 0      , } ,
    {"detail"     ,            16, NULL , ft_ascii,   ft_static, {a:FS_detail}, {v:NULL},   NULL         , } ,
} ;
struct device d_sys_adapter = { "adapter", "adapter", pn_system, NFT(sys_adapter), sys_adapter } ;

struct filetype sys_process[] = {
    {"pidfile"    , -fl_pidfile  , NULL , ft_ascii,   ft_static, {a:FS_ascii} , {v:NULL}, (void *) 2      , } ,
    {"pid"        ,            12, NULL , ft_unsigned,ft_static, {u:FS_uint}  , {v:NULL}, (void *) 1      , } ,
} ;
struct device d_sys_process = { "process", "process", pn_system, NFT(sys_process), sys_process } ;

/* special entry -- picked off by parsing before filetypes tried */
struct filetype sys_structure[] = {
    {"indevices"  ,            12, NULL , ft_unsigned,ft_static, {u:FS_uint}  , {v:NULL}, (void *) 2      , } ,
    {"outdevices" ,            12, NULL , ft_unsigned,ft_static, {u:FS_uint}  , {v:NULL}, (void *) 3      , } ,
} ;
struct device d_sys_structure = { "structure", "structure", pn_system, NFT(sys_structure), sys_structure } ;

/* ------- Functions ------------ */

/* special check, -remote file length won't match local sizes */
static int FS_ascii(char *buf, const size_t size, const off_t offset , const struct parsedname * pn) {
    char * x[] = { pn->in->adapter_name, pn->in->name, pid_file, } ;
    char * c = x[(int)(pn->ft->data)] ;
    size_t s ;
    if( c == NULL ) return -ENODEV ;
#if 0
    // offset and size adjustment already handled in FS_parse_read... isn't it?
    s = strlen(c) ;
    if ( offset>s ) return -ERANGE ;
    s -= offset ;
    if ( s>size ) s = size ;
    strncpy( buf, c, s ) ;
#else
    memcpy( buf, &c[offset], size);
#endif
    return size ;
}

static int FS_uint(unsigned int * u, const struct parsedname * pn) {
    unsigned int x[] = { pn->in->Adapter, pid_num, indevices, outdevices, } ;
    u[0] = x[(int)(pn->ft->data)] ;
    return 0 ;
}

static int FS_detail(char *buf, const size_t size, const off_t offset , const struct parsedname * pn) {
    char tmp[16];
    switch(pn->in->Adapter) {
    case adapter_LINK:
    case adapter_LINK_Multi:
        if ( LI_reset(pn) || BUS_write(" ",1,pn) ) {
            return -ENODEV ;
#if 0
	    /* Why not handle offset when possible... */
        } else if (offset) {
            return -EADDRNOTAVAIL ;
#endif
        } else {
             memset(tmp,0,size) ;
             BUS_read(tmp,size,pn) ; // ignore return value -- will time out, probably
             COM_flush(pn) ;
	     memcpy(buf,&tmp[offset],size) ;
             return size ;
        }
    default:
        memset(tmp,0,size) ;
	strcpy(tmp, "(none)");
	memcpy(buf,&tmp[offset],size) ;
        return size;
    }
}
