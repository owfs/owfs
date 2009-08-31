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
#include "ow_settings.h"

/* ------- Prototypes ----------- */
/* Statistics reporting */
READ_FUNCTION(FS_r_timeout);
WRITE_FUNCTION(FS_w_timeout);
READ_FUNCTION(FS_r_TS);
WRITE_FUNCTION(FS_w_TS);

/* -------- Structures ---------- */

struct filetype set_cache[] = {
	{"volatile", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_static, FS_r_timeout, FS_w_timeout, {v:&Globals.timeout_volatile},},
	{"stable", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_static, FS_r_timeout, FS_w_timeout, {v:&Globals.timeout_stable},},
	{"directory", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_static, FS_r_timeout, FS_w_timeout, {v:&Globals.timeout_directory},},
	{"presence", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_static, FS_r_timeout, FS_w_timeout, {v:&Globals.timeout_presence},},
	{"serial", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_static, FS_r_timeout, FS_w_timeout, {v:&Globals.timeout_serial},},
	{"usb", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_static, FS_r_timeout, FS_w_timeout, {v:&Globals.timeout_usb},},
	{"network", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_static, FS_r_timeout, FS_w_timeout, {v:&Globals.timeout_network},},
	{"server", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_static, FS_r_timeout, FS_w_timeout, {v:&Globals.timeout_server},},
	{"ftp", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_static, FS_r_timeout, FS_w_timeout, {v:&Globals.timeout_ftp},},
	{"ha7", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_static, FS_r_timeout, FS_w_timeout, {v:&Globals.timeout_ha7},},
	{"w1", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_static, FS_r_timeout, FS_w_timeout, {v:&Globals.timeout_w1},},
}

;
struct device d_set_cache = { "timeout", "timeout", ePN_settings, COUNT_OF_FILETYPES(set_cache),
	set_cache
};
struct filetype set_units[] = {
  {"temperature_scale", 1, NULL, ft_ascii, fc_static, FS_r_TS, FS_w_TS, NO_FILETYPE_DATA,},
}

;
struct device d_set_units = { "units", "units", ePN_settings, COUNT_OF_FILETYPES(set_units),
	set_units
};

/* ------- Functions ------------ */

static int FS_r_timeout(struct one_wire_query *owq)
{
	CACHE_RLOCK;
	OWQ_I(owq) = ((UINT *) OWQ_pn(owq).selected_filetype->data.v)[0];
	CACHE_RUNLOCK;
	return 0;
}

static int FS_w_timeout(struct one_wire_query *owq)
{
	int previous;
	struct parsedname *pn = PN(owq);
	CACHE_WLOCK;
	//printf("FS_w_timeout!!!\n");
	previous = ((UINT *) pn->selected_filetype->data.v)[0];
	((UINT *) pn->selected_filetype->data.v)[0] = OWQ_I(owq);
	CACHE_WUNLOCK;
	if (previous > OWQ_I(owq)) {
		Cache_Clear();
	}
	return 0;
}

static int FS_w_TS(struct one_wire_query *owq)
{
	int ret = 0;
	if (OWQ_size(owq) == 0 || OWQ_offset(owq) > 0) {
		return 0;				/* do nothing */
	}

	CONTROLFLAGSLOCK;
	switch (OWQ_buffer(owq)[0]) {
	case 'C':
	case 'c':
		set_controlflags(&LocalControlFlags, TEMPSCALE_MASK, TEMPSCALE_BIT, temp_celsius);
		break;
	case 'F':
	case 'f':
		set_controlflags(&LocalControlFlags, TEMPSCALE_MASK, TEMPSCALE_BIT, temp_fahrenheit);
		break;
	case 'R':
	case 'r':
		set_controlflags(&LocalControlFlags, TEMPSCALE_MASK, TEMPSCALE_BIT, temp_rankine);
		break;
	case 'K':
	case 'k':
		set_controlflags(&LocalControlFlags, TEMPSCALE_MASK, TEMPSCALE_BIT, temp_kelvin);
		break;
	default:
		ret = -EINVAL;
	}
	CONTROLFLAGSUNLOCK;
	return ret;
}

static int FS_r_TS(struct one_wire_query *owq)
{
	Fowq_output_offset_and_size_z(TemperatureScaleName(TemperatureScale(PN(owq))), owq);
	return 0;
}
