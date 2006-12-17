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

#include <config.h>
#include "owfs_config.h"
#include "ow_1977.h"

/* ------- Prototypes ----------- */

/* DS1977 counter */
bREAD_FUNCTION(FS_r_mem);
bWRITE_FUNCTION(FS_w_mem);
bREAD_FUNCTION(FS_r_page);
bWRITE_FUNCTION(FS_w_page);
uREAD_FUNCTION(FS_ver);
yREAD_FUNCTION(FS_r_pwd);
yWRITE_FUNCTION(FS_w_pwd);
bWRITE_FUNCTION(FS_set);
aWRITE_FUNCTION(FS_nset);
#if OW_CACHE
bWRITE_FUNCTION(FS_use);
aWRITE_FUNCTION(FS_nuse);
#endif							/* OW_CACHE */

/* ------- Structures ----------- */

struct aggregate A1977 = { 511, ag_numbers, ag_separate, };
struct filetype DS1977[] = {
	F_STANDARD,
  {"memory", 32704, NULL, ft_binary, fc_stable, {b: FS_r_mem}, {b: FS_w_mem}, {v:NULL},},
  {"pages", 0, NULL, ft_subdir, fc_stable, {v: NULL}, {v: NULL}, {v:NULL},},
  {"pages/page", 64, &A1977, ft_binary, fc_stable, {b: FS_r_page}, {b: FS_w_page}, {v:NULL},},
  {"set_password", 0, NULL, ft_subdir, fc_stable, {v: NULL}, {v: NULL}, {v:NULL},},
  {"set_password/read", 8, NULL, ft_binary, fc_stable, {v: NULL}, {b: FS_set}, {i:0},},
  {"set_password/full", 8, NULL, ft_binary, fc_stable, {v: NULL}, {b: FS_set}, {i:8},},
  {"set_password/enabled", 1, NULL, ft_yesno, fc_stable, {y: FS_r_pwd}, {y: FS_w_pwd}, {v:NULL},},
  {"set_number", 0, NULL, ft_subdir, fc_stable, {v: NULL}, {v: NULL}, {v:NULL},},
  {"set_number/read", 47, NULL, ft_ascii, fc_stable, {v: NULL}, {a: FS_nset}, {i:0},},
  {"set_number/full", 47, NULL, ft_ascii, fc_stable, {v: NULL}, {a: FS_nset}, {i:8},},
  {"version", 12, NULL, ft_unsigned, fc_stable, {u: FS_ver}, {v: NULL}, {v:NULL},},
#if OW_CACHE
  {"use_password", 0, NULL, ft_subdir, fc_stable, {v: NULL}, {v: NULL}, {v:NULL},},
  {"use_password/read", 8, NULL, ft_binary, fc_stable, {v: NULL}, {b: FS_use}, {i:0},},
  {"use_password/full", 8, NULL, ft_binary, fc_stable, {v: NULL}, {b: FS_use}, {i:8},},
  {"use_number", 0, NULL, ft_subdir, fc_stable, {v: NULL}, {v: NULL}, {v:NULL},},
  {"use_number/read", 47, NULL, ft_ascii, fc_stable, {v: NULL}, {a: FS_nuse}, {i:0},},
  {"use_number/full", 47, NULL, ft_ascii, fc_stable, {v: NULL}, {a: FS_nuse}, {i:8},},
#endif							/*OW_CACHE */
};

DeviceEntryExtended(37, DS1977, DEV_resume | DEV_ovdr);

/* Persistent storage */
static struct internal_prop ip_rea = { "REA", fc_persistent };
static struct internal_prop ip_ful = { "FUL", fc_persistent };

/* ------- Functions ------------ */

/* DS2423 */
static int OW_w_mem(const BYTE * data, const size_t size,
					const off_t offset, const struct parsedname *pn);
static int OW_r_mem(BYTE * data, const size_t size, const off_t offset,
					const struct parsedname *pn);
static int OW_r_pmem(BYTE * data, const BYTE * pwd, const size_t size,
					 const off_t offset, const struct parsedname *pn);
static int OW_ver(UINT * u, const struct parsedname *pn);
static int OW_verify(BYTE * pwd, const off_t offset,
					 const struct parsedname *pn);
static int OW_clear(const struct parsedname *pn);

/* 1977 password */
static int FS_r_page(BYTE * buf, const size_t size, const off_t offset,
					 const struct parsedname *pn)
{
	if (OW_r_mem(buf, size, offset + ((pn->extension) << 6), pn))
		return -EACCES;
	return size;
}

static int FS_w_page(const BYTE * buf, const size_t size,
					 const off_t offset, const struct parsedname *pn)
{
	if (OW_w_mem(buf, size, offset + ((pn->extension) << 6), pn))
		return -EACCES;
	return 0;
}

static int FS_r_mem(BYTE * buf, const size_t size, const off_t offset,
					const struct parsedname *pn)
{
	if (OW_read_paged(buf, size, offset, pn, 64, OW_r_mem))
		return -EACCES;
	return size;
}

static int FS_w_mem(const BYTE * buf, const size_t size,
					const off_t offset, const struct parsedname *pn)
{
	if (OW_write_paged(buf, size, offset, pn, 64, OW_w_mem))
		return -EACCES;
	return 0;
}

static int FS_ver(UINT * u, const struct parsedname *pn)
{
	return OW_ver(u, pn) ? -EINVAL : 0;
}

static int FS_r_pwd(int *y, const struct parsedname *pn)
{
	BYTE p;
	if (OW_r_mem(&p, 1, 0x7FD0, pn))
		return -EACCES;
	y[0] = (p == 0xAA);
	return 0;
}

static int FS_w_pwd(const int *y, const struct parsedname *pn)
{
	BYTE p = y[0] ? 0x00 : 0xAA;
	if (OW_w_mem(&p, 1, 0x7FD0, pn))
		return -EACCES;
	return 0;
}

static int FS_nset(const char *buf, const size_t size, const off_t offset,
				   const struct parsedname *pn)
{
	char a[48];
	char *end;
	union {
		BYTE b[8];
		long long int i;
	} xfer;

	bzero(a, 48);
	bzero(xfer.b, 8);
	memcpy(&a[offset], buf, size);
	xfer.i = strtoll(a, &end, 0);
	if (end == a || errno)
		return -EINVAL;
	return FS_set(xfer.b, 8, 0, pn);
}

static int FS_set(const BYTE * buf, const size_t size, const off_t offset,
				  const struct parsedname *pn)
{
	int ret;
	BYTE p[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, };
	int oldy, y = 0;
	size_t off = 0x7FC0 + pn->ft->data.i;
	size_t s = size;
	if (s > 8)
		s = 8;
	memcpy(p, buf, s);

	(void) offset;

	/* disable passwords */
	if (FS_r_pwd(&oldy, pn))
		return -EACCES;
	if (oldy)
		if (FS_w_pwd(&y, pn))
			return -EACCES;

	/* write password */
	ret = FS_w_mem(p, 8, off, pn);
	OW_clear(pn);
	if (ret)
		return -EINVAL;

	/* Verify */
	if (OW_verify(p, off, pn))
		return -EINVAL;

#if OW_CACHE
	/* set cache */
	FS_use(p, 8, 0, pn);
#endif							/* OW_CACHE */

	/* reenable passwords */
	if (oldy)
		if (FS_w_pwd(&oldy, pn))
			return -EACCES;

	return 0;
}

#if OW_CACHE
static int FS_nuse(const char *buf, const size_t size, const off_t offset,
				   const struct parsedname *pn)
{
	char a[48];
	char *end;
	union {
		BYTE b[8];
		long long int i;
	} xfer;

	bzero(a, 48);
	bzero(xfer.b, 8);
	memcpy(&a[offset], buf, size);
	xfer.i = strtoll(a, &end, 0);
	if (end == a || errno)
		return -EINVAL;
	return FS_use(xfer.b, 8, 0, pn);
}

static int FS_use(const BYTE * buf, const size_t size, const off_t offset,
				  const struct parsedname *pn)
{
	BYTE p[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, };
	size_t s = size;
	if (s > 8)
		s = 8;
	memcpy(p, buf, s);

	(void) offset;

	if (Cache_Add_Internal(p, s, pn->ft->data.i ? &ip_ful : &ip_rea, pn))
		return -EINVAL;
	return 0;
}
#endif							/* OW_CACHE */

static int OW_w_mem(const BYTE * data, const size_t size,
					const off_t offset, const struct parsedname *pn)
{
	BYTE p[1 + 2 + 64 + 2] = { 0x0F, offset & 0xFF, offset >> 8, };
	int ret;

	/* Copy to scratchpad */
	memcpy(&p[3], data, size);

	BUSLOCK(pn);
	ret = BUS_select(pn) || BUS_send_data(p, size + 3, pn);
	if (ret == 0 && ((offset + size) & 0x2F) == 0)
		ret = BUS_readin_data(&p[size + 3], 2, pn)
			|| CRC16(p, 1 + 2 + size + 2);
	BUSUNLOCK(pn);
	if (ret)
		return 1;

	/* Re-read scratchpad and compare */
	/* Note that we tacitly shift the data one byte down for the E/S byte */
	p[0] = 0xAA;
	BUSLOCK(pn);
	ret = BUS_select(pn) || BUS_send_data(p, 1, pn)
		|| BUS_readin_data(&p[1], 3 + size, pn)
		|| memcmp(&p[4], data, size);
	BUSUNLOCK(pn);
	if (ret)
		return 1;

#if OW_CACHE
	Cache_Get_Internal_Strict((void *) (&p[4]), 8, &ip_ful, pn);
#endif							/* OW_CACHE */

	/* Copy Scratchpad to SRAM */
	p[0] = 0x99;
	BUSLOCK(pn);
	ret = BUS_select(pn) || BUS_send_data(p, 4 + 7, pn)
		|| BUS_PowerByte(p[4 + 7], &p[4 + 7], 10, pn);
	BUSUNLOCK(pn);
	if (ret)
		return 1;

	return 0;
}

static int OW_r_mem(BYTE * data, const size_t size, const off_t offset,
					const struct parsedname *pn)
{
	BYTE pwd[8] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, };

#if OW_CACHE
	if (Cache_Get_Internal_Strict((void *) pwd, sizeof(pwd), &ip_ful, pn)
		|| OW_r_pmem(data, pwd, size, offset, pn))
		return 0;
	Cache_Get_Internal_Strict((void *) pwd, sizeof(pwd), &ip_rea, pn);
#endif							/* OW_CACHE */
	return OW_r_pmem(data, pwd, size, offset, pn);
}

static int OW_r_pmem(BYTE * data, const BYTE * pwd, const size_t size,
					 const off_t offset, const struct parsedname *pn)
{
	int ret;
	BYTE p[1 + 2 + 64 + 2] = { 0x69, offset & 0xFF, offset >> 8, };

	BUSLOCK(pn);
	ret = BUS_select(pn) || BUS_send_data(p, 3, pn)
		|| BUS_send_data(pwd, 7, pn)
		|| BUS_PowerByte(pwd[7], &pwd[7], 5, pn);
	if (ret) {
	} else if ((offset + size) & 0x3F) {	/* not a page boundary */
		ret = BUS_readin_data(&p[3], size, pn);
	} else {
		ret = BUS_readin_data(&p[3], size + 2, pn) || CRC16(p, size + 5);
	}
	BUSUNLOCK(pn);

	if (ret)
		return ret;
	memcpy(data, &p[3], size);
	return 0;
}

static int OW_ver(UINT * u, const struct parsedname *pn)
{
	int ret;
	BYTE p[] = { 0xCC, };
	BYTE b[2];

	BUSLOCK(pn);
	ret = BUS_select(pn) || BUS_send_data(p, 1, pn)
		|| BUS_readin_data(b, 2, pn);
	BUSUNLOCK(pn);

	if (ret || b[0] != b[1])
		return 1;
	u[0] = b[0];
}

static int OW_verify(BYTE * pwd, const off_t offset,
					 const struct parsedname *pn)
{
	int ret;
	BYTE p[1 + 2 + 8] = { 0xC3, offset & 0xFF, offset >> 8, };
	BYTE c;

	BUSLOCK(pn);
	ret = BUS_select(pn) || BUS_send_data(p, 3 + 7, pn)
		|| BUS_PowerByte(p[3 + 7], &p[3 + 7], 5, pn)
		|| BUS_readin_data(&c, 1, pn);
	BUSUNLOCK(pn);

	return (ret || c != 0xFF) ? 1 : 0;
}

/* Clear first 16 bytes of scratchpad after password setting */
static int OW_clear(const struct parsedname *pn)
{
	BYTE p[1 + 2 + 16] = { 0x0F, 0x00, 0x00, };
	int ret;

	/* Copy to scratchpad */
	bzero(&p[3], 16);

	BUSLOCK(pn);
	ret = BUS_select(pn) || BUS_send_data(p, 3 + 16, pn);
	BUSUNLOCK(pn);
	return ret;
}
