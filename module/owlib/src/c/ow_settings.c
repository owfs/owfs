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
  {"volatile", 15, NULL, ft_unsigned, fc_static, {o: FS_r_timeout}, {o: FS_w_timeout}, {v:&Global.timeout_volatile},},
  {"stable", 15, NULL, ft_unsigned, fc_static, {o: FS_r_timeout}, {o: FS_w_timeout}, {v:&Global.timeout_stable},},
  {"directory", 15, NULL, ft_unsigned, fc_static, {o: FS_r_timeout}, {o: FS_w_timeout}, {v:&Global.timeout_directory},},
  {"presence", 15, NULL, ft_unsigned, fc_static, {o: FS_r_timeout}, {o: FS_w_timeout}, {v:&Global.timeout_presence},},
  {"serial", 15, NULL, ft_unsigned, fc_static, {o: FS_r_timeout}, {o: FS_w_timeout}, {v:&Global.timeout_serial},},
  {"usb", 15, NULL, ft_unsigned, fc_static, {o: FS_r_timeout}, {o: FS_w_timeout}, {v:&Global.timeout_usb},},
  {"network", 15, NULL, ft_unsigned, fc_static, {o: FS_r_timeout}, {o: FS_w_timeout}, {v:&Global.timeout_network},},
  {"server", 15, NULL, ft_unsigned, fc_static, {o: FS_r_timeout}, {o: FS_w_timeout}, {v:&Global.timeout_server},},
  {"ftp", 15, NULL, ft_unsigned, fc_static, {o: FS_r_timeout}, {o: FS_w_timeout}, {v:&Global.timeout_ftp},},
}

;
struct device d_set_cache =
	{ "timeout", "timeout", pn_settings, NFT(set_cache), set_cache };
struct filetype set_units[] = {
  {"temperature_scale", 1, NULL, ft_ascii, fc_static, {o: FS_r_TS}, {o: FS_w_TS}, {v:NULL},},
}

;
struct device d_set_units =
	{ "units", "units", pn_settings, NFT(set_units), set_units };

/* ------- Functions ------------ */

static int FS_r_timeout(struct one_wire_query * owq)
{
	CACHELOCK;
    OWQ_I(owq) = ((UINT *) OWQ_pn(owq).ft->data.v)[0];
	CACHEUNLOCK;
	return 0;
}

static int FS_w_timeout(struct one_wire_query * owq)
{
	int previous;
    struct parsedname * pn = PN(owq) ;
	CACHELOCK;
    printf("FS_w_timeout!!!\n");
	previous = ((UINT *) pn->ft->data.v)[0];
    ((UINT *) pn->ft->data.v)[0] = OWQ_I(owq);
	CACHEUNLOCK;
    if (previous > OWQ_I(owq))
		Cache_Clear();
	return 0;
}

static int FS_w_TS(struct one_wire_query * owq)
{
    if ( OWQ_size(owq) == 0 || OWQ_offset(owq) > 0 ) return 0 ; /* do nothing */
    switch (OWQ_buffer(owq)[0]) {
	case 'C':
		set_semiglobal(&SemiGlobal, TEMPSCALE_MASK, TEMPSCALE_BIT,
					   temp_celsius);
		return 0;
	case 'F':
		set_semiglobal(&SemiGlobal, TEMPSCALE_MASK, TEMPSCALE_BIT,
					   temp_fahrenheit);
		return 0;
	case 'R':
		set_semiglobal(&SemiGlobal, TEMPSCALE_MASK, TEMPSCALE_BIT,
					   temp_rankine);
		return 0;
	case 'K':
		set_semiglobal(&SemiGlobal, TEMPSCALE_MASK, TEMPSCALE_BIT,
					   temp_kelvin);
		return 0;
	}
	return -EINVAL;
}

static int FS_r_TS(struct one_wire_query * owq)
{
    Fowq_output_offset_and_size_z( TemperatureScaleName(TemperatureScale(PN(owq))), owq);
    return 0 ;
}
