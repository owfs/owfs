/*
$Id$
    OWFS -- One-Wire filesystem
    OWHTTPD -- One-Wire Web Server
    Written 2003 Paul H Alfille
    email: paul.alfille@gmail.com
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
#include "ow_1963.h"

/* ------- Prototypes ----------- */

READ_FUNCTION(FS_r_page);
WRITE_FUNCTION(FS_w_page);
READ_FUNCTION(FS_r_mem);
WRITE_FUNCTION(FS_w_mem);
WRITE_FUNCTION(FS_w_password);
READ_FUNCTION(FS_counter);

/* ------- Structures ----------- */

static struct aggregate A1963S = { 16, ag_numbers, ag_separate, };
static struct filetype DS1963S[] = {
	F_STANDARD,
	{"memory", 512, NON_AGGREGATE, ft_binary, fc_link, FS_r_mem, FS_w_mem, VISIBLE, NO_FILETYPE_DATA, },
	{"pages", PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir, fc_subdir, NO_READ_FUNCTION, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA, },
	{"pages/page", 32, &A1963S, ft_binary, fc_page, FS_r_page, FS_w_page, VISIBLE, NO_FILETYPE_DATA, },
	{"pages/count", PROPERTY_LENGTH_UNSIGNED, &A1963S, ft_unsigned, fc_volatile, FS_counter, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA, },
	{"pages/password", 8, NON_AGGREGATE, ft_binary, fc_stable, NO_READ_FUNCTION, FS_w_password, VISIBLE, NO_FILETYPE_DATA, },

	{"password", 8, NON_AGGREGATE, ft_binary, fc_stable, NO_READ_FUNCTION, FS_w_password, VISIBLE, NO_FILETYPE_DATA, },
};

DeviceEntryExtended(18, DS1963S, DEV_resume | DEV_ovdr, NO_GENERIC_READ, NO_GENERIC_WRITE);

static struct aggregate A1963L = { 16, ag_numbers, ag_separate, };
static struct filetype DS1963L[] = {
	F_STANDARD,
	{"memory", 512, NON_AGGREGATE, ft_binary, fc_link, FS_r_mem, FS_w_mem, VISIBLE, NO_FILETYPE_DATA, },
	{"pages", PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir, fc_subdir, NO_READ_FUNCTION, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA, },
	{"pages/page", 32, &A1963L, ft_binary, fc_page, FS_r_page, FS_w_page, VISIBLE, NO_FILETYPE_DATA, },
	{"pages/count", PROPERTY_LENGTH_UNSIGNED, &A1963L, ft_unsigned, fc_volatile, FS_counter, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA, },
};

DeviceEntryExtended(1A, DS1963L, DEV_ovdr, NO_GENERIC_READ, NO_GENERIC_WRITE);

#define _1W_WRITE_SCRATCHPAD 0x0F
#define _1W_READ_SCRATCHPAD 0xAA
#define _1W_COPY_SCRATCHPAD 0x5A
#define _1W_READ_MEMORY 0xF0
#define _1W_READ_MEMORY_PLUS_COUNTER 0xA5

#define _1W_COUNTER_FILL 0x55

/* ------- Functions ------------ */

static GOOD_OR_BAD OW_w_mem(BYTE * data, size_t size, off_t offset, struct parsedname *pn);
static GOOD_OR_BAD OW_r_counter(struct one_wire_query *owq, size_t page, size_t pagesize);

static ZERO_OR_ERROR FS_w_password(struct one_wire_query *owq)
{
	(void) owq;
	return -EINVAL;
}

static ZERO_OR_ERROR FS_r_page(struct one_wire_query *owq)
{
	size_t pagesize = 32;
	return COMMON_offset_process( FS_r_mem, owq, OWQ_pn(owq).extension*pagesize) ;
}

static ZERO_OR_ERROR FS_r_mem(struct one_wire_query *owq)
{
	/* read is not page-limited */
	size_t pagesize = 32;
	return GB_to_Z_OR_E(COMMON_OWQ_readwrite_paged(owq, 0, pagesize, COMMON_read_memory_toss_counter)) ;
}

static ZERO_OR_ERROR FS_counter(struct one_wire_query *owq)
{
	size_t pagesize = 32;
	return GB_to_Z_OR_E(OW_r_counter(owq, OWQ_pn(owq).extension, pagesize)) ;
}

static ZERO_OR_ERROR FS_w_page(struct one_wire_query *owq)
{
	size_t pagesize = 32;
	return COMMON_offset_process( FS_w_mem, owq, OWQ_pn(owq).extension*pagesize) ;
}

static ZERO_OR_ERROR FS_w_mem(struct one_wire_query *owq)
{
	size_t pagesize = 32;
	return GB_to_Z_OR_E(COMMON_readwrite_paged(owq, 0, pagesize, OW_w_mem)) ;
}

/* paged, and pre-screened */
static GOOD_OR_BAD OW_w_mem(BYTE * data, size_t size, off_t offset, struct parsedname *pn)
{
	BYTE p[1 + 2 + 32 + 2] = { _1W_WRITE_SCRATCHPAD, LOW_HIGH_ADDRESS(offset), };
	struct transaction_log tcopy[] = {
		TRXN_START,
		TRXN_WRITE(p, 3 + size),
		TRXN_END,
	};
	struct transaction_log tcopy_crc16[] = {
		TRXN_START,
		TRXN_WR_CRC16(p, 3 + size, 0),
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
	memcpy(&p[3], data, size);

	RETURN_BAD_IF_BAD(BUS_transaction(((offset + size) & 0x1F) != 0 ? tcopy : tcopy_crc16, pn)) ;

	/* Re-read scratchpad and compare */
	/* Note that we tacitly shift the data one byte down for the E/S byte */
	p[0] = _1W_READ_SCRATCHPAD;
	RETURN_BAD_IF_BAD(BUS_transaction(tread, pn)) ;

	/* Copy Scratchpad to SRAM */
	p[0] = _1W_COPY_SCRATCHPAD;
	return BUS_transaction(tsram, pn) ;
}

/* read counter (just past memory) */
/* Nathan Holmes helped troubleshoot this one! */
static GOOD_OR_BAD OW_r_counter(struct one_wire_query *owq, size_t page, size_t pagesize)
{
	BYTE extra[8];
	if (COMMON_read_memory_plus_counter(extra, page, pagesize, PN(owq))) {
		return gbBAD;
	}
	if (extra[4] != _1W_COUNTER_FILL || extra[5] != _1W_COUNTER_FILL || extra[6] != _1W_COUNTER_FILL || extra[7] != _1W_COUNTER_FILL) {
		return gbBAD;
	}
	/* counter is held in the 4 bytes after the data */
	OWQ_U(owq) = UT_uint32(extra);
	return gbGOOD;
}
