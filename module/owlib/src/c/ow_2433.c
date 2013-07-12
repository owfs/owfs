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
#include "ow_2433.h"

/* ------- Prototypes ----------- */

/* DS2433 EEPROM */
READ_FUNCTION(FS_r_page);
WRITE_FUNCTION(FS_w_page);
READ_FUNCTION(FS_r_mem);
WRITE_FUNCTION(FS_w_mem);

 /* ------- Structures ----------- */

static struct aggregate A2431 = { 4, ag_numbers, ag_separate, };
static struct filetype DS2431[] = {
	F_STANDARD,
	{"memory", 128, NON_AGGREGATE, ft_binary, fc_link, FS_r_mem, FS_w_mem, VISIBLE, NO_FILETYPE_DATA, },
	{"pages", PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir, fc_subdir, NO_READ_FUNCTION, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA, },
	{"pages/page", 32, &A2431, ft_binary, fc_page, FS_r_page, FS_w_page, VISIBLE, NO_FILETYPE_DATA, },
};

DeviceEntryExtended(2D, DS2431, DEV_ovdr | DEV_resume, NO_GENERIC_READ, NO_GENERIC_WRITE);

static struct aggregate A2433 = { 16, ag_numbers, ag_separate, };
static struct filetype DS2433[] = {
	F_STANDARD,
	{"memory", 512, NON_AGGREGATE, ft_binary, fc_link, FS_r_mem, FS_w_mem, VISIBLE, NO_FILETYPE_DATA, },
	{"pages", PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir, fc_subdir, NO_READ_FUNCTION, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA, },
	{"pages/page", 32, &A2433, ft_binary, fc_page, FS_r_page, FS_w_page, VISIBLE, NO_FILETYPE_DATA, },
};

DeviceEntryExtended(23, DS2433, DEV_ovdr, NO_GENERIC_READ, NO_GENERIC_WRITE);

static struct aggregate A28EC20 = { 80, ag_numbers, ag_separate, };
static struct filetype DS28EC20[] = {
	F_STANDARD,
	{"memory", 2560, NON_AGGREGATE, ft_binary, fc_link, FS_r_mem, FS_w_mem, VISIBLE, NO_FILETYPE_DATA, },
	{"pages", PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir, fc_subdir, NO_READ_FUNCTION, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA, },
	{"pages/page", 32, &A28EC20, ft_binary, fc_page, FS_r_page, FS_w_page, VISIBLE, NO_FILETYPE_DATA, },
};

DeviceEntryExtended(43, DS28EC20, DEV_ovdr | DEV_resume, NO_GENERIC_READ, NO_GENERIC_WRITE);

#define _1W_WRITE_SCRATCHPAD 0x0F
#define _1W_READ_SCRATCHPAD 0xAA
#define _1W_COPY_SCRATCHPAD 0x55
#define _1W_READ_MEMORY 0xF0
#define _1W_EXTENDED_READ_MEMORY 0xA5

/* ------- Functions ------------ */

/* DS2433 */

static GOOD_OR_BAD OW_w_23page(BYTE * data, size_t size, off_t offset, struct parsedname *pn);
static GOOD_OR_BAD OW_w_2Dpage(BYTE * data, size_t size, off_t offset, struct parsedname *pn);

static ZERO_OR_ERROR FS_r_mem(struct one_wire_query *owq)
{
    size_t pagesize = 32;
    /* read is not page-limited */
    if (COMMON_read_memory_F0(owq, 0, pagesize)) {
		return -EINVAL;
	}
	return 0;
}

static ZERO_OR_ERROR FS_w_mem(struct one_wire_query *owq)
{
	/* paged access */
	size_t pagesize;
	switch (PN(owq)->sn[0]) {
		case 0x2D:
			pagesize = 8 ;
			return GB_to_Z_OR_E(COMMON_readwrite_paged(owq, 0, pagesize, OW_w_2Dpage)) ;
		default:
			pagesize = 8 ;
			return GB_to_Z_OR_E(COMMON_readwrite_paged(owq, 0, pagesize, OW_w_23page)) ;
	}
}

static ZERO_OR_ERROR FS_r_page(struct one_wire_query *owq)
{
	size_t pagesize = 32;
	if (COMMON_read_memory_F0(owq, OWQ_pn(owq).extension, pagesize)) {
		return -EINVAL;
	}
	return 0;
}

static ZERO_OR_ERROR FS_w_page(struct one_wire_query *owq)
{
	/* paged access */
	size_t pagesize = 32;
	return COMMON_offset_process( FS_w_mem, owq, OWQ_pn(owq).extension*pagesize) ;
}

/* paged, and pre-screened */
static GOOD_OR_BAD OW_w_23page(BYTE * data, size_t size, off_t offset, struct parsedname *pn)
{
	BYTE p[1 + 2 + 32 + 2] = { _1W_WRITE_SCRATCHPAD, LOW_HIGH_ADDRESS(offset), };
	struct transaction_log tcopy[] = {
		TRXN_START,
		TRXN_WR_CRC16(p, 3 + size, 0),
		TRXN_END,
	};
	struct transaction_log treread[] = {
		TRXN_START,
		TRXN_WRITE1(p),
		TRXN_READ(&p[1], 3 + size),
		TRXN_COMPARE(&p[4], data, size),
		TRXN_END,
	};
	struct transaction_log twrite33[] = {
		TRXN_START,
		TRXN_WRITE(p, 4),
		TRXN_DELAY(5),
		TRXN_END,
	};
	struct transaction_log twriteEC20[] = {
		TRXN_START,
		TRXN_WRITE(p, 4),
		TRXN_DELAY(10),
		TRXN_END,
	};

	/* Copy to scratchpad */
	memcpy(&p[3], data, size);

	if (((offset + size) & 0x1F)) {	// doesn't end on page boundary, no crc16
		tcopy[2].type = tcopy[3].type = trxn_nop;
	}

	RETURN_BAD_IF_BAD(BUS_transaction(tcopy, pn)) ;

	/* Re-read scratchpad and compare */
	/* Note that we tacitly shift the data one byte down for the E/S byte */
	p[0] = _1W_READ_SCRATCHPAD;
	RETURN_BAD_IF_BAD(BUS_transaction(treread, pn)) ;

	/* Copy Scratchpad to SRAM */
	p[0] = _1W_COPY_SCRATCHPAD;
	switch (pn->sn[0]) {
	case 0x23:					// DS2433
		return BUS_transaction(twrite33,pn);
	case 0x43:
	default:					// DS28EC20
		return BUS_transaction(twriteEC20,pn);
	}
}

/* paged, and pre-screened */
/* read REAL DS2431 pages -- 8 bytes. */
static GOOD_OR_BAD OW_w_2Dpage(BYTE * data, size_t size, off_t offset, struct parsedname *pn)
{
	off_t pageoff = offset & 0x07;
	BYTE p[4 + 8 + 2] = { _1W_WRITE_SCRATCHPAD, LOW_HIGH_ADDRESS(offset - pageoff),
	};
	struct transaction_log tcopy[] = {
		TRXN_START,
		TRXN_WRITE(p, 3 + 8),
		TRXN_END,
	};
	struct transaction_log tread[] = {
		TRXN_START,
		TRXN_WR_CRC16(p, 1, 3 + 8),
		TRXN_COMPARE(&p[4], data, size),
		TRXN_END,
	};
	struct transaction_log tsram[] = {
		TRXN_START,
		TRXN_WRITE(p, 4),
		TRXN_DELAY(13),
		TRXN_END,
	};

	if (size != 8) {			// incomplete page
		OWQ_allocate_struct_and_pointer(owq_old);
		OWQ_create_temporary(owq_old, (char *) &p[3], 8, offset - pageoff, pn);
		if (COMMON_read_memory_F0(owq_old, 0, 0)) {
			return gbBAD;
		}
	}

	memcpy(&p[3 + pageoff], data, size);

	/* Copy to scratchpad */
	RETURN_BAD_IF_BAD(BUS_transaction(tcopy, pn)) ;

	/* Re-read scratchpad and compare */
	p[0] = _1W_READ_SCRATCHPAD;
	RETURN_BAD_IF_BAD(BUS_transaction(tread, pn)) ;

	/* Copy Scratchpad to SRAM */
	p[0] = _1W_COPY_SCRATCHPAD;
	return BUS_transaction(tsram, pn) ;
}
