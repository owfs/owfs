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
 aREAD_FUNCTION( FS_name ) ;
 aREAD_FUNCTION( FS_port ) ;
 aREAD_FUNCTION( FS_pidfile ) ;
 uREAD_FUNCTION( FS_uint ) ;
 uREAD_FUNCTION( FS_version ) ;
 aREAD_FUNCTION( FS_detail ) ;

/* -------- Structures ---------- */
/* Rare PUBLIC aggregate structure to allow changing the number of adapters */
struct aggregate Asystem = { 1, ag_numbers, ag_separate, } ;
struct filetype sys_adapter[] = {
    {"name"       ,       16, &Asystem, ft_ascii,   ft_static, {a:FS_name}   , {v:NULL}, NULL , } ,
    {"address"    ,      512, &Asystem, ft_ascii,   ft_static, {a:FS_port}   , {v:NULL}, NULL , } ,
    {"version"    ,       12, &Asystem, ft_unsigned,ft_static, {u:FS_version}, {v:NULL}, NULL , } ,
    {"detail"     ,       16, &Asystem, ft_ascii,   ft_static, {a:FS_detail} , {v:NULL}, NULL , } ,
} ;
struct device d_sys_adapter = { "adapter", "adapter", pn_system, NFT(sys_adapter), sys_adapter } ;

struct filetype sys_process[] = {
  //    {"pidfile"    ,-fl_pidfile, NULL    , ft_ascii,   ft_static, {a:FS_pidfile}, {v:NULL}, NULL , } ,
    {"pidfile"    ,      128, NULL    , ft_ascii,   ft_static, {a:FS_pidfile}, {v:NULL}, NULL , } ,
    {"pid"        ,       12, NULL    , ft_unsigned,ft_static, {u:FS_uint}   , {v:NULL}, &pid_num , } ,
} ;
struct device d_sys_process = { "process", "process", pn_system, NFT(sys_process), sys_process } ;

/* special entry -- picked off by parsing before filetypes tried */
struct filetype sys_structure[] = {
    {"indevices"  ,       12, NULL    , ft_unsigned,ft_static, {u:FS_uint}   , {v:NULL}, &indevices , } ,
    {"outdevices" ,       12, NULL    , ft_unsigned,ft_static, {u:FS_uint}   , {v:NULL}, &outdevices , } ,
} ;
struct device d_sys_structure = { "structure", "structure", pn_system, NFT(sys_structure), sys_structure } ;

/* ------- Functions ------------ */

/* special check, -remote file length won't match local sizes */
static int FS_name(char *buf, const size_t size, const off_t offset , const struct parsedname * pn) {
    int dindex = pn->extension ;
    struct connection_in * in;

    if (dindex<0) dindex = 0 ;
    in = find_connection_in(dindex);
    if(!in) return -ENOENT ;
    
    if ( in->adapter_name == NULL ) return -EINVAL ;
    strncpy(buf,&(in->adapter_name[offset]),size);
    return buf[size-1]?size:strlen(buf) ;
}

/* special check, -remote file length won't match local sizes */
static int FS_port(char *buf, const size_t size, const off_t offset , const struct parsedname * pn) {
    int dindex = pn->extension ;
    struct connection_in * in;

    if (dindex<0) dindex = 0 ;
    in = find_connection_in(dindex);
    if(!in) return -ENOENT ;
    
    strncpy(buf,&(in->name[offset]),size);
    return buf[size-1]?size:strlen(buf) ;
}

/* special check, -remote file length won't match local sizes */
static int FS_version(unsigned int * u, const struct parsedname * pn) {
    int dindex = pn->extension ;
    struct connection_in * in;

    if (dindex<0) dindex = 0 ;
    in = find_connection_in(dindex);
    if(!in) return -ENOENT ;
    
    u[0] = in->Adapter ;
    return 0 ;
}

static int FS_pidfile(char *buf, const size_t size, const off_t offset , const struct parsedname * pn) {
    (void) pn ;
    if( pid_file == NULL ) return -ENODEV ;
    strncpy( buf,&pid_file[offset],size ) ;
    return buf[size-1]?size:strlen(buf) ;
}

static int FS_uint(unsigned int * u, const struct parsedname * pn) {
    if(!pn->ft) return -ENODEV ;
    if(!pn->ft->data) return -ENODEV ;
    u[0] = ((unsigned int *) pn->ft->data)[0] ;
    //printf("FS_uint: pid=%ld nr=%d\n", pthread_self(), u[0]);
    return 0 ;
}

static int FS_detail(char *buf, const size_t size, const off_t offset , const struct parsedname * pn) {
    char tmp[16] = "(none)";
    int dindex = pn->extension ;
    struct connection_in * in;

    if (dindex<0) dindex = 0 ;
    in = find_connection_in(dindex);
    if(!in) return -ENOENT ;
    
    switch(in->Adapter) {
    case adapter_LINK:
    case adapter_LINK_Multi:
    {
        struct parsedname pn2 ;
	memcpy( &pn2, pn, sizeof(struct parsedname) ) ; /* shallow copy */
        pn2.in = in ; /* so correct bus is querried */
        if ( LI_reset(&pn2) || BUS_write(" ",1,&pn2) ) {
            return -ENODEV ;
        } else {
             memset(tmp,0,size) ;
             BUS_read(tmp,size,&pn2) ; // ignore return value -- will time out, probably
             COM_flush(&pn2) ;
	     strncpy(buf,&tmp[offset],size) ;
        }
    }
    default:
        if ( offset>strlen(tmp) ) return 0 ;
	strncpy(buf,&tmp[offset],size) ;
    }
    return buf[size-1]?size:strlen(buf) ;
}
