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
#include "ow_eds.h"

/* ------- Prototypes ----------- */

READ_FUNCTION(FS_r_page);
WRITE_FUNCTION(FS_w_page);
READ_FUNCTION(FS_r_status);
WRITE_FUNCTION(FS_w_status);
READ_FUNCTION(FS_r_memory);
WRITE_FUNCTION(FS_w_memory);

#define _EDS_WRITE_STATUS 0x00  //TODO set correct value here
#define _EDS_WRITE_SCRATCHPAD 0x0F
#define _EDS_READ_SCRATCHPAD 0xAA
#define _EDS_COPY_SCRATCHPAD 0x55
#define _EDS_READ_MEMMORYNO_CRC 0xF0
#define _EDS_READ_MEMORY_WITH_CRC 0xA5
#define _EDS_CLEAR_ALARMS 0x33

#define _EDS_PAGES 3
#define _EDS_PAGESIZE 32

/* ------- Structures ----------- */

struct aggregate AEDS = { _EDS_PAGES, ag_numbers, ag_separate, };
struct filetype EDS[] = {
	F_STANDARD,
	{"memory", _EDS_PAGES * _EDS_PAGESIZE, NON_AGGREGATE, ft_binary, fc_stable, FS_r_memory, FS_w_memory, VISIBLE, NO_FILETYPE_DATA,},
	{"pages", PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir, fc_volatile, NO_READ_FUNCTION, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA,},
	{"pages/page", _EDS_PAGESIZE, &AEDS, ft_binary, fc_stable, FS_r_page, FS_w_page, VISIBLE, NO_FILETYPE_DATA,},
};

DeviceEntryExtended(7E, EDS, DEV_alarm);

/* ------- Functions ------------ */
static int OW_w_mem(BYTE * data, size_t size, off_t offset, struct parsedname *pn) ;
static int OW_w_status(BYTE * data, size_t size, off_t offset, struct parsedname *pn);

/* 2505 memory */
static int FS_r_memory(struct one_wire_query *owq)
{
	size_t pagesize = 32;
//    if ( OW_r_mem( buf, size, (size_t) offset, pn) ) { return -EINVAL ; }
	if (COMMON_OWQ_readwrite_paged(owq, 0, pagesize, COMMON_read_memory_F0)) {
		return -EINVAL;
	}
	return 0;
}

static int FS_r_page(struct one_wire_query *owq)
{
	size_t pagesize = 32;
	if (COMMON_OWQ_readwrite_paged(owq, OWQ_pn(owq).extension, pagesize, COMMON_read_memory_F0)) {
		return -EINVAL;
	}
	return 0;
}

static int FS_r_status(struct one_wire_query *owq)
{
	size_t pagesize = FileLength(PN(owq)) ;
	if (COMMON_OWQ_readwrite_paged(owq, OWQ_pn(owq).extension, pagesize, COMMON_read_memory_crc16_AA)) {
		return -EINVAL;
	}
	return 0;
}

static int FS_w_memory(struct one_wire_query *owq)
{
	size_t size = OWQ_size(owq) ;
	off_t start = OWQ_offset(owq) ;
	BYTE * position = (BYTE *)OWQ_buffer(owq) ;
	size_t write_size = _EDS_PAGESIZE - (start % _EDS_PAGESIZE) ;
	
	while ( size > 0 ) {
		if ( write_size > size ) {
			write_size = size ;
		}
		if ( OW_w_mem(position, write_size, start, PN(owq))) {
			return -EINVAL ;
		}
		position += write_size ;
		start += write_size ;
		size -= write_size ;
		write_size = _EDS_PAGESIZE ;
	}
	return 0;
}

static int FS_w_status(struct one_wire_query *owq)
{
	if (OW_w_status(OWQ_explode(owq))) {
		return -EINVAL;
	}
	return 0;
}

static int FS_w_page(struct one_wire_query *owq)
{
	struct parsedname * pn = PN(owq) ;
	return OW_w_mem(OWQ_buffer(owq),OWQ_size(owq),OWQ_offset(owq) + _EDS_PAGESIZE * pn->extension,pn) ? -EINVAL : 0 ;
}

static int OW_w_status(BYTE * data, size_t size, off_t offset, struct parsedname *pn)
{
	BYTE p[6] = { _EDS_WRITE_STATUS, LOW_HIGH_ADDRESS(offset), data[0] };
	int ret = 0;
	struct transaction_log tfirst[] = {
		TRXN_START,
		TRXN_WR_CRC16(p, 4, 0),
		TRXN_PROGRAM,
		TRXN_READ1(p),
		TRXN_END,
	};

	if (size == 0) {
		return 0;
	}
	if (size == 1) {
		return BUS_transaction(tfirst, pn) || (p[0] & (~data[0]));
	}
	BUSLOCK(pn);
	if (BUS_transaction(tfirst, pn) || (p[0] & ~data[0])) {
		ret = 1;
	} else {
		size_t i;
		const BYTE *d = &data[1];
		UINT s = offset + 1;
		struct transaction_log trest[] = {
			//TRXN_WR_CRC16_SEEDED( p, &s, 1, 0 ) ,
			TRXN_WR_CRC16_SEEDED(p, p, 1, 0),
			TRXN_PROGRAM,
			TRXN_READ1(p),
			TRXN_END,
		};
		for (i = 0; i < size; ++i, ++d, ++s) {
			if (BUS_transaction(trest, pn) || (p[0] & ~d[0])) {
				ret = 1;
				break;
			}
		}
	}
	BUSUNLOCK(pn);
	return ret;
}

static int OW_w_mem(BYTE * data, size_t size, off_t offset, struct parsedname *pn)
{
	BYTE p[3 + 1 + _EDS_PAGESIZE + 2] = { _EDS_WRITE_SCRATCHPAD, LOW_HIGH_ADDRESS(offset), };
	int rest = _EDS_PAGESIZE - (offset & 0x1F);
	struct transaction_log tcopy[] = {
		TRXN_START,
		TRXN_WRITE(p, 3 + size),
		TRXN_END,
	};
	struct transaction_log tcopy_crc[] = {
		TRXN_START,
		TRXN_WR_CRC16(p, 3 + size, 0),
		TRXN_END,
	};
	struct transaction_log tread[] = {
		TRXN_START,
		TRXN_WR_CRC16(p, 3, 1 + rest),
		TRXN_COMPARE(&p[4], data, size),
		TRXN_END,
	};
	struct transaction_log twrite[] = {
		TRXN_START,
		TRXN_WRITE(p, 4),
		TRXN_END,
	};

	/* Copy to scratchpad -- use CRC16 if write to end of page, but don't force it */
	memcpy(&p[3], data, size);
	if ((offset + size) & 0x1F) {	/* to end of page */
		if (BUS_transaction(tcopy, pn)) {
			return 1;
		}
	} else {
		if (BUS_transaction(tcopy_crc, pn)) {
			return 1;
		}
	}

	/* Re-read scratchpad and compare */
	/* Note: location of data has now shifted down a byte for E/S register */
	p[0] = _EDS_READ_SCRATCHPAD;
	if (BUS_transaction(tread, pn)) {
		return 1;
	}

	/* write Scratchpad to SRAM */
	p[0] = _EDS_COPY_SCRATCHPAD;
	if (BUS_transaction(twrite, pn)) {
		return 1;
	}

	return 0;
}

