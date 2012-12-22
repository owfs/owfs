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
#include "ow_2502.h"

/* ------- Prototypes ----------- */

/* DS2502 counter */
READ_FUNCTION(FS_r_page);
WRITE_FUNCTION(FS_w_page);
READ_FUNCTION(FS_r_mem);
WRITE_FUNCTION(FS_w_mem);
READ_FUNCTION(FS_r_param);

/* ------- Structures ----------- */

static struct aggregate A2502 = { 4, ag_numbers, ag_separate, };
static struct filetype DS2502[] = {
	F_STANDARD,
	{"memory", 128, NON_AGGREGATE, ft_binary, fc_link, FS_r_mem, FS_w_mem, VISIBLE, NO_FILETYPE_DATA, },
	{"pages", PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir, fc_subdir, NO_READ_FUNCTION, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA, },
	{"pages/page", 32, &A2502, ft_binary, fc_page, FS_r_page, FS_w_page, VISIBLE, NO_FILETYPE_DATA, },
};

DeviceEntry(09, DS2502, NO_GENERIC_READ, NO_GENERIC_WRITE);

static struct filetype DS1982U[] = {
	F_STANDARD,
	{"memory", 128, NON_AGGREGATE, ft_binary, fc_link, FS_r_mem, FS_w_mem, VISIBLE, NO_FILETYPE_DATA, },
	{"pages", PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir, fc_subdir, NO_READ_FUNCTION, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA, },
	{"pages/page", 32, &A2502, ft_binary, fc_page, FS_r_page, FS_w_page, VISIBLE, NO_FILETYPE_DATA, },

	{"mac_e", 6, NON_AGGREGATE, ft_binary, fc_stable, FS_r_param, NO_WRITE_FUNCTION, VISIBLE, {i:4}, },
	{"mac_fw", 8, NON_AGGREGATE, ft_binary, fc_stable, FS_r_param, NO_WRITE_FUNCTION, VISIBLE, {i:4}, },
	{"project", 4, NON_AGGREGATE, ft_binary, fc_stable, FS_r_param, NO_WRITE_FUNCTION, VISIBLE, {i:0}, },
};

DeviceEntry(89, DS1982U, NO_GENERIC_READ, NO_GENERIC_WRITE);

#define _1W_READ_MEMORY     0xF0
#define _1W_READ_STATUS     0xAA
#define _1W_READ_DATA_CRC8  0xC3
#define _1W_WRITE_MEMORY    0x0F
#define _1W_WRITE_STATUS    0x55
// DS2704 specific
#define _1W_WRITE_CHALLENGE             0x0C
#define _1W_COMPUTE_MAC_WITHOUT         0x36
#define _1W_COMPUTE_MAC_WITH            0x35
#define _1W_CLEAR_SECRET                0x5A
#define _1W_COMPUTE_NEXT_SECRET_WITHOUT 0x30
#define _1W_COMPUTE_NEXT_SECRET_WITH    0x33
#define _1W_LOCK_SECRET                 0x6A
#define _1W_READ_ALL                    0x65
#define _1W_READ_SCRATCHPAD             0x69
#define _1W_WRITE_SCRATCHPAD            0x6C
#define _1W_COPY_SCRATCHPAD             0x48
#define _1W_SET_OVERDRIVE               0x8B
#define _1W_CLEAR_OVERDRIVE             0x8D
#define _1W_RESET                       0xBB


/* ------- Functions ------------ */

/* DS2502 */
static GOOD_OR_BAD OW_r_page( BYTE * data, size_t size, off_t offset, struct parsedname *pn);
static GOOD_OR_BAD OW_w_bytes(BYTE * data, size_t size, off_t offset, struct parsedname *pn) ;
static GOOD_OR_BAD OW_w_byte(BYTE data, off_t offset, struct parsedname *pn) ;

/* 2502 memory */
static ZERO_OR_ERROR FS_r_mem(struct one_wire_query *owq)
{
	size_t pagesize = 32;
	return GB_to_Z_OR_E( COMMON_readwrite_paged(owq, 0, pagesize, OW_r_page) ) ;
}

static ZERO_OR_ERROR FS_r_page(struct one_wire_query *owq)
{
	size_t pagesize = 32;
	return GB_to_Z_OR_E( OW_r_page( (BYTE *) OWQ_buffer(owq), OWQ_size(owq), OWQ_offset(owq) + pagesize * PN(owq)->extension, PN(owq) ) ) ;
}

static ZERO_OR_ERROR FS_r_param(struct one_wire_query *owq)
{
	struct parsedname *pn = PN(owq);
	size_t pagesize = 32;
	BYTE data[pagesize];
	off_t param_offset = pn->selected_filetype->data.i ;
	
	RETURN_ERROR_IF_BAD( OW_r_page(data, pagesize-param_offset, param_offset, pn) );
	return OWQ_format_output_offset_and_size((ASCII *) data, FileLength(pn), owq);
}

static ZERO_OR_ERROR FS_w_mem(struct one_wire_query *owq)
{
	size_t pagesize = 1;
	return GB_to_Z_OR_E( COMMON_readwrite_paged(owq, 0, pagesize, OW_w_bytes) ) ;
}

static ZERO_OR_ERROR FS_w_page(struct one_wire_query *owq)
{
	size_t pagesize = 32;
	return COMMON_offset_process( FS_w_mem, owq, OWQ_pn(owq).extension*pagesize ) ;
}

// reads to end of page and discards extra
// uses CRC8
static GOOD_OR_BAD OW_r_page(BYTE * data, size_t size, off_t offset, struct parsedname *pn)
{
	BYTE p[4] = { _1W_READ_DATA_CRC8, LOW_HIGH_ADDRESS(offset), };
	BYTE q[33];
	int rest = 33 - (offset & 0x1F);
	struct transaction_log t[] = {
		TRXN_START,
		TRXN_WRITE3(p),
		TRXN_READ1(&p[3]),
		TRXN_CRC8(p, 4),
		TRXN_READ(q, rest),
		TRXN_CRC8(q, rest),
		TRXN_END,
	};
	RETURN_BAD_IF_BAD(BUS_transaction(t, pn)) ;

	memcpy(data, q, size);
	return gbGOOD;
}

// placeholder for OW_w_byte but uses common arguments for COMMON_readwrite_paged
static GOOD_OR_BAD OW_w_bytes(BYTE * data, size_t size, off_t offset, struct parsedname *pn)
{
	(void) size ; // must be 1
	return OW_w_byte( data[0], offset, pn ) ;
} 

static GOOD_OR_BAD OW_w_byte(BYTE data, off_t offset, struct parsedname *pn)
{
	BYTE p[5] = { _1W_WRITE_MEMORY, LOW_HIGH_ADDRESS(offset), data, 0xFF };
	BYTE q[1];
	struct transaction_log t[] = {
		TRXN_START,
		TRXN_WRITE(p,4),
		TRXN_READ1(&p[4]),
		TRXN_CRC8(p, 4+1),
		TRXN_PROGRAM,
		TRXN_READ1(q),
		TRXN_COMPARE(q,&data,1) ,
		TRXN_END,
	};

	return BUS_transaction(t, pn) ;
}

