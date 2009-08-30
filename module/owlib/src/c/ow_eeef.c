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
#include "ow_eeef.h"

/* ------- Prototypes ----------- */

/* Hobby boards UVI and colleagues */
READ_FUNCTION(FS_temperature);
READ_FUNCTION(FS_version);
READ_FUNCTION(FS_type_number);
READ_FUNCTION(FS_localtype);
READ_FUNCTION(FS_r_temperature_offset);
WRITE_FUNCTION(FS_w_temperature_offset);
READ_FUNCTION(FS_r_UVI_offset);
WRITE_FUNCTION(FS_w_UVI_offset);
READ_FUNCTION(FS_r_in_case);
WRITE_FUNCTION(FS_w_in_case);
READ_FUNCTION(FS_UVI);
READ_FUNCTION(FS_UVI_valid);

/* ------- Structures ----------- */

struct filetype HobbyBoards_EE[] = {
    F_STANDARD_NO_TYPE,
    {"temperature", PROPERTY_LENGTH_TEMP, NULL, ft_temperature, fc_volatile, FS_temperature, NO_WRITE_FUNCTION, {v:NULL},},
    {"temperature_offset", PROPERTY_LENGTH_TEMPGAP, NULL, ft_tempgap, fc_stable, FS_r_temperature_offset, FS_w_temperature_offset, {v:NULL},},
    {"in_case", PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_stable, FS_r_in_case, FS_w_in_case, {v:NULL},},
    {"version", 4, NULL, ft_ascii, fc_stable, FS_version, NO_WRITE_FUNCTION, {v:NULL},},
    {"type_number", PROPERTY_LENGTH_UNSIGNED, NULL, ft_unsigned, fc_stable, FS_type_number, NO_WRITE_FUNCTION, {v:NULL},},
    {"type", PROPERTY_LENGTH_TYPE, NULL, ft_ASCII, fc_LINK, FS_localtype, NO_WRITE_FUNCTION, {v:NULL},},
    {"UVI", PROPERTY_LENGTH_SUBDIR, NULL, ft_subdir, fc_volatile, NO_READ_FUNCTION, NO_WRITE_FUNCTION, {v:NULL},},
    {"UVI/UVI", PROPERTY_LENGTH_FLOAT, NULL, ft_float, fc_volatile, FS_UVI, NO_WRITE_FUNCTION, {v:NULL},},
    {"UVI/valid", PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_link, FS_UVI_valid, NO_WRITE_FUNCTION, {v:NULL},},
    {"UVI/UVI_offset", PROPERTY_LENGTH_FLOAT, NULL, ft_float, fc_stable, FS_r_UVI_offset, FS_w_UVI_offset, {v:NULL},},
};

DeviceEntry(EE, Hobby_Boards_generic);

struct filetype HobbyBoards_EF[] = {
    F_STANDARD_NO_TYPE,
    {"in_case", PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_stable, FS_r_in_case, FS_w_in_case, {v:NULL},},
    {"version", 4, NULL, ft_ascii, fc_stable, FS_version, NO_WRITE_FUNCTION, {v:NULL},},
    {"type_number", PROPERTY_LENGTH_UNSIGNED, NULL, ft_unsigned, fc_stable, FS_type_number, NO_WRITE_FUNCTION, {v:NULL},},
    {"type", PROPERTY_LENGTH_TYPE, NULL, ft_ASCII, fc_LINK, FS_localtype, NO_WRITE_FUNCTION, {v:NULL},},
    {"UVI", PROPERTY_LENGTH_SUBDIR, NULL, ft_subdir, fc_volatile, NO_READ_FUNCTION, NO_WRITE_FUNCTION, {v:NULL},},
    {"UVI/UVI", PROPERTY_LENGTH_FLOAT, NULL, ft_float, fc_volatile, FS_UVI, NO_WRITE_FUNCTION, {v:NULL},},
    {"UVI/valid", PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_link, FS_UVI_valid, NO_WRITE_FUNCTION, {v:NULL},},
    {"UVI/UVI_offset", PROPERTY_LENGTH_FLOAT, NULL, ft_float, fc_stable, FS_r_UVI_offset, FS_w_UVI_offset, {v:NULL},},
};

DeviceEntry(EE, Hobby_Boards_generic);


#define _EEEF_READ_VERSION 0x11
#define _EEEF_READ_TYPE 0x12
#define _EEEF_READ_TEMPERATURE 0x21
#define _EEEF_SET_TEMPERATURE_OFFSET 0x22
#define _EEEF_READ_TEMPERATURE_OFFSET 0x23
#define _EEEF_READ_UVI 0x24
#define _EEEF_SET_UVI_OFFSET 0x25
#define _EEEF_READ_UVI_OFFSET 0x26
#define _EEEF_SET_IN_CASE 0x27
#define _EEEF_READ_IN_CASE 0x28

/* ------- Functions ------------ */

/* DS2502 */
static int OW_r_mem(BYTE * data, size_t size, off_t offset, struct parsedname *pn);
static int OW_r_data(BYTE * data, struct parsedname *pn);

/* 2502 memory */
static int FS_r_memory(struct one_wire_query *owq)
{
	size_t pagesize = 32;
	if (COMMON_readwrite_paged(owq, 0, pagesize, OW_r_mem)) {
		return -EINVAL;
	}
	return 0;
}

static int FS_r_page(struct one_wire_query *owq)
{
	size_t pagesize = 32;
	if (COMMON_readwrite_paged(owq, OWQ_pn(owq).extension, pagesize, OW_r_mem)) {
		return -EINVAL;
	}
	return 0;
}

static int FS_r_param(struct one_wire_query *owq)
{
	struct parsedname *pn = PN(owq);
	BYTE data[32];
	if (OW_r_data(data, pn)) {
		return -EINVAL;
	}
	return Fowq_output_offset_and_size((ASCII *) & data[pn->selected_filetype->data.i], FileLength(pn), owq);
}

static int FS_w_memory(struct one_wire_query *owq)
{
	if (COMMON_write_eprom_mem(OWQ_explode(owq))) {
		return -EINVAL;
	}
	return 0;
}

static int FS_w_page(struct one_wire_query *owq)
{
	size_t pagesize = 32;
	OWQ_offset(owq) += OWQ_pn(owq).extension * pagesize;
	if (COMMON_write_eprom_mem(OWQ_explode(owq))) {
		return -EINVAL;
	}
	return 0;
}

static int OW_r_mem(BYTE * data, size_t size, off_t offset, struct parsedname *pn)
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
		return 1;
	}

	memcpy(data, q, size);
	return 0;
}

static int OW_r_data(BYTE * data, struct parsedname *pn)
{
	BYTE p[32];

	if (OW_r_mem(p, 32, 0, pn) || CRC16(p, 3 + p[0])) {
		return 1;
	}
	memcpy(data, &p[1], p[0]);
	return 0;
}
