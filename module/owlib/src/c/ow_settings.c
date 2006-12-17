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
iREAD_FUNCTION(FS_r_timeout);
iWRITE_FUNCTION(FS_w_timeout);
aREAD_FUNCTION(FS_r_TS);
aWRITE_FUNCTION(FS_w_TS);

/* -------- Structures ---------- */

struct filetype set_cache[] = {
  {"volatile", 15, NULL, ft_unsigned, fc_static, {i: FS_r_timeout}, {i: FS_w_timeout}, {v:&Global.timeout_volatile},},
  {"stable", 15, NULL, ft_unsigned, fc_static, {i: FS_r_timeout}, {i: FS_w_timeout}, {v:&Global.timeout_stable},},
  {"directory", 15, NULL, ft_unsigned, fc_static, {i: FS_r_timeout}, {i: FS_w_timeout}, {v:&Global.timeout_directory},},
  {"presence", 15, NULL, ft_unsigned, fc_static, {i: FS_r_timeout}, {i: FS_w_timeout}, {v:&Global.timeout_presence},},
  {"serial", 15, NULL, ft_unsigned, fc_static, {i: FS_r_timeout}, {i: FS_w_timeout}, {v:&Global.timeout_serial},},
  {"usb", 15, NULL, ft_unsigned, fc_static, {i: FS_r_timeout}, {i: FS_w_timeout}, {v:&Global.timeout_usb},},
  {"network", 15, NULL, ft_unsigned, fc_static, {i: FS_r_timeout}, {i: FS_w_timeout}, {v:&Global.timeout_network},},
  {"server", 15, NULL, ft_unsigned, fc_static, {i: FS_r_timeout}, {i: FS_w_timeout}, {v:&Global.timeout_server},},
  {"ftp", 15, NULL, ft_unsigned, fc_static, {i: FS_r_timeout}, {i: FS_w_timeout}, {v:&Global.timeout_ftp},},
}

;
struct device d_set_cache =
	{ "timeout", "timeout", pn_settings, NFT(set_cache), set_cache };
struct filetype set_units[] = {
  {"temperature_scale", 1, NULL, ft_ascii, fc_static, {a: FS_r_TS}, {a: FS_w_TS}, {v:NULL},},
}

;
struct device d_set_units =
	{ "units", "units", pn_settings, NFT(set_units), set_units };

/* ------- Functions ------------ */

static int FS_r_timeout(int *i, const struct parsedname *pn)
{
	CACHELOCK;
	i[0] = ((UINT *) pn->ft->data.v)[0];
	CACHEUNLOCK;
	return 0;
}

static int FS_w_timeout(const int *i, const struct parsedname *pn)
{
	int previous;
	CACHELOCK;
	previous = ((UINT *) pn->ft->data.v)[0];
	((UINT *) pn->ft->data.v)[0] = i[0];
	CACHEUNLOCK;
	if (previous > i[0])
		Cache_Clear();
	return 0;
}

static int FS_w_TS(const char *buf, const size_t size, const off_t offset,
				   const struct parsedname *pn)
{
	(void) size;
	(void) offset;
	(void) pn;
	switch (buf[0]) {
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

static int FS_r_TS(char *buf, const size_t size, const off_t offset,
				   const struct parsedname *pn)
{
	const char *t = TemperatureScaleName(TemperatureScale(pn));
	return FS_output_ascii_z(buf, size, offset, t);
}
