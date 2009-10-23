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
WRITE_FUNCTION(FS_w_extended);
WRITE_FUNCTION(FS_writebyte);

/* ------- Structures ----------- */

struct aggregate Abae = { 8, ag_numbers, ag_separate, };
struct filetype BAE[] = {
	F_STANDARD,
  {"memory", 64, NULL, ft_binary, fc_stable, FS_r_mem, FS_w_mem, NO_FILETYPE_DATA,},
  {"pages", PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir, fc_volatile, NO_READ_FUNCTION, NO_WRITE_FUNCTION, NO_FILETYPE_DATA,},
  {"pages/page", 8, &Abae, ft_binary, fc_stable, FS_r_page, FS_w_page, NO_FILETYPE_DATA,},
  {"command", 32, NULL, ft_binary, fc_stable, NO_READ_FUNCTION, FS_w_extended, NO_FILETYPE_DATA,},
  {"writebyte", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_stable, FS_writebyte, NO_WRITE_FUNCTION, NO_FILETYPE_DATA, },
};

DeviceEntryExtended(FC, BAE, DEV_resume | DEV_alarm );

#define _1W_ERASE_FIRMWARE 0xBB
#define _1W_FLASH_FIRMWARE 0xBA

#define _1W_READ_MEMORY 0xAA

#define _1W_READ_VERSION 0x11
#define _1W_READ_TYPE 0x12
#define _1W_EXTENDED_COMMAND 0x13
#define _1W_READ_BLOCK_WITH_LEN 0x14
#define _1W_WRITE_BLOCK_WITH_LEN 0x15

#define _1W_CONFIRM_WRITE 0xBC

/* Note, read and write page sizes are differnt -- 32 bytes for write and no page boundary. 8 Bytes for read */

/* ------- Functions ------------ */

static int OW_w_mem(BYTE * data, size_t size, off_t offset, struct parsedname *pn);
static int OW_w_mem_with_code(BYTE code, BYTE * data, size_t size, off_t offset, struct parsedname *pn);
static int BAE_r_memory_crc16_14(struct one_wire_query *owq, size_t page, size_t pagesize) ;

/* 8 byte pages with CRC */
static int FS_r_page(struct one_wire_query *owq)
{
	size_t pagesize = 8;
	if (COMMON_OWQ_readwrite_paged(owq, OWQ_pn(owq).extension, pagesize, BAE_r_memory_crc16_14)) {
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
	if (COMMON_OWQ_readwrite_paged(owq, 0, pagesize, BAE_r_memory_crc16_14)) {
		return -EINVAL;
	}
	return 0;
}

static int FS_w_mem(struct one_wire_query *owq)
{
	size_t pagesize = 32; // different from read page size
	size_t remain = OWQ_size(owq) ;
	BYTE * data = (BYTE *) OWQ_buffer(owq) ;
	off_t location = OWQ_offset(owq) ;
	
	// Write data 32 bytes at a time ignoring page boundaries
	while ( remain > 0 ) {
		size_t bolus = remain ;
		if ( bolus > pagesize ) {
			bolus = pagesize ;
		}
		if ( OW_w_mem(data, bolus, location, PN(owq)  ) ) {
			return -EINVAL ;
		}
		remain -= bolus ;
		data += bolus ;
		location += bolus ;
	}
	
	return 0;
}

static int FS_w_extended(struct one_wire_query *owq)
{
	// Write data 32 bytes maximum
	if ( OW_w_mem_with_code(_1W_EXTENDED_COMMAND, (BYTE *) OWQ_buffer(owq), OWQ_size(owq), OWQ_offset(owq), PN(owq)  ) ) {
		return -EINVAL ;
	}
	
	return 0;
}

static int FS_writebyte(struct one_wire_query *owq)
{
	off_t location = OWQ_U(owq)>>8 ;
	BYTE data = OWQ_U(owq) & 0xFF ;
	
	// Write 1 byte ,
	if ( OW_w_mem_with_code(_1W_WRITE_BLOCK_WITH_LEN, &data, 1, location, PN(owq)  ) ) {
		return -EINVAL ;
	}
	
	return 0;
}

static int OW_w_mem(BYTE * data, size_t size, off_t offset, struct parsedname *pn)
{
	return OW_w_mem_with_code(_1W_WRITE_BLOCK_WITH_LEN, data, size, offset, pn) ;
}

/* Used for both memory and extended command writes */
static int OW_w_mem_with_code(BYTE code, BYTE * data, size_t size, off_t offset, struct parsedname *pn)
{
	BYTE p[1 + 2 + 1 + 32 + 2] = { code, LOW_HIGH_ADDRESS(offset), BYTE_MASK(size), };
	BYTE q[] = { _1W_CONFIRM_WRITE, } ;
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
	return BUS_transaction(tcommit, pn) ;
}

/* read up to LEN of page to CRC16 -- 0x14 BAE code*/
static int BAE_r_memory_crc16_14(struct one_wire_query *owq, size_t page, size_t pagesize)
{
	off_t offset = OWQ_offset(owq) + page * pagesize;
	size_t size = OWQ_size(owq);
	BYTE p[1+2+1 + pagesize + 2] ;
	struct transaction_log t[] = {
		TRXN_START,
		TRXN_WR_CRC16(p, 4, size),
		TRXN_END,
	};
	
	p[0] = _1W_READ_BLOCK_WITH_LEN ;
	p[1] =  BYTE_MASK(offset) ;
	p[2] = BYTE_MASK(offset>>8) ; ;
	p[3] = BYTE_MASK(size) ;

	if (BUS_transaction(t, PN(owq))) {
		return 1;
	}
	memcpy(OWQ_buffer(owq), &p[4], size);
	OWQ_length(owq) = OWQ_size(owq);
	return 0;
}

