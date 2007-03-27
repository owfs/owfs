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
READ_FUNCTION(FS_name);
READ_FUNCTION(FS_port);
READ_FUNCTION(FS_pidfile);
READ_FUNCTION(FS_pid);
READ_FUNCTION(FS_in);
READ_FUNCTION(FS_out);
READ_FUNCTION(FS_version);
READ_FUNCTION(FS_r_overdrive);
WRITE_FUNCTION(FS_w_overdrive);
READ_FUNCTION(FS_r_ds2404_compliance);
WRITE_FUNCTION(FS_w_ds2404_compliance);
READ_FUNCTION(FS_define);

/* -------- Structures ---------- */
/* Rare PUBLIC aggregate structure to allow changing the number of adapters */
struct aggregate Asystem = { 1, ag_numbers, ag_separate, };
struct filetype sys_adapter[] = {
  {"name", 128, &Asystem, ft_vascii, fc_static, {o: FS_name}, {v: NULL}, {v:NULL},},
	// variable length
  {"address", 512, &Asystem, ft_vascii, fc_static, {o: FS_port}, {v: NULL}, {v:NULL},},
	// variable length
  {"ds2404_compliance",PROPERTY_LENGTH_YESNO, &Asystem, ft_yesno, fc_static, {o: FS_r_ds2404_compliance}, {o: FS_w_ds2404_compliance}, {v:NULL},},
  {"overdrive",PROPERTY_LENGTH_UNSIGNED, &Asystem, ft_unsigned, fc_static, {o: FS_r_overdrive}, {o: FS_w_overdrive}, {v:NULL},},
  {"version",PROPERTY_LENGTH_UNSIGNED, &Asystem, ft_unsigned, fc_static, {o: FS_version}, {v: NULL}, {v:NULL},},
};
struct device d_sys_adapter =
	{ "adapter", "adapter", pn_system, NFT(sys_adapter), sys_adapter };

/* special entry -- picked off by parsing before filetypes tried */
struct filetype sys_process[] = {
	//    {"pidfile"    ,-fl_pidfile, NULL    , ft_ascii,   fc_static, {a:FS_pidfile}, {v:NULL}, {v: NULL }, } ,
  {"pidfile", 128, NULL, ft_vascii, fc_static, {o: FS_pidfile}, {v: NULL}, {v:NULL},},
	// variable length
  {"pid",PROPERTY_LENGTH_UNSIGNED, NULL, ft_unsigned, fc_static, {o: FS_pid}, {v: NULL}, {v:NULL},},
};
struct device d_sys_process =
	{ "process", "process", pn_system, NFT(sys_process), sys_process };

struct filetype sys_connections[] = {
  {"indevices",PROPERTY_LENGTH_UNSIGNED, NULL, ft_unsigned, fc_static, {o: FS_in}, {v: NULL}, {v:NULL},},
  {"outdevices",PROPERTY_LENGTH_UNSIGNED, NULL, ft_unsigned, fc_static, {o: FS_out}, {v: NULL}, {v:NULL},},
};
struct device d_sys_connections =
	{ "connections", "connections", pn_system, NFT(sys_connections),
	sys_connections
};

struct filetype sys_configure[] = {
  {"threaded",PROPERTY_LENGTH_INTEGER, NULL, ft_integer, fc_static, {o: FS_define}, {v: NULL}, {i:OW_MT},},
  {"tai8570",PROPERTY_LENGTH_INTEGER, NULL, ft_integer, fc_static, {o: FS_define}, {v: NULL}, {i:OW_TAI8570},},
  {"thermocouples",PROPERTY_LENGTH_INTEGER, NULL, ft_integer, fc_static, {o: FS_define}, {v: NULL}, {i:OW_THERMOCOUPLE},},
  {"parport",PROPERTY_LENGTH_INTEGER, NULL, ft_integer, fc_static, {o: FS_define}, {v: NULL}, {i:OW_PARPORT},},
  {"USB",PROPERTY_LENGTH_INTEGER, NULL, ft_integer, fc_static, {o: FS_define}, {v: NULL}, {i:OW_USB},},
  {"i2c",PROPERTY_LENGTH_INTEGER, NULL, ft_integer, fc_static, {o: FS_define}, {v: NULL}, {i:OW_I2C},},
  {"cache",PROPERTY_LENGTH_INTEGER, NULL, ft_integer, fc_static, {o: FS_define}, {v: NULL}, {i:OW_CACHE},},
  {"HA7Net",PROPERTY_LENGTH_INTEGER, NULL, ft_integer, fc_static, {o: FS_define}, {v: NULL}, {i:OW_HA7},},
  {"DebugInfo",PROPERTY_LENGTH_INTEGER, NULL, ft_integer, fc_static, {o: FS_define}, {v: NULL}, {i:OW_DEBUG},},
  {"zeroconf",PROPERTY_LENGTH_INTEGER, NULL, ft_integer, fc_static, {o: FS_define}, {v: NULL}, {i:1},},
};
struct device d_sys_configure =
	{ "configuration", "configuration", pn_system, NFT(sys_configure),
	sys_configure
};

/* ------- Functions ------------ */


/* Just some tests to support change of extra delay */
static int FS_r_ds2404_compliance(struct one_wire_query * owq)
{
    struct parsedname * pn = PN(owq) ;
    int dindex = pn->extension;
	struct connection_in *in;

	if (dindex < 0)
		dindex = 0;
	in = find_connection_in(dindex);
	if (!in)
		return -ENOENT;

    OWQ_Y(owq) = in->ds2404_compliance;
	return 0;
}

static int FS_w_ds2404_compliance(struct one_wire_query * owq)
{
    struct parsedname * pn = PN(owq) ;
    int dindex = pn->extension;
	struct connection_in *in;

	if (dindex < 0)
		dindex = 0;
	in = find_connection_in(dindex);
	if (!in)
		return -ENOENT;

    in->ds2404_compliance = OWQ_Y(owq);
	return 0;
}

/* Just some tests to support overdrive */
static int FS_r_overdrive(struct one_wire_query * owq)
{
    struct parsedname * pn = PN(owq) ;
    int dindex = pn->extension;
	struct connection_in *in;

	if (dindex < 0)
		dindex = 0;
	in = find_connection_in(dindex);
	if (!in)
		return -ENOENT;

    OWQ_U(owq) = in->use_overdrive_speed;
	return 0;
}

static int FS_w_overdrive(struct one_wire_query * owq)
{
    struct parsedname * pn = PN(owq) ;
    int dindex = pn->extension;
	struct connection_in *in;

	if (dindex < 0)
		dindex = 0;
	in = find_connection_in(dindex);
	if (!in)
		return -ENOENT;

    switch (OWQ_U(owq)) {
	case 0:
		in->use_overdrive_speed = ONEWIREBUSSPEED_REGULAR;
		break;
	case 1:
		if (pn->in->Adapter != adapter_DS9490)
			return -ENOTSUP;
		in->use_overdrive_speed = ONEWIREBUSSPEED_FLEXIBLE;
		break;
	case 2:
		in->use_overdrive_speed = ONEWIREBUSSPEED_OVERDRIVE;
		break;
	default:
		return -ENOTSUP;
	}
	return 0;
}

/* special check, -remote file length won't match local sizes */
static int FS_name(struct one_wire_query * owq)
{
    struct parsedname * pn = PN(owq) ;
    int dindex = pn->extension;
	struct connection_in *in;
    char * name = "" ;

	if (dindex < 0)
		dindex = 0;
	in = find_connection_in(dindex);
	if (!in)
		return -ENOENT;

	if (in->adapter_name )
        name = in->adapter_name ;
    Fowq_output_offset_and_size_z( name, owq ) ;
	return 0 ;
}

/* special check, -remote file length won't match local sizes */
static int FS_port(struct one_wire_query * owq)
{
    struct parsedname * pn = PN(owq) ;
    int dindex = pn->extension;
	struct connection_in *in;
    char * name = "" ;

	if (dindex < 0)
		dindex = 0;
	in = find_connection_in(dindex);
	if (!in)
		return -ENOENT;

    if (in->name) name = in->name ;
    Fowq_output_offset_and_size_z( name, owq ) ;
    return 0 ;
}

/* special check, -remote file length won't match local sizes */
static int FS_version(struct one_wire_query * owq)
{
    struct parsedname * pn = PN(owq) ;
    int dindex = pn->extension;
	struct connection_in *in;

	if (dindex < 0)
		dindex = 0;
	in = find_connection_in(dindex);
	if (!in)
		return -ENOENT;

    OWQ_U(owq) = in->Adapter;
	return 0;
}

static int FS_pidfile(struct one_wire_query * owq)
{
    char * name = "" ;
    if (pid_file) name = pid_file ;
    Fowq_output_offset_and_size_z( name, owq ) ;
    return 0 ;
}

static int FS_pid(struct one_wire_query * owq)
{
    OWQ_U(owq) = getpid();
	return 0;
}

static int FS_in(struct one_wire_query * owq)
{
	CONNINLOCK;
    OWQ_U(owq) = indevices;
	CONNINUNLOCK;
	return 0;
}

static int FS_out(struct one_wire_query * owq)
{
    OWQ_U(owq) = outdevices;
	return 0;
}

static int FS_define(struct one_wire_query * owq)
{
    OWQ_Y(owq) = OWQ_pn(owq).ft->data.i;
	return 0;
}
