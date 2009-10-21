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

/* Pascal Baerten's BAE device -- preliminary */

#include <config.h>
#include "owfs_config.h"
#include "ow_bae.h"

/* ------- Prototypes ----------- */

/* BAE */
READ_FUNCTION(FS_r_mem);
WRITE_FUNCTION(FS_w_mem);
READ_FUNCTION(FS_r_page);
WRITE_FUNCTION(FS_w_page);
//WRITE_FUNCTION(FS_writebyte);

/* ------- Structures ----------- */

struct aggregate Abae = { 11, ag_numbers, ag_separate, };
struct filetype BAE[] = {
	F_STANDARD,
  {"memory", 88, NULL, ft_binary, fc_stable, FS_r_mem, FS_w_mem, NO_FILETYPE_DATA,},
  {"pages", PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir, fc_volatile, NO_READ_FUNCTION, NO_WRITE_FUNCTION, NO_FILETYPE_DATA,},
  {"pages/page", 8, &Abae, ft_binary, fc_stable, FS_r_page, FS_w_page, NO_FILETYPE_DATA,},
//  {"writebyte", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_stable, FS_writebyte, NO_WRITE_FUNCTION, NO_FILETYPE_DATA, },
};

DeviceEntryExtended(FC, BAE, DEV_resume | DEV_alarm );

#define _1W_ERASE_FIRMWARE 0xBB
#define _1W_FLASH_FIRMWARE 0xBA

#define _1W_READ_MEMORY 0xAA

#define _1W_WRITE_SCRATCHPAD 0x55
#define _1W_COPY_SCRATCHPAD 0xBC
#define _1W_EXTENDED_COMMAND 0x13

/* Note, read and write page sizes are differnt -- 32 bytes for write and no page boundary. 8 Bytes for read */

/* ------- Functions ------------ */

/* DS2423 */
static int OW_w_mem(BYTE * data, size_t size, off_t offset, struct parsedname *pn);

/* 2423A/D Counter */
static int FS_r_page(struct one_wire_query *owq)
{
	size_t pagesize = 8;
	if (COMMON_OWQ_readwrite_paged(owq, OWQ_pn(owq).extension, pagesize, COMMON_read_memory_crc16_AA)) {
		return -EINVAL;
	}
	return 0;
}

static int FS_w_page(struct one_wire_query *owq)
{
	size_t pagesize = 8;
	if (COMMON_readwrite_paged(owq, OWQ_pn(owq).extension, pagesize, OW_w_mem)) {
		return -EINVAL;
	}
	return 0;
}

static int FS_r_mem(struct one_wire_query *owq)
{
	size_t pagesize = 8;
	if (COMMON_OWQ_readwrite_paged(owq, 0, pagesize, COMMON_read_memory_crc16_AA)) {
		return -EINVAL;
	}
	return 0;
}

static int FS_w_mem(struct one_wire_query *owq)
{
	size_t pagesize = 32; // different from read page size
	size_t remain = OWQ_size(owq) ;
	BYTE * data = OWQ_buffer(owq) ;
	off_t location = OWQ_offset(owq) ;

	// Write data 32 bytes at a time ignoring page boundaries
	while ( remain > 0 ) {
		size_t bolus = remain ;
		if ( bolus > pagesize ) {
			bolus = pagesize ;
		}
		if ( OW_w_mem(data, bolus, location, OWQ_PN(owq) ) {
			return -EINVAL ;
		}
		remain -= bolus ;
		data += bolus ;
		location += bolus ;
	}
	
	return 0;
}

static int OW_w_mem(BYTE * data, size_t size, off_t offset, struct parsedname *pn)
{
	BYTE p[1 + 2 + 1 + 32 + 2] = { _1W_WRITE_SCRATCHPAD, LOW_HIGH_ADDRESS(offset), BYTE_MASK(size), };
	BYTE q[] = { _1W_COPY_SCRATCHPAD, } ;
	struct transaction_log tcopy_crc[] = {
		TRXN_START,
		TRXN_WR_CRC16(p, 1+ 2 + 1 + size, 0),
		TRXN_END,
	};
	struct transaction_log tcommit[] = {
		TRXN_START,
		TRXN_WRITE1(q),
		TRXN_END,
	};

	/* Copy to scratchpad */
	memcpy(&p[3], data, size);

	if (BUS_transaction(tcopy_crc, pn)) {
		return 1;
	}

	/* Copy Scratchpad to SRAM */
	return BUS_transaction(tcommit, pn)) {
}

