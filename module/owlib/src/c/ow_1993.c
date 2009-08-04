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
#include "ow_1993.h"

/* ------- Prototypes ----------- */

/* DS1902 ibutton memory */
READ_FUNCTION(FS_r_page);
WRITE_FUNCTION(FS_w_page);
READ_FUNCTION(FS_r_memory);
WRITE_FUNCTION(FS_w_memory);

/* ------- Structures ----------- */

struct aggregate A1992 = { 4, ag_numbers, ag_separate, };
struct filetype DS1992[] = {
	F_STANDARD,
  {"pages", PROPERTY_LENGTH_SUBDIR, NULL, ft_subdir, fc_volatile, NO_READ_FUNCTION, NO_WRITE_FUNCTION, {v:NULL},},
  {"pages/page", 32, &A1992, ft_binary, fc_stable, FS_r_page, FS_w_page, {v:NULL},},
  {"memory", 128, NULL, ft_binary, fc_stable, FS_r_memory, FS_w_memory, {v:NULL},},
};

DeviceEntry(08, DS1992);

struct aggregate A1993 = { 16, ag_numbers, ag_separate, };
struct filetype DS1993[] = {
	F_STANDARD,
  {"pages", PROPERTY_LENGTH_SUBDIR, NULL, ft_subdir, fc_volatile, NO_READ_FUNCTION, NO_WRITE_FUNCTION, {v:NULL},},
  {"pages/page", 32, &A1993, ft_binary, fc_stable, FS_r_page, FS_w_page, {v:NULL},},
  {"memory", 512, NULL, ft_binary, fc_stable, FS_r_memory, FS_w_memory, {v:NULL},},
};

DeviceEntry(06, DS1993);

struct aggregate A1995 = { 64, ag_numbers, ag_separate, };
struct filetype DS1995[] = {
	F_STANDARD,
  {"pages", PROPERTY_LENGTH_SUBDIR, NULL, ft_subdir, fc_volatile, NO_READ_FUNCTION, NO_WRITE_FUNCTION, {v:NULL},},
  {"pages/page", 32, &A1995, ft_binary, fc_stable, FS_r_page, FS_w_page, {v:NULL},},
  {"memory", 2048, NULL, ft_binary, fc_stable, FS_r_memory, FS_w_memory, {v:NULL},},
};

DeviceEntryExtended(0A, DS1995, DEV_ovdr);

struct aggregate A1996 = { 256, ag_numbers, ag_separate, };
struct filetype DS1996[] = {
	F_STANDARD,
  {"pages", PROPERTY_LENGTH_SUBDIR, NULL, ft_subdir, fc_volatile, NO_READ_FUNCTION, NO_WRITE_FUNCTION, {v:NULL},},
  {"pages/page", 32, &A1996, ft_binary, fc_stable, FS_r_page, FS_w_page, {v:NULL},},
  {"memory", 8192, NULL, ft_binary, fc_stable, FS_r_memory, FS_w_memory, {v:NULL},},
};

DeviceEntryExtended(0C, DS1996, DEV_ovdr);

#define _1W_WRITE_SCRATCHPAD 0x0F
#define _1W_READ_SCRATCHPAD 0xAA
#define _1W_COPY_SCRATCHPAD 0x55
#define _1W_READ_MEMORY 0xF0

/* ------- Functions ------------ */

/* DS1902 */
static int OW_w_mem(BYTE * data, size_t size, off_t offset, struct parsedname *pn);

/* 1902 */
static int FS_r_page(struct one_wire_query *owq)
{
	size_t pagesize = 32;
	if (COMMON_read_memory_F0(owq, OWQ_pn(owq).extension, pagesize)) {
		return -EINVAL;
	}
	return OWQ_size(owq);
}

static int FS_r_memory(struct one_wire_query *owq)
{
	/* read is not page-limited */
	if (COMMON_read_memory_F0(owq, 0, 0)) {
		return -EINVAL;
	}
	return OWQ_size(owq);
}

static int FS_w_page(struct one_wire_query *owq)
{
	size_t pagesize = 32;
	if (COMMON_readwrite_paged(owq, OWQ_pn(owq).extension, pagesize, OW_w_mem)) {
		return -EFAULT;
	}
	return 0;
}

static int FS_w_memory(struct one_wire_query *owq)
{
	/* paged access */
	size_t pagesize = 32;
	if (COMMON_readwrite_paged(owq, 0, pagesize, OW_w_mem)) {
		return -EFAULT;
	}
	return 0;
}

/* paged, and pre-screened */
static int OW_w_mem(BYTE * data, size_t size, off_t offset, struct parsedname *pn)
{
	BYTE p[4 + 32] = { _1W_WRITE_SCRATCHPAD, LOW_HIGH_ADDRESS(offset), };
	struct transaction_log tcopy[] = {
		TRXN_START,
		TRXN_WRITE3(p),
		TRXN_WRITE(data, size),
		TRXN_END,
	};
	struct transaction_log tread[] = {
		TRXN_START,
		TRXN_WRITE1(p),
		TRXN_READ(&p[1], 3 + size),
		TRXN_COMPARE(data, &p[4], size),
		TRXN_END,
	};
	struct transaction_log tsram[] = {
		TRXN_START,
		TRXN_WRITE(p, 4),
		TRXN_END,
	};

	/* Copy to scratchpad */
	if (BUS_transaction(tcopy, pn)) {
		return 1;
	}

	/* Re-read scratchpad and compare */
	p[0] = _1W_READ_SCRATCHPAD;
	if (BUS_transaction(tread, pn)) {
		return 1;
	}

	/* Copy Scratchpad to SRAM */
	p[0] = _1W_COPY_SCRATCHPAD;
	if (BUS_transaction(tsram, pn)) {
		return 1;
	}

	UT_delay(32);
	return 0;
}
