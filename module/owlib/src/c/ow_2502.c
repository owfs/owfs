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

/* DS2423 counter */
READ_FUNCTION(FS_r_page);
WRITE_FUNCTION(FS_w_page);
READ_FUNCTION(FS_r_memory);
WRITE_FUNCTION(FS_w_memory);
READ_FUNCTION(FS_r_param);

/* ------- Structures ----------- */

struct aggregate A2502 = { 4, ag_numbers, ag_separate, };
struct filetype DS2502[] = {
	F_STANDARD,
	{"memory", 128, NON_AGGREGATE, ft_binary, fc_stable, FS_r_memory, FS_w_memory, VISIBLE, NO_FILETYPE_DATA,},
	{"pages", PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir, fc_volatile, NO_READ_FUNCTION, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA,},
  {"pages/page", 32, &A2502, ft_binary, fc_stable, FS_r_page, FS_w_page, VISIBLE, NO_FILETYPE_DATA,},
};

DeviceEntry(09, DS2502);

struct filetype DS1982U[] = {
	F_STANDARD,
	{"mac_e", 6, NON_AGGREGATE, ft_binary, fc_stable, FS_r_param, NO_WRITE_FUNCTION, VISIBLE, {i:4},},
	{"mac_fw", 8, NON_AGGREGATE, ft_binary, fc_stable, FS_r_param, NO_WRITE_FUNCTION, VISIBLE, {i:4},},
	{"project", 4, NON_AGGREGATE, ft_binary, fc_stable, FS_r_param, NO_WRITE_FUNCTION, VISIBLE, {i:0},},
	{"memory", 128, NON_AGGREGATE, ft_binary, fc_stable, FS_r_memory, FS_w_memory, VISIBLE, NO_FILETYPE_DATA,},
	{"pages", PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir, fc_volatile, NO_READ_FUNCTION, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA,},
  {"pages/page", 32, &A2502, ft_binary, fc_stable, FS_r_page, FS_w_page, VISIBLE, NO_FILETYPE_DATA,},
};

DeviceEntry(89, DS1982U);

#define _1W_READ_MEMORY 0xF0
#define _1W_READ_STATUS 0xAA
#define _1W_READ_DATA_CRC8 0xC3
#define _1W_WRITE_MEMORY 0x0F
#define _1W_WRITE_STATUS 0x55

/* ------- Functions ------------ */

/* DS2502 */
static GOOD_OR_BAD OW_r_mem(BYTE * data, size_t size, off_t offset, struct parsedname *pn);
static GOOD_OR_BAD OW_r_data(BYTE * data, struct parsedname *pn);

/* 2502 memory */
static ZERO_OR_ERROR FS_r_memory(struct one_wire_query *owq)
{
	size_t pagesize = 32;
	return GB_to_Z_OR_E(COMMON_readwrite_paged(owq, 0, pagesize, OW_r_mem)) ;
}

static ZERO_OR_ERROR FS_r_page(struct one_wire_query *owq)
{
	size_t pagesize = 32;
	return GB_to_Z_OR_E(COMMON_readwrite_paged(owq, OWQ_pn(owq).extension, pagesize, OW_r_mem)) ;
}

static ZERO_OR_ERROR FS_r_param(struct one_wire_query *owq)
{
	struct parsedname *pn = PN(owq);
	BYTE data[32];
	RETURN_ERROR_IF_BAD( OW_r_data(data, pn) );
	return OWQ_format_output_offset_and_size((ASCII *) & data[pn->selected_filetype->data.i], FileLength(pn), owq);
}

static ZERO_OR_ERROR FS_w_memory(struct one_wire_query *owq)
{
	return COMMON_write_eprom_mem_owq(owq) ;
}

static ZERO_OR_ERROR FS_w_page(struct one_wire_query *owq)
{
	size_t pagesize = 32;
	OWQ_offset(owq) += OWQ_pn(owq).extension * pagesize;
	return COMMON_write_eprom_mem_owq(owq) ;
}

static GOOD_OR_BAD OW_r_mem(BYTE * data, size_t size, off_t offset, struct parsedname *pn)
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
	if (BUS_transaction(t, pn)) {
		return gbBAD;
	}

	memcpy(data, q, size);
	return gbGOOD;
}

static GOOD_OR_BAD OW_r_data(BYTE * data, struct parsedname *pn)
{
	BYTE p[32];

	RETURN_BAD_IF_BAD( OW_r_mem(p, 32, 0, pn) ) ;
	if ( CRC16(p, 3 + p[0])) {
		return gbBAD;
	}
	memcpy(data, &p[1], p[0]);
	return gbGOOD;
}
