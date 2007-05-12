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
READ_FUNCTION(FS_r_mem);
WRITE_FUNCTION(FS_w_mem);
READ_FUNCTION(FS_r_page);
WRITE_FUNCTION(FS_w_page);
READ_FUNCTION(FS_ver);
READ_FUNCTION(FS_r_pwd);
WRITE_FUNCTION(FS_w_pwd);
WRITE_FUNCTION(FS_set);
WRITE_FUNCTION(FS_nset);
#if OW_CACHE
WRITE_FUNCTION(FS_use);
WRITE_FUNCTION(FS_nuse);
#endif							/* OW_CACHE */

/* ------- Structures ----------- */

struct aggregate A1977 = { 511, ag_numbers, ag_separate, };
struct filetype DS1977[] = {
	F_STANDARD,
  {"memory", 32704, NULL, ft_binary, fc_stable,  {o:FS_r_mem}, {o:FS_w_mem}, {v:NULL},},  {"memory", 32704, NULL, ft_binary, fc_stable,  {o:FS_r_mem}, {o:FS_w_mem}, {v:NULL},},  {"memory", 32704, NULL, ft_binary, fc_stable,  {o:FS_r_mem}, {o:FS_w_mem}, {v:NULL},},  {"memory", 32704, NULL, ft_binary, fc_stable, {o: FS_r_mem}, {o: FS_w_mem}, {v:NULL},},
  {"pages",PROPERTY_LENGTH_SUBDIR, NULL, ft_subdir, fc_stable,  {o:NO_READ_FUNCTION}, {o:NO_WRITE_FUNCTION}, {v:NULL},},  {"pages",PROPERTY_LENGTH_SUBDIR, NULL, ft_subdir, fc_stable,  {o:NO_READ_FUNCTION}, {o:NO_WRITE_FUNCTION}, {v:NULL},},  {"pages",PROPERTY_LENGTH_SUBDIR, NULL, ft_subdir, fc_stable,  {o:NO_READ_FUNCTION}, {o:NO_WRITE_FUNCTION}, {v:NULL},},  {"pages",PROPERTY_LENGTH_SUBDIR, NULL, ft_subdir, fc_stable, {o: NULL}, {o: NULL}, {v:NULL},},
  {"pages/page", 64, &A1977, ft_binary, fc_stable,  {o:FS_r_page}, {o:FS_w_page}, {v:NULL},},  {"pages/page", 64, &A1977, ft_binary, fc_stable,  {o:FS_r_page}, {o:FS_w_page}, {v:NULL},},  {"pages/page", 64, &A1977, ft_binary, fc_stable,  {o:FS_r_page}, {o:FS_w_page}, {v:NULL},},  {"pages/page", 64, &A1977, ft_binary, fc_stable, {o: FS_r_page}, {o: FS_w_page}, {v:NULL},},
  {"set_password",PROPERTY_LENGTH_SUBDIR, NULL, ft_subdir, fc_stable,  {o:NO_READ_FUNCTION}, {o:NO_WRITE_FUNCTION}, {v:NULL},},  {"set_password",PROPERTY_LENGTH_SUBDIR, NULL, ft_subdir, fc_stable,  {o:NO_READ_FUNCTION}, {o:NO_WRITE_FUNCTION}, {v:NULL},},  {"set_password",PROPERTY_LENGTH_SUBDIR, NULL, ft_subdir, fc_stable,  {o:NO_READ_FUNCTION}, {o:NO_WRITE_FUNCTION}, {v:NULL},},  {"set_password",PROPERTY_LENGTH_SUBDIR, NULL, ft_subdir, fc_stable, {o: NULL}, {o: NULL}, {v:NULL},},
  {"set_password/read", 8, NULL, ft_binary, fc_stable,  {o:NO_READ_FUNCTION}, {o:FS_set}, {i:0},},  {"set_password/read", 8, NULL, ft_binary, fc_stable,  {o:NO_READ_FUNCTION}, {o:FS_set}, {i:0},},  {"set_password/read", 8, NULL, ft_binary, fc_stable,  {o:NO_READ_FUNCTION}, {o:FS_set}, {i:0},},  {"set_password/read", 8, NULL, ft_binary, fc_stable, {o: NULL}, {o: FS_set}, {i:0},},
  {"set_password/full", 8, NULL, ft_binary, fc_stable,  {o:NO_READ_FUNCTION}, {o:FS_set}, {i:8},},  {"set_password/full", 8, NULL, ft_binary, fc_stable,  {o:NO_READ_FUNCTION}, {o:FS_set}, {i:8},},  {"set_password/full", 8, NULL, ft_binary, fc_stable,  {o:NO_READ_FUNCTION}, {o:FS_set}, {i:8},},  {"set_password/full", 8, NULL, ft_binary, fc_stable, {o: NULL}, {o: FS_set}, {i:8},},
  {"set_password/enabled",PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_stable,  {o:FS_r_pwd}, {o:FS_w_pwd}, {v:NULL},},  {"set_password/enabled",PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_stable,  {o:FS_r_pwd}, {o:FS_w_pwd}, {v:NULL},},  {"set_password/enabled",PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_stable,  {o:FS_r_pwd}, {o:FS_w_pwd}, {v:NULL},},  {"set_password/enabled",PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_stable, {o: FS_r_pwd}, {o: FS_w_pwd}, {v:NULL},},
  {"set_number",PROPERTY_LENGTH_SUBDIR, NULL, ft_subdir, fc_stable,  {o:NO_READ_FUNCTION}, {o:NO_WRITE_FUNCTION}, {v:NULL},},  {"set_number",PROPERTY_LENGTH_SUBDIR, NULL, ft_subdir, fc_stable,  {o:NO_READ_FUNCTION}, {o:NO_WRITE_FUNCTION}, {v:NULL},},  {"set_number",PROPERTY_LENGTH_SUBDIR, NULL, ft_subdir, fc_stable,  {o:NO_READ_FUNCTION}, {o:NO_WRITE_FUNCTION}, {v:NULL},},  {"set_number",PROPERTY_LENGTH_SUBDIR, NULL, ft_subdir, fc_stable, {o: NULL}, {o: NULL}, {v:NULL},},
  {"set_number/read", 47, NULL, ft_ascii, fc_stable,  {o:NO_READ_FUNCTION}, {o:FS_nset}, {i:0},},  {"set_number/read", 47, NULL, ft_ascii, fc_stable,  {o:NO_READ_FUNCTION}, {o:FS_nset}, {i:0},},  {"set_number/read", 47, NULL, ft_ascii, fc_stable,  {o:NO_READ_FUNCTION}, {o:FS_nset}, {i:0},},  {"set_number/read", 47, NULL, ft_ascii, fc_stable, {o: NULL}, {o: FS_nset}, {i:0},},
  {"set_number/full", 47, NULL, ft_ascii, fc_stable,  {o:NO_READ_FUNCTION}, {o:FS_nset}, {i:8},},  {"set_number/full", 47, NULL, ft_ascii, fc_stable,  {o:NO_READ_FUNCTION}, {o:FS_nset}, {i:8},},  {"set_number/full", 47, NULL, ft_ascii, fc_stable,  {o:NO_READ_FUNCTION}, {o:FS_nset}, {i:8},},  {"set_number/full", 47, NULL, ft_ascii, fc_stable, {o: NULL}, {o: FS_nset}, {i:8},},
  {"version",PROPERTY_LENGTH_UNSIGNED, NULL, ft_unsigned, fc_stable,  {o:FS_ver}, {o:NO_WRITE_FUNCTION}, {v:NULL},},  {"version",PROPERTY_LENGTH_UNSIGNED, NULL, ft_unsigned, fc_stable,  {o:FS_ver}, {o:NO_WRITE_FUNCTION}, {v:NULL},},  {"version",PROPERTY_LENGTH_UNSIGNED, NULL, ft_unsigned, fc_stable,  {o:FS_ver}, {o:NO_WRITE_FUNCTION}, {v:NULL},},  {"version",PROPERTY_LENGTH_UNSIGNED, NULL, ft_unsigned, fc_stable, {o: FS_ver}, {o: NULL}, {v:NULL},},
#if OW_CACHE
  {"use_password",PROPERTY_LENGTH_SUBDIR, NULL, ft_subdir, fc_stable,  {o:NO_READ_FUNCTION}, {o:NO_WRITE_FUNCTION}, {v:NULL},},  {"use_password",PROPERTY_LENGTH_SUBDIR, NULL, ft_subdir, fc_stable,  {o:NO_READ_FUNCTION}, {o:NO_WRITE_FUNCTION}, {v:NULL},},  {"use_password",PROPERTY_LENGTH_SUBDIR, NULL, ft_subdir, fc_stable,  {o:NO_READ_FUNCTION}, {o:NO_WRITE_FUNCTION}, {v:NULL},},  {"use_password",PROPERTY_LENGTH_SUBDIR, NULL, ft_subdir, fc_stable, {o: NULL}, {o: NULL}, {v:NULL},},
  {"use_password/read", 8, NULL, ft_binary, fc_stable,  {o:NO_READ_FUNCTION}, {o:FS_use}, {i:0},},  {"use_password/read", 8, NULL, ft_binary, fc_stable,  {o:NO_READ_FUNCTION}, {o:FS_use}, {i:0},},  {"use_password/read", 8, NULL, ft_binary, fc_stable,  {o:NO_READ_FUNCTION}, {o:FS_use}, {i:0},},  {"use_password/read", 8, NULL, ft_binary, fc_stable, {o: NULL}, {o: FS_use}, {i:0},},
  {"use_password/full", 8, NULL, ft_binary, fc_stable,  {o:NO_READ_FUNCTION}, {o:FS_use}, {i:8},},  {"use_password/full", 8, NULL, ft_binary, fc_stable,  {o:NO_READ_FUNCTION}, {o:FS_use}, {i:8},},  {"use_password/full", 8, NULL, ft_binary, fc_stable,  {o:NO_READ_FUNCTION}, {o:FS_use}, {i:8},},  {"use_password/full", 8, NULL, ft_binary, fc_stable, {o: NULL}, {o: FS_use}, {i:8},},
  {"use_number",PROPERTY_LENGTH_SUBDIR, NULL, ft_subdir, fc_stable,  {o:NO_READ_FUNCTION}, {o:NO_WRITE_FUNCTION}, {v:NULL},},  {"use_number",PROPERTY_LENGTH_SUBDIR, NULL, ft_subdir, fc_stable,  {o:NO_READ_FUNCTION}, {o:NO_WRITE_FUNCTION}, {v:NULL},},  {"use_number",PROPERTY_LENGTH_SUBDIR, NULL, ft_subdir, fc_stable,  {o:NO_READ_FUNCTION}, {o:NO_WRITE_FUNCTION}, {v:NULL},},  {"use_number",PROPERTY_LENGTH_SUBDIR, NULL, ft_subdir, fc_stable, {o: NULL}, {o: NULL}, {v:NULL},},
  {"use_number/read", 47, NULL, ft_ascii, fc_stable,  {o:NO_READ_FUNCTION}, {o:FS_nuse}, {i:0},},  {"use_number/read", 47, NULL, ft_ascii, fc_stable,  {o:NO_READ_FUNCTION}, {o:FS_nuse}, {i:0},},  {"use_number/read", 47, NULL, ft_ascii, fc_stable,  {o:NO_READ_FUNCTION}, {o:FS_nuse}, {i:0},},  {"use_number/read", 47, NULL, ft_ascii, fc_stable, {o: NULL}, {o: FS_nuse}, {i:0},},
  {"use_number/full", 47, NULL, ft_ascii, fc_stable,  {o:NO_READ_FUNCTION}, {o:FS_nuse}, {i:8},},  {"use_number/full", 47, NULL, ft_ascii, fc_stable,  {o:NO_READ_FUNCTION}, {o:FS_nuse}, {i:8},},  {"use_number/full", 47, NULL, ft_ascii, fc_stable,  {o:NO_READ_FUNCTION}, {o:FS_nuse}, {i:8},},  {"use_number/full", 47, NULL, ft_ascii, fc_stable, {o: NULL}, {o: FS_nuse}, {i:8},},
#endif							/*OW_CACHE */
};

DeviceEntryExtended(37, DS1977, DEV_resume | DEV_ovdr);

#define _1W_WRITE_SCRATCHPAD 0x0F
#define _1W_READ_SCRATCHPAD 0xAA
#define _1W_COPY_SCRATCHPAD_WITH_PASSWORD 0x99
#define _1W_READ_MEMORY_WITH_PASSWORD 0xC3
#define _1W_VERIFY_PASSWORD 0x69
#define _1W_READ_VERSION 0xCC

/* Persistent storage */
static struct internal_prop ip_rea = { "REA", fc_persistent };
static struct internal_prop ip_ful = { "FUL", fc_persistent };

/* ------- Functions ------------ */

/* DS2423 */
static int OW_w_mem( BYTE * data,  size_t size,
					 off_t offset,  struct parsedname *pn);
static int OW_r_mem(BYTE * data,  size_t size,  off_t offset,
					 struct parsedname *pn);
static int OW_r_pmem(BYTE * data,  BYTE * pwd,  size_t size,
					  off_t offset,  struct parsedname *pn);
static int OW_ver(UINT * u,  struct parsedname *pn);
static int OW_verify(BYTE * pwd,  off_t offset,
					  struct parsedname *pn);
static int OW_clear( struct parsedname *pn);

/* 1977 password */
static int FS_r_page(struct one_wire_query * owq)
{
	size_t pagesize = 64 ;
	if ( OW_readwrite_paged( owq, OWQ_pn(owq).extension, pagesize, OW_r_mem ) )
		return -EACCES;
    return 0;
}

static int FS_w_page(struct one_wire_query * owq)
{
	size_t pagesize = 64 ;
	if ( OW_readwrite_paged( owq, OWQ_pn(owq).extension, pagesize, OW_w_mem ) )
		return -EACCES;
	return 0;
}

static int FS_r_mem(struct one_wire_query * owq)
{
	size_t pagesize = 64 ;
	if ( OW_readwrite_paged( owq, 0, pagesize, OW_r_mem ) )
		return -EACCES;
    return OWQ_size(owq);
}

static int FS_w_mem(struct one_wire_query * owq)
{
	size_t pagesize = 64 ;
	if ( OW_readwrite_paged( owq, 0, pagesize, OW_w_mem ) )
        return -EACCES;
	return 0;
}

static int FS_ver(struct one_wire_query * owq)
{
    return OW_ver(&OWQ_U(owq), PN(owq)) ? -EINVAL : 0;
}

static int FS_r_pwd(struct one_wire_query * owq)
{
	BYTE p;
    if (OW_r_mem(&p, 1, 0x7FD0, PN(owq)))
		return -EACCES;
    OWQ_Y(owq) = (p == _1W_READ_SCRATCHPAD);
	return 0;
}

static int FS_w_pwd(struct one_wire_query * owq)
{
    BYTE p = OWQ_Y(owq) ? 0x00 : _1W_READ_SCRATCHPAD;
    if (OW_w_mem(&p, 1, 0x7FD0, PN(owq)))
		return -EACCES;
	return 0;
}

static int FS_nset(struct one_wire_query * owq)
{
	char a[48];
	char *end;
	OWQ_make( owq_scratch ) ;
    union {
		BYTE b[8];
		int64_t i;
	} xfer;

	bzero(a, 48);
	bzero(xfer.b, 8);
    memcpy(&a[OWQ_offset(owq)], OWQ_buffer(owq), OWQ_size(owq));
	xfer.i = strtoll(a, &end, 0);
	if (end == a || errno)
		return -EINVAL;
    OWQ_create_shallow_single( owq_scratch, owq ) ; // won't bother to destroy
    OWQ_buffer(owq_scratch) = (char *) xfer.b ;
    OWQ_size(owq_scratch)= 8 ;
    OWQ_offset(owq_scratch) = 0 ;
    return FS_set(owq_scratch);
}

static int FS_set(struct one_wire_query * owq)
{
    struct parsedname * pn = PN(owq) ;
	OWQ_make( owq_scratch ) ;
    int ret;
	int oldy ;

    if ( OWQ_size(owq) < 8 ) return -ERANGE ;
    
    OWQ_create_shallow_single( owq_scratch, owq ) ; // won't bother to destroy

	/* disable passwords */
	if (FS_r_pwd(owq_scratch))
		return -EACCES;
    oldy = OWQ_Y(owq) ;
    if (oldy) {
        OWQ_Y(owq_scratch) = 0 ;
		if (FS_w_pwd(owq_scratch))
			return -EACCES;
    }

	/* write password */
    OWQ_offset(owq_scratch) = 0x7FC0 + pn->ft->data.i;
    OWQ_size(owq_scratch) = 8 ;
	ret = FS_w_mem(owq_scratch);
	OW_clear(pn);
	if (ret)
		return -EINVAL;

	/* Verify */
    if (OW_verify( (BYTE *) OWQ_buffer(owq_scratch),OWQ_offset(owq_scratch),PN(owq)))
		return -EINVAL;

#if OW_CACHE
	/* set cache */
    OWQ_offset(owq_scratch) = 0 ;
	FS_use(owq_scratch);
#endif							/* OW_CACHE */

	/* reenable passwords */
	if (oldy) {
        OWQ_Y(owq_scratch) = 1 ;
		if (FS_w_pwd(owq_scratch))
			return -EACCES;
    }

	return 0;
}

#if OW_CACHE
static int FS_nuse(struct one_wire_query * owq)
{
	char a[48];
	char *end;
	union {
		BYTE b[8];
		int64_t i;
	} xfer;
	OWQ_make( owq_scratch ) ;

	bzero(a, 48);
	bzero(xfer.b, 8);
    memcpy(&a[OWQ_offset(owq)], OWQ_buffer(owq), OWQ_size(owq));
	xfer.i = strtoll(a, &end, 0);
	if (end == a || errno)
		return -EINVAL;
    OWQ_create_shallow_single( owq_scratch, owq ) ; // won't bother to destroy
    OWQ_buffer(owq_scratch) = (char *) xfer.b ;
    OWQ_size(owq_scratch)= 8 ;
    OWQ_offset(owq_scratch) = 0 ;
    return FS_use(owq_scratch);
}

static int FS_use(struct one_wire_query * owq)
{
    if ( OWQ_size(owq) < 8 ) return -ERANGE ;
    
    if (Cache_Add_Internal((BYTE *) OWQ_buffer(owq), 8, OWQ_pn(owq).ft->data.i ? &ip_ful : &ip_rea, PN(owq)))
		return -EINVAL;
	return 0;
}
#endif							/* OW_CACHE */

static int OW_w_mem( BYTE * data,  size_t size,
					 off_t offset,  struct parsedname *pn)
{
    BYTE p[1 + 2 + 64 + 2] = { _1W_WRITE_SCRATCHPAD, LOW_HIGH_ADDRESS(offset), };
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
    p[0] = _1W_READ_SCRATCHPAD;
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
	p[0] = _1W_COPY_SCRATCHPAD_WITH_PASSWORD;
	BUSLOCK(pn);
	ret = BUS_select(pn) || BUS_send_data(p, 4 + 7, pn)
		|| BUS_PowerByte(p[4 + 7], &p[4 + 7], 10, pn);
	BUSUNLOCK(pn);
	if (ret)
		return 1;

	return 0;
}

static int OW_r_mem(BYTE * data,  size_t size,  off_t offset,
					 struct parsedname *pn)
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

static int OW_r_pmem(BYTE * data,  BYTE * pwd,  size_t size,
					  off_t offset,  struct parsedname *pn)
{
	int ret;
    BYTE p[1 + 2 + 64 + 2] = { _1W_VERIFY_PASSWORD, LOW_HIGH_ADDRESS(offset), };

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

static int OW_ver(UINT * u,  struct parsedname *pn)
{
	int ret;
    BYTE p[] = { _1W_READ_VERSION, };
	BYTE b[2];

	BUSLOCK(pn);
	ret = BUS_select(pn) || BUS_send_data(p, 1, pn)
		|| BUS_readin_data(b, 2, pn);
	BUSUNLOCK(pn);

	if (ret || b[0] != b[1])
		return 1;
	u[0] = b[0];
    return 0 ;
}

static int OW_verify(BYTE * pwd,  off_t offset,
					  struct parsedname *pn)
{
	int ret;
    BYTE p[1 + 2 + 8] = { _1W_READ_MEMORY_WITH_PASSWORD, LOW_HIGH_ADDRESS(offset), };
	BYTE c;

	BUSLOCK(pn);
	ret = BUS_select(pn) || BUS_send_data(p, 3 + 7, pn)
		|| BUS_PowerByte(p[3 + 7], &p[3 + 7], 5, pn)
		|| BUS_readin_data(&c, 1, pn);
	BUSUNLOCK(pn);

	return (ret || c != 0xFF) ? 1 : 0;
}

/* Clear first 16 bytes of scratchpad after password setting */
static int OW_clear( struct parsedname *pn)
{
    BYTE p[1 + 2 + 16] = { _1W_WRITE_SCRATCHPAD, LOW_HIGH_ADDRESS(0x00), };
	int ret;

	/* Copy to scratchpad */
	bzero(&p[3], 16);

	BUSLOCK(pn);
	ret = BUS_select(pn) || BUS_send_data(p, 3 + 16, pn);
	BUSUNLOCK(pn);
	return ret;
}
