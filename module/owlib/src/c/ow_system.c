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
READ_FUNCTION(FS_pidfile);
READ_FUNCTION(FS_pid);
READ_FUNCTION(FS_in);
READ_FUNCTION(FS_out);
READ_FUNCTION(FS_define);

/* -------- Structures ---------- */
/* special entry -- picked off by parsing before filetypes tried */
struct filetype sys_process[] = {
	{"pidfile", 128, NON_AGGREGATE, ft_vascii, fc_static, FS_pidfile, NO_WRITE_FUNCTION, NO_FILETYPE_DATA,},
	// variable length
	{"pid", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_static, FS_pid, NO_WRITE_FUNCTION, NO_FILETYPE_DATA,},
};
struct device d_sys_process = { "process", "process", ePN_system, COUNT_OF_FILETYPES(sys_process),
	sys_process
};

struct filetype sys_connections[] = {
	{"count_current_buses", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_static, FS_in, NO_WRITE_FUNCTION, NO_FILETYPE_DATA,},
	{"count_outbound_connections", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_static, FS_out, NO_WRITE_FUNCTION, NO_FILETYPE_DATA,},
};
struct device d_sys_connections = { "connections", "connections", ePN_system,
	COUNT_OF_FILETYPES(sys_connections),
	sys_connections
};

struct filetype sys_configure[] = {
	{"threaded", PROPERTY_LENGTH_INTEGER, NON_AGGREGATE, ft_integer, fc_static, FS_define, NO_WRITE_FUNCTION, {i:OW_MT},},
	{"tai8570", PROPERTY_LENGTH_INTEGER, NON_AGGREGATE, ft_integer, fc_static, FS_define, NO_WRITE_FUNCTION, {i:OW_TAI8570},},
	{"thermocouples", PROPERTY_LENGTH_INTEGER, NON_AGGREGATE, ft_integer, fc_static, FS_define, NO_WRITE_FUNCTION, {i:OW_THERMOCOUPLE},},
	{"parport", PROPERTY_LENGTH_INTEGER, NON_AGGREGATE, ft_integer, fc_static, FS_define, NO_WRITE_FUNCTION, {i:OW_PARPORT},},
	{"USB", PROPERTY_LENGTH_INTEGER, NON_AGGREGATE, ft_integer, fc_static, FS_define, NO_WRITE_FUNCTION, {i:OW_USB},},
	{"i2c", PROPERTY_LENGTH_INTEGER, NON_AGGREGATE, ft_integer, fc_static, FS_define, NO_WRITE_FUNCTION, {i:OW_I2C},},
	{"cache", PROPERTY_LENGTH_INTEGER, NON_AGGREGATE, ft_integer, fc_static, FS_define, NO_WRITE_FUNCTION, {i:OW_CACHE},},
	{"HA7Net", PROPERTY_LENGTH_INTEGER, NON_AGGREGATE, ft_integer, fc_static, FS_define, NO_WRITE_FUNCTION, {i:OW_HA7},},
	{"DebugInfo", PROPERTY_LENGTH_INTEGER, NON_AGGREGATE, ft_integer, fc_static, FS_define, NO_WRITE_FUNCTION, {i:OW_DEBUG},},
	{"zeroconf", PROPERTY_LENGTH_INTEGER, NON_AGGREGATE, ft_integer, fc_static, FS_define, NO_WRITE_FUNCTION, {i:1},},
};
struct device d_sys_configure = { "configuration", "configuration", ePN_system,
	COUNT_OF_FILETYPES(sys_configure),
	sys_configure
};

/* ------- Functions ------------ */

static int FS_pidfile(struct one_wire_query *owq)
{
	char *name = "";
	if (pid_file) {
		name = pid_file;
	}
	Fowq_output_offset_and_size_z(name, owq);
	return 0;
}

static int FS_pid(struct one_wire_query *owq)
{
	OWQ_U(owq) = getpid();
	return 0;
}

static int FS_in(struct one_wire_query *owq)
{
	OWQ_U(owq) = Inbound_Control.active;
	return 0;
}

static int FS_out(struct one_wire_query *owq)
{
	OWQ_U(owq) = Outbound_Control.active;
	return 0;
}

static int FS_define(struct one_wire_query *owq)
{
	OWQ_Y(owq) = OWQ_pn(owq).selected_filetype->data.i;
	return 0;
}
