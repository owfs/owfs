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

/* Settings are a pseudo-device -- used to dynamicalls change timeouts and the like
 * these settings can also be changed at the command line
 * Except for performance, none of these settings has security implications
 * */

#include <config.h>
#include "owfs_config.h"
#include "ow_settings.h"

/* ------- Prototypes ----------- */
/* Statistics reporting */
READ_FUNCTION(FS_r_timeout);
WRITE_FUNCTION(FS_w_timeout);
READ_FUNCTION(FS_r_yesno);
WRITE_FUNCTION(FS_w_yesno);
READ_FUNCTION(FS_r_TS);
WRITE_FUNCTION(FS_w_TS);
READ_FUNCTION(FS_r_PS);
WRITE_FUNCTION(FS_w_PS);
READ_FUNCTION(FS_aliaslist);

/* -------- Structures ---------- */

struct filetype set_timeout[] = {
	{"volatile", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_static, FS_r_timeout, FS_w_timeout, VISIBLE, {v:&Globals.timeout_volatile},},
	{"stable", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_static, FS_r_timeout, FS_w_timeout, VISIBLE, {v:&Globals.timeout_stable},},
	{"directory", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_static, FS_r_timeout, FS_w_timeout, VISIBLE, {v:&Globals.timeout_directory},},
	{"presence", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_static, FS_r_timeout, FS_w_timeout, VISIBLE, {v:&Globals.timeout_presence},},
	{"serial", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_static, FS_r_timeout, FS_w_timeout, VISIBLE, {v:&Globals.timeout_serial},},
	{"usb", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_static, FS_r_timeout, FS_w_timeout, VISIBLE, {v:&Globals.timeout_usb},},
	{"network", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_static, FS_r_timeout, FS_w_timeout, VISIBLE, {v:&Globals.timeout_network},},
	{"server", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_static, FS_r_timeout, FS_w_timeout, VISIBLE, {v:&Globals.timeout_server},},
	{"ftp", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_static, FS_r_timeout, FS_w_timeout, VISIBLE, {v:&Globals.timeout_ftp},},
	{"ha7", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_static, FS_r_timeout, FS_w_timeout, VISIBLE, {v:&Globals.timeout_ha7},},
	{"w1", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_static, FS_r_timeout, FS_w_timeout, VISIBLE, {v:&Globals.timeout_w1},},
	{"uncached", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_static, FS_r_yesno, FS_w_yesno, VISIBLE, {v:&Globals.uncached},},
};
struct device d_set_timeout = { "timeout", "timeout", ePN_settings, COUNT_OF_FILETYPES(set_timeout),
	set_timeout, NO_GENERIC_READ, NO_GENERIC_WRITE
};

struct filetype set_units[] = {
 	{"temperature_scale", 1, NON_AGGREGATE, ft_ascii, fc_static, FS_r_TS, FS_w_TS, VISIBLE, NO_FILETYPE_DATA,},
 	{"pressure_scale", 12, NON_AGGREGATE, ft_ascii, fc_static, FS_r_PS, FS_w_PS, VISIBLE, NO_FILETYPE_DATA,},
};
struct device d_set_units = { "units", "units", ePN_settings, COUNT_OF_FILETYPES(set_units),
	set_units, NO_GENERIC_READ, NO_GENERIC_WRITE
};

struct filetype set_alias[] = {
 	{"list", MAX_OWSERVER_PROTOCOL_PAYLOAD_SIZE, NON_AGGREGATE, ft_ascii, fc_static, FS_aliaslist, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA,},
	{"unaliased", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_static, FS_r_yesno, FS_w_yesno, VISIBLE, {v:&Globals.unaliased},},
};
struct device d_set_alias = { "alias", "alias", ePN_settings, COUNT_OF_FILETYPES(set_alias),
	set_alias, NO_GENERIC_READ, NO_GENERIC_WRITE
};

/* ------- Functions ------------ */

static ZERO_OR_ERROR FS_r_timeout(struct one_wire_query *owq)
{
	CACHE_RLOCK;
	OWQ_U(owq) = ((UINT *) OWQ_pn(owq).selected_filetype->data.v)[0];
	CACHE_RUNLOCK;
	return 0;
}

static ZERO_OR_ERROR FS_w_timeout(struct one_wire_query *owq)
{
	UINT previous;
	struct parsedname *pn = PN(owq);
	CACHE_WLOCK;
	//printf("FS_w_timeout!!!\n");
	previous = ((UINT *) pn->selected_filetype->data.v)[0];
	((UINT *) pn->selected_filetype->data.v)[0] = OWQ_U(owq);
	CACHE_WUNLOCK;
	if (previous > OWQ_U(owq)) {
		Cache_Clear();
	}
	return 0;
}

static ZERO_OR_ERROR FS_r_yesno(struct one_wire_query *owq)
{
	OWQ_Y(owq) = ((UINT *) OWQ_pn(owq).selected_filetype->data.v)[0];
	return 0;
}

static ZERO_OR_ERROR FS_w_yesno(struct one_wire_query *owq)
{
	((UINT *) OWQ_pn(owq).selected_filetype->data.v)[0] = OWQ_Y(owq);
	SetLocalControlFlags() ;
	return 0;
}

static ZERO_OR_ERROR FS_w_TS(struct one_wire_query *owq)
{
	ZERO_OR_ERROR ret = 0;
	if (OWQ_size(owq) == 0 || OWQ_offset(owq) > 0) {
		return 0;				/* do nothing */
	}

	switch (OWQ_buffer(owq)[0]) {
	case 'C':
	case 'c':
		Globals.temp_scale = temp_celsius ;
		break;
	case 'F':
	case 'f':
		Globals.temp_scale = temp_fahrenheit ;
		break;
	case 'R':
	case 'r':
		Globals.temp_scale = temp_rankine ;
		break;
	case 'K':
	case 'k':
		Globals.temp_scale = temp_kelvin ;
		break;
	default:
		ret = -EINVAL;
	}
	SetLocalControlFlags() ;
	return ret;
}

static ZERO_OR_ERROR FS_w_PS(struct one_wire_query *owq)
{
	int ret = -EINVAL ;
	enum pressure_type pindex ;
	if (OWQ_size(owq) < 2 || OWQ_offset(owq) > 0) {
		return -EINVAL ;				/* do nothing */
	}

	for ( pindex = 0 ; pindex < pressure_end_mark ; ++pindex ) {
		if ( strncasecmp(  OWQ_buffer(owq), PressureScaleName(pindex), 2 ) == 0 ) {
			Globals.pressure_scale = pindex ;
			SetLocalControlFlags() ;
			ret = 0 ;
			break;
		}
	}
	return ret;
}

static ZERO_OR_ERROR FS_r_TS(struct one_wire_query *owq)
{
	return OWQ_format_output_offset_and_size_z(TemperatureScaleName(Globals.temp_scale), owq);
}

static ZERO_OR_ERROR FS_r_PS(struct one_wire_query *owq)
{
	return OWQ_format_output_offset_and_size_z(PressureScaleName(Globals.pressure_scale), owq);
}

static ZERO_OR_ERROR FS_aliaslist( struct one_wire_query * owq )
{
	struct memblob mb ;
	ZERO_OR_ERROR zoe ;
	MemblobInit( &mb, PATH_MAX ) ;
	Aliaslist( &mb ) ;
	if ( MemblobPure( &mb ) ) {
		zoe = OWQ_format_output_offset_and_size( (char *) MemblobData( &mb ), MemblobLength( &mb ), owq ) ;
	} else {
		zoe = -EINVAL ;
	}
	MemblobClear( &mb ) ;
	return zoe ;
}
