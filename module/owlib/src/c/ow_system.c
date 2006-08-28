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

#include <config.h>
#include "owfs_config.h"
#include "ow_system.h"
#include "ow_pid.h"
#include "ow_connection.h"

/* ------- Prototypes ----------- */
/* Statistics reporting */
 aREAD_FUNCTION( FS_name ) ;
 aREAD_FUNCTION( FS_port ) ;
 aREAD_FUNCTION( FS_pidfile ) ;
 uREAD_FUNCTION( FS_pid ) ;
 uREAD_FUNCTION( FS_in ) ;
 uREAD_FUNCTION( FS_out ) ;
 uREAD_FUNCTION( FS_version ) ;
 uREAD_FUNCTION( FS_r_overdrive ) ;
uWRITE_FUNCTION( FS_w_overdrive ) ;
 uREAD_FUNCTION( FS_r_ds2404_compliance ) ;
uWRITE_FUNCTION( FS_w_ds2404_compliance ) ;
 iREAD_FUNCTION( FS_define ) ;

static int FS_nullstring( char * buf ) ;

/* -------- Structures ---------- */
/* Rare PUBLIC aggregate structure to allow changing the number of adapters */
struct aggregate Asystem = { 1, ag_numbers, ag_separate, } ;
struct filetype sys_adapter[] = {
    {"name"       ,      128, &Asystem, ft_vascii,  fc_static, {a:FS_name}   , {v:NULL}, {v: NULL }, } , // variable length
    {"address"    ,      512, &Asystem, ft_vascii,  fc_static, {a:FS_port}   , {v:NULL}, {v: NULL }, } , // variable length
    {"ds2404_compliance",  1, &Asystem, ft_unsigned,fc_static, {u:FS_r_ds2404_compliance}   , {u:FS_w_ds2404_compliance}, {v: NULL }, } ,
    {"overdrive"  ,        1, &Asystem, ft_unsigned,fc_static, {u:FS_r_overdrive},{u:FS_w_overdrive}, {v: NULL }, } ,
    {"version"    ,       12, &Asystem, ft_unsigned,fc_static, {u:FS_version}, {v:NULL}, {v: NULL }, } ,
} ;
struct device d_sys_adapter = { "adapter", "adapter", pn_system, NFT(sys_adapter), sys_adapter } ;

/* special entry -- picked off by parsing before filetypes tried */
struct filetype sys_process[] = {
  //    {"pidfile"    ,-fl_pidfile, NULL    , ft_ascii,   fc_static, {a:FS_pidfile}, {v:NULL}, {v: NULL }, } ,
    {"pidfile"    ,      128, NULL    , ft_vascii,   fc_static, {a:FS_pidfile}, {v:NULL}, {v: NULL }, } , // variable length
    {"pid"        ,       12, NULL    , ft_unsigned,fc_static, {u:FS_pid}    , {v:NULL}, {v: NULL }, } ,
} ;
struct device d_sys_process = { "process", "process", pn_system, NFT(sys_process), sys_process } ;

struct filetype sys_connections[] = {
    {"indevices"  ,       12, NULL    , ft_unsigned,fc_static, {u:FS_in}    , {v:NULL}, {v: NULL } , } ,
    {"outdevices" ,       12, NULL    , ft_unsigned,fc_static, {u:FS_out}   , {v:NULL}, {v: NULL } , } ,
} ;
struct device d_sys_connections = { "connections", "connections", pn_system, NFT(sys_connections), sys_connections } ;

struct filetype sys_configure[] = {
    {"threaded"     ,    12, NULL    , ft_integer,fc_static, {i:FS_define}  , {v:NULL}, {i: OW_MT } , } ,
    {"tai8570"      ,    12, NULL    , ft_integer,fc_static, {i:FS_define}  , {v:NULL}, {i: OW_TAI8570 } , } ,
    {"thermocouples",    12, NULL    , ft_integer,fc_static, {i:FS_define}  , {v:NULL}, {i: OW_THERMOCOUPLE } , } ,
    {"parport"      ,    12, NULL    , ft_integer,fc_static, {i:FS_define}  , {v:NULL}, {i: OW_PARPORT } , } ,
    {"USB"          ,    12, NULL    , ft_integer,fc_static, {i:FS_define}  , {v:NULL}, {i: OW_USB } , } ,
    {"i2c"          ,    12, NULL    , ft_integer,fc_static, {i:FS_define}  , {v:NULL}, {i: OW_I2C } , } ,
    {"cache"        ,    12, NULL    , ft_integer,fc_static, {i:FS_define}  , {v:NULL}, {i: OW_CACHE } , } ,
    {"HA7Net"       ,    12, NULL    , ft_integer,fc_static, {i:FS_define}  , {v:NULL}, {i: OW_HA7 } , } ,
    {"DebugInfo"    ,    12, NULL    , ft_integer,fc_static, {i:FS_define}  , {v:NULL}, {i: OW_DEBUG } , } ,
    {"zeroconf"     ,    12, NULL    , ft_integer,fc_static, {i:FS_define}  , {v:NULL}, {i: OW_ZERO } , } ,
} ;
struct device d_sys_configure = { "configuration", "configuration", pn_system, NFT(sys_configure), sys_configure } ;

/* ------- Functions ------------ */


/* Just some tests to support change of extra delay */
static int FS_r_ds2404_compliance(UINT * u , const struct parsedname * pn) {
    int dindex = pn->extension ;
    struct connection_in * in;

    if (dindex<0) dindex = 0 ;
    in = find_connection_in(dindex);
    if(!in) return -ENOENT ;

    u[0] = in->ds2404_compliance ;
    return 0 ;
}

static int FS_w_ds2404_compliance(const UINT * u , const struct parsedname * pn) {
    int dindex = pn->extension ;
    struct connection_in * in;

    if (dindex<0) dindex = 0 ;
    in = find_connection_in(dindex);
    if(!in) return -ENOENT ;

    in->ds2404_compliance = (u[0] ? 1 : 0);
    return 0 ;
}

/* Just some tests to support overdrive */
static int FS_r_overdrive(UINT * u , const struct parsedname * pn) {
    int dindex = pn->extension ;
    struct connection_in * in;

    if (dindex<0) dindex = 0 ;
    in = find_connection_in(dindex);
    if(!in) return -ENOENT ;

    u[0] = in->use_overdrive_speed ;
    return 0 ;
}

static int FS_w_overdrive(const UINT * u , const struct parsedname * pn) {
    int dindex = pn->extension ;
    struct connection_in * in;

    if (dindex<0) dindex = 0 ;
    in = find_connection_in(dindex);
    if(!in) return -ENOENT ;

    switch(u[0]) {
    case 0:
      in->use_overdrive_speed = ONEWIREBUSSPEED_REGULAR ;
      break ;
    case 1:
      if(pn->in->Adapter != adapter_DS9490) return -ENOTSUP ;
      in->use_overdrive_speed = ONEWIREBUSSPEED_FLEXIBLE ;
      break ;
    case 2:
      in->use_overdrive_speed = ONEWIREBUSSPEED_OVERDRIVE ;
      break ;
    default:
      return -ENOTSUP ;
    }
    return 0 ;
}

/* special check, -remote file length won't match local sizes */
static int FS_name(char *buf, const size_t size, const off_t offset , const struct parsedname * pn) {
    int dindex = pn->extension ;
    struct connection_in * in;

    if (dindex<0) dindex = 0 ;
    in = find_connection_in(dindex);
    if(!in) return -ENOENT ;
    
    if ( in->adapter_name == NULL ) return FS_nullstring(buf) ;
    return FS_output_ascii( buf, size, offset, in->adapter_name, strlen(in->adapter_name) ) ;
}

/* special check, -remote file length won't match local sizes */
static int FS_port(char *buf, const size_t size, const off_t offset , const struct parsedname * pn) {
    int dindex = pn->extension ;
    struct connection_in * in;

    if (dindex<0) dindex = 0 ;
    in = find_connection_in(dindex);
    if(!in) return -ENOENT ;
    
    if ( in->name == NULL ) return FS_nullstring(buf) ;
    return FS_output_ascii( buf, size, offset, in->name, strlen(in->name) ) ;
}

/* special check, -remote file length won't match local sizes */
static int FS_version(UINT * u, const struct parsedname * pn) {
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
    if( pid_file ) return FS_output_ascii( buf, size, offset, pid_file, strlen(pid_file) ) ;
    return FS_nullstring(buf) ;
}

static int FS_pid(UINT * u, const struct parsedname * pn) {
    (void) pn ;
    u[0] = getpid() ;
    return 0 ;
}

static int FS_in(UINT * u, const struct parsedname * pn) {
    (void) pn ;
    u[0] = indevices ;
    return 0 ;
}

static int FS_out(UINT * u, const struct parsedname * pn) {
    (void) pn ;
    u[0] = outdevices ;
    return 0 ;
}

static int FS_nullstring( char * buf ) {
    buf[0] = '\0' ;
    return 0 ;
}

static int FS_define( int * y, const struct parsedname * pn ) {
    y[0] = pn->ft->data.i ;
    return 0 ;
}
