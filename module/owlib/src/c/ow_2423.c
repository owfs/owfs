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
#include "ow_2423.h"

/* ------- Prototypes ----------- */

/* DS2423 counter */
bREAD_FUNCTION(FS_r_mem);
bWRITE_FUNCTION(FS_w_mem);
bREAD_FUNCTION(FS_r_page);
bWRITE_FUNCTION(FS_w_page);
uREAD_FUNCTION(FS_counter);
uREAD_FUNCTION(FS_pagecount);
#if OW_CACHE
uREAD_FUNCTION(FS_r_mincount);
uWRITE_FUNCTION(FS_w_mincount);
#endif							/* OW_CACHE */

/* ------- Structures ----------- */

struct aggregate A2423 = { 16, ag_numbers, ag_separate, };
struct aggregate A2423c = { 2, ag_letters, ag_separate, };
struct filetype DS2423[] = {
	F_STANDARD,
  {"memory", 512, NULL, ft_binary, fc_stable, {b: FS_r_mem}, {b: FS_w_mem}, {v:NULL},},
  {"pages", 0, NULL, ft_subdir, fc_volatile, {v: NULL}, {v: NULL}, {v:NULL},},
  {"pages/page", 32, &A2423, ft_binary, fc_stable, {b: FS_r_page}, {b: FS_w_page}, {v:NULL},},
  {"counters", 12, &A2423c, ft_unsigned, fc_volatile, {u: FS_counter}, {v: NULL}, {v:NULL},},
#if OW_CACHE
  {"mincount", 12, NULL, ft_unsigned, fc_volatile, {u: FS_r_mincount}, {u: FS_w_mincount}, {v:NULL},},
#endif							/*OW_CACHE */
  {"pages/count", 12, &A2423, ft_unsigned, fc_volatile, {u: FS_pagecount}, {v: NULL}, {v:NULL},},
};

DeviceEntryExtended(1 D, DS2423, DEV_ovdr);

/* Persistent storage */
static struct internal_prop ip_cum = { "CUM", fc_persistent };

/* ------- Functions ------------ */

/* DS2423 */
static int OW_w_mem(const BYTE * data, const size_t size,
					const off_t offset, const struct parsedname *pn);
static int OW_r_mem(BYTE * data, const size_t size, const off_t offset,
					const struct parsedname *pn);
static int OW_counter(UINT * counter, const int page,
					  const struct parsedname *pn);
static int OW_r_mem_counter(BYTE * p, UINT * counter, const size_t size,
							const off_t offset,
							const struct parsedname *pn);

/* 2423A/D Counter */
static int FS_r_page(BYTE * buf, const size_t size, const off_t offset,
					 const struct parsedname *pn)
{
	if (OW_r_mem(buf, size, offset + ((pn->extension) << 5), pn))
		return -EINVAL;
	return size;
}

static int FS_w_page(const BYTE * buf, const size_t size,
					 const off_t offset, const struct parsedname *pn)
{
	if (OW_w_mem(buf, size, offset + ((pn->extension) << 5), pn))
		return -EINVAL;
	return 0;
}

static int FS_r_mem(BYTE * buf, const size_t size, const off_t offset,
					const struct parsedname *pn)
{
	if (OW_read_paged(buf, size, offset, pn, 32, OW_r_mem))
		return -EINVAL;
	return size;
}

static int FS_w_mem(const BYTE * buf, const size_t size,
					const off_t offset, const struct parsedname *pn)
{
	if (OW_write_paged(buf, size, offset, pn, 32, OW_w_mem))
		return -EINVAL;
	return 0;
}

static int FS_counter(UINT * u, const struct parsedname *pn)
{
	if (OW_counter(u, (pn->extension) + 14, pn))
		return -EINVAL;
	return 0;
}

static int FS_pagecount(UINT * u, const struct parsedname *pn)
{
	if (OW_counter(u, pn->extension, pn))
		return -EINVAL;
	return 0;
}

#if OW_CACHE					/* Special code for cumulative counters -- read/write -- uses the caching system for storage */
/* Different from LCD system, counters are NOT reset with each read */
static int FS_r_mincount(UINT * u, const struct parsedname *pn)
{
	size_t s = 3 * sizeof(UINT);
	UINT st[3], ct[2];			// stored and current counter values

	if (OW_counter(&ct[0], 0, pn) || OW_counter(&ct[1], 1, pn))
		return -EINVAL;			// current counters
	if (Cache_Get_Internal_Strict((void *) st, 3 * sizeof(UINT), &ip_cum, pn)) {	// record doesn't (yet) exist
		st[2] = ct[0] < ct[1] ? ct[0] : ct[1];
	} else {
		UINT d0 = ct[0] - st[0];	//delta counter.A
		UINT d1 = ct[1] - st[1];	// delta counter.B
		st[2] += d0 < d1 ? d0 : d1;	// add minimum delta
	}
	st[0] = ct[0];
	st[1] = ct[1];
	u[0] = st[2];
	return Cache_Add_Internal((void *) st, s, &ip_cum, pn) ? -EINVAL : 0;
}

static int FS_w_mincount(const UINT * u, const struct parsedname *pn)
{
	UINT st[3];					// stored and current counter values

	if (OW_counter(&st[0], 0, pn) || OW_counter(&st[1], 1, pn))
		return -EINVAL;
	st[2] = u[0];
	return Cache_Add_Internal((void *) st, 3 * sizeof(UINT), &ip_cum,
							  pn) ? -EINVAL : 0;
}
#endif							/*OW_CACHE */

static int OW_w_mem(const BYTE * data, const size_t size,
					const off_t offset, const struct parsedname *pn)
{
	BYTE p[1 + 2 + 32 + 2] = { 0x0F, offset & 0xFF, offset >> 8, };
	struct transaction_log tcopy[] = {
		TRXN_START,
		{p, NULL, size + 3, trxn_match,},
		{NULL, &p[size + 3], 2, trxn_read,},
		{p, NULL, 1 + 2 + size + 2, trxn_crc16,},
		TRXN_END,
	};
	struct transaction_log treread[] = {
		TRXN_START,
		{p, NULL, 1, trxn_match,},
		{NULL, &p[1], 3 + size, trxn_read,},
		TRXN_END,
	};
	struct transaction_log twrite[] = {
		TRXN_START,
		{p, NULL, 4, trxn_match,},
		TRXN_END,
	};

	/* Copy to scratchpad */
	memcpy(&p[3], data, size);

	if (((offset + size) & 0x1F)) {	// doesn't end on page boundary, no crc16
		tcopy[2].type = tcopy[3].type = trxn_nop;
	}

	if (BUS_transaction(tcopy, pn))
		return 1;

	/* Re-read scratchpad and compare */
	/* Note that we tacitly shift the data one byte down for the E/S byte */
	p[0] = 0xAA;
	if (BUS_transaction(treread, pn) || memcmp(&p[4], data, size))
		return 1;

	/* Copy Scratchpad to SRAM */
	p[0] = 0x5A;
	if (BUS_transaction(twrite, pn))
		return 1;

	UT_delay(32);
	return 0;
}

static int OW_r_mem(BYTE * data, const size_t size, const off_t offset,
					const struct parsedname *pn)
{
	return OW_r_mem_counter(data, NULL, size, offset, pn);
}

static int OW_counter(UINT * counter, const int page,
					  const struct parsedname *pn)
{
	return OW_r_mem_counter(NULL, counter, 1, (page << 5) + 31, pn);
}

/* read memory area and counter (just past memory) */
/* Nathan Holmes help troubleshoot this one! */
static int OW_r_mem_counter(BYTE * p, UINT * counter, const size_t size,
							const off_t offset,
							const struct parsedname *pn)
{

	BYTE extra[8];
	if (OW_r_mem_p8_crc16(p, size, offset, pn, 32, extra))
		return 1;

	/* counter is held in the 4 bytes after the data */
	//if ( counter ) *counter = (((((((UINT) extra[3])<<8) | extra[2])<<8) | extra[1])<<8) | extra[0] ;
	if (counter)
		counter[0] = UT_uint32(extra);

	return 0;
}
