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
READ_FUNCTION(FS_version_state) ;
READ_FUNCTION(FS_version) ;
READ_FUNCTION(FS_version_device) ;
READ_FUNCTION(FS_version_bootstrap) ;
READ_FUNCTION(FS_type_state) ;
READ_FUNCTION(FS_localtype) ;
READ_FUNCTION(FS_type_device) ;
READ_FUNCTION(FS_type_chip) ;

/* ------- Structures ----------- */

struct aggregate Abae = { 8, ag_numbers, ag_separate, };
struct filetype BAE[] = {
	F_STANDARD,
  {"memory", 64, NULL, ft_binary, fc_stable, FS_r_mem, FS_w_mem, NO_FILETYPE_DATA,},
  {"pages", PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir, fc_volatile, NO_READ_FUNCTION, NO_WRITE_FUNCTION, NO_FILETYPE_DATA,},
  {"pages/page", 8, &Abae, ft_binary, fc_stable, FS_r_page, FS_w_page, NO_FILETYPE_DATA,},
  {"command", 32, NULL, ft_binary, fc_stable, NO_READ_FUNCTION, FS_w_extended, NO_FILETYPE_DATA,},
  {"writebyte", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_stable, NO_READ_FUNCTION, FS_writebyte, NO_FILETYPE_DATA, },
  {"versionstate", PROPERTY_LENGTH_HIDDEN, NON_AGGREGATE, ft_unsigned, fc_volatile, FS_version_state, NO_WRITE_FUNCTION, NO_FILETYPE_DATA, },
  {"version", 5, NON_AGGREGATE, ft_ascii, fc_link, FS_version, NO_WRITE_FUNCTION, NO_FILETYPE_DATA,},
  {"device_version", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_link, FS_version_device, NO_WRITE_FUNCTION, NO_FILETYPE_DATA,},
  {"bootstrap_version", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_link, FS_version_bootstrap, NO_WRITE_FUNCTION, NO_FILETYPE_DATA,},
  {"typestate", PROPERTY_LENGTH_HIDDEN, NON_AGGREGATE, ft_unsigned, fc_volatile, FS_type_state, NO_WRITE_FUNCTION, NO_FILETYPE_DATA, },
  {"localtype", 5, NON_AGGREGATE, ft_ascii, fc_link, FS_localtype, NO_WRITE_FUNCTION, NO_FILETYPE_DATA,},
  {"device_type", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_link, FS_type_device, NO_WRITE_FUNCTION, NO_FILETYPE_DATA,},
  {"chip_type", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_link, FS_type_chip, NO_WRITE_FUNCTION, NO_FILETYPE_DATA,},
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
static int OW_w_extended(BYTE * data, size_t size, UINT * return_code, struct parsedname *pn);
static int OW_version( UINT * version, struct parsedname * pn ) ;
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
	UINT ret ;
	// Write data 32 bytes maximum
	if ( OW_w_extended( (BYTE *) OWQ_buffer(owq), OWQ_size(owq), &ret, PN(owq)  ) ) {
		return -EINVAL ;
	}

	if ( ret == 0xFFFF ) {
		LEVEL_DEBUG("Bad return code for extended command\n") ;
		return -EINVAL ;
	}
	
	return 0;
}

static int FS_writebyte(struct one_wire_query *owq)
{
	off_t location = OWQ_U(owq)>>8 ;
	BYTE data = OWQ_U(owq) & 0xFF ;
	
	// Write 1 byte ,
	if ( OW_w_mem( &data, 1, location, PN(owq)  ) ) {
		return -EINVAL ;
	}
	
	return 0;
}

static int FS_version_state(struct one_wire_query *owq)
{
	UINT v ;
	if ( OW_version( &v, PN(owq) ) ) {
		return -EINVAL ;
	}
	OWQ_U(owq) = v ;
	return 0 ;
}

static int FS_version(struct one_wire_query *owq)
{
	char v[6];
	UINT version ;
	
	if ( FS_r_sibling_U( &version, "versionstate", owq ) ) {
		return -EINVAL ;
	}
	
	UCLIBCLOCK;
	snprintf(v,6,"%.2X.%.2X",version&0xFF, (version>>8)&0xFF);
	UCLIBCUNLOCK;
	
	return Fowq_output_offset_and_size(v, 5, owq);
}

static int FS_version_device(struct one_wire_query *owq)
{
	UINT version ;
	
	if ( FS_r_sibling_U( &version, "versionstate", owq ) ) {
		return -EINVAL ;
	}
	
	OWQ_U(owq) = version & 0xFF ;
	
	return 0 ;
}

static int FS_version_bootstrap(struct one_wire_query *owq)
{
	UINT version ;
	
	if ( FS_r_sibling_U( &version, "versionstate", owq ) ) {
		return -EINVAL ;
	}
	
	OWQ_U(owq) = (version>>8) & 0xFF ;
	
	return 0 ;
}

static int FS_type_state(struct one_wire_query *owq)
{
	UINT t ;
	if ( OW_version( &t, PN(owq) ) ) {
		return -EINVAL ;
	}
	OWQ_U(owq) = t ;
	return 0 ;
}

static int FS_localtype(struct one_wire_query *owq)
{
	char t[6];
	UINT version ;
	
	if ( FS_r_sibling_U( &version, "versionstate", owq ) ) {
		return -EINVAL ;
	}
	
	UCLIBCLOCK;
	snprintf(t,6,"%.2X.%.2X",version&0xFF, (version>>8)&0xFF);
	UCLIBCUNLOCK;
	
	return Fowq_output_offset_and_size(t, 5, owq);
}

static int FS_type_device(struct one_wire_query *owq)
{
	UINT t ;
	
	if ( FS_r_sibling_U( &t, "typestate", owq ) ) {
		return -EINVAL ;
	}
	
	OWQ_U(owq) = t & 0xFF ;
	
	return 0 ;
}

static int FS_type_chip(struct one_wire_query *owq)
{
	UINT t ;
	
	if ( FS_r_sibling_U( &t, "typestate", owq ) ) {
		return -EINVAL ;
	}
	
	OWQ_U(owq) = (t>>8) & 0xFF ;
	
	return 0 ;
}

static int OW_w_mem(BYTE * data, size_t size, off_t offset, struct parsedname *pn)
{
	BYTE p[1 + 2 + 1 + 32 + 2] = { _1W_WRITE_BLOCK_WITH_LEN, LOW_HIGH_ADDRESS(offset), BYTE_MASK(size), };
	BYTE q[] = { _1W_CONFIRM_WRITE, } ;
	struct transaction_log t[] = {
		TRXN_START,
		TRXN_WR_CRC16(p, 1+ 2 + 1 + size, 0),
		TRXN_WRITE1(q),
		TRXN_END,
	};
	
	/* Copy to scratchpad */
	memcpy(&p[4], data, size);
	
	return BUS_transaction(t, pn) ;
}

static int OW_w_extended(BYTE * data, size_t size, UINT * return_code, struct parsedname *pn)
{
	BYTE p[1 + 1 + 32 + 2] = { _1W_EXTENDED_COMMAND, BYTE_MASK(size), };
	BYTE q[] = { _1W_CONFIRM_WRITE, } ;
	BYTE r[2] ;
	int ret ;
	struct transaction_log t[] = {
		TRXN_START,
		TRXN_WR_CRC16(p, 1+ 1 + size, 0),
		TRXN_WRITE1(q),
		TRXN_READ2(r),
		TRXN_END,
	};
	
	/* Copy to scratchpad */
	memcpy(&p[4], data, size);
	
	ret = BUS_transaction(t, pn) ;
	if (ret) {
		return 1 ;
	}
	return_code[0] = (r[1]<<8) + r[0] ;
	return 0 ;
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

static int OW_version( UINT * version, struct parsedname * pn )
{
	BYTE p[5] = { _1W_READ_VERSION, } ;
	struct transaction_log t[] = {
		TRXN_START,
		TRXN_WR_CRC16(p, 1, 2),
		TRXN_END,
	} ;
		
	if (BUS_transaction(t, pn)) {
		return 1;
	}

	version[0] = (p[2]<<8) + p[1] ;
	return 0 ;
};

static int OW_type( UINT * type, struct parsedname * pn )
{
	BYTE p[5] = { _1W_READ_TYPE, } ;
	struct transaction_log t[] = {
		TRXN_START,
		TRXN_WR_CRC16(p, 1, 2),
		TRXN_END,
	} ;
	
	if (BUS_transaction(t, pn)) {
		return 1;
	}
	
	type[0] = (p[2]<<8) + p[1] ;
	return 0 ;
};

