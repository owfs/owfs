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
#include "ow_2505.h"

/* ------- Prototypes ----------- */

READ_FUNCTION(FS_r_page);
WRITE_FUNCTION(FS_w_page);
READ_FUNCTION(FS_r_status);
WRITE_FUNCTION(FS_w_status);
READ_FUNCTION(FS_r_memory);
WRITE_FUNCTION(FS_w_memory);

#define _1W_READ_MEMORY          0xF0
#define _1W_READ_STATUS          0xAA
#define _1W_EXTENDED_READ_MEMORY 0xA5
#define _1W_WRITE_MEMORY         0x0F
#define _1W_SPEED_WRITE_MEMORY   0xF3
#define _1W_WRITE_STATUS         0x55
#define _1W_SPEED_WRITE_STATUS   0xF5

/* ------- Structures ----------- */

struct aggregate A2505 = { 64, ag_numbers, ag_separate, };
struct aggregate A2505s = { 11, ag_numbers, ag_separate, };
struct filetype DS2505[] = {
	F_STANDARD,
  {"status", PROPERTY_LENGTH_SUBDIR, NULL, ft_subdir, fc_volatile, NO_READ_FUNCTION, NO_WRITE_FUNCTION, {v:NULL},},
  {"status/page", 8, &A2505s, ft_binary, fc_stable, FS_r_status, FS_w_status, {v:NULL},},
  {"memory", 2048, NULL, ft_binary, fc_stable, FS_r_memory, FS_w_memory, {v:NULL},},
  {"pages", PROPERTY_LENGTH_SUBDIR, NULL, ft_subdir, fc_volatile, NO_READ_FUNCTION, NO_WRITE_FUNCTION, {v:NULL},},
  {"pages/page", 32, &A2505, ft_binary, fc_stable, FS_r_page, FS_w_page, {v:NULL},},
};

DeviceEntry(0B, DS2505);

struct filetype DS1985U[] = {
	F_STANDARD,
  {"status", PROPERTY_LENGTH_SUBDIR, NULL, ft_subdir, fc_volatile, NO_READ_FUNCTION, NO_WRITE_FUNCTION, {v:NULL},},
  {"status/page", 8, &A2505s, ft_binary, fc_stable, FS_r_status, FS_w_status, {v:NULL},},
  {"memory", 2048, NULL, ft_binary, fc_stable, FS_r_memory, FS_w_memory, {v:NULL},},
  {"pages", PROPERTY_LENGTH_SUBDIR, NULL, ft_subdir, fc_volatile, NO_READ_FUNCTION, NO_WRITE_FUNCTION, {v:NULL},},
  {"pages/page", 32, &A2505, ft_binary, fc_stable, FS_r_page, FS_w_page, {v:NULL},},
};

DeviceEntry(8B, DS1985U);

struct aggregate A2506 = { 256, ag_numbers, ag_separate, };
struct aggregate A2506s = { 11, ag_numbers, ag_separate, };
struct filetype DS2506[] = {
	F_STANDARD,
  {"status", PROPERTY_LENGTH_SUBDIR, NULL, ft_subdir, fc_volatile, NO_READ_FUNCTION, NO_WRITE_FUNCTION, {v:NULL},},
  {"status/page", 32, &A2506s, ft_binary, fc_stable, FS_r_status, FS_w_status, {v:NULL},},
  {"memory", 8192, NULL, ft_binary, fc_stable, FS_r_memory, FS_w_memory, {v:NULL},},
  {"pages", PROPERTY_LENGTH_SUBDIR, NULL, ft_subdir, fc_volatile, NO_READ_FUNCTION, NO_WRITE_FUNCTION, {v:NULL},},
  {"pages/page", 32, &A2506, ft_binary, fc_stable, FS_r_page, FS_w_page, {v:NULL},},
};

DeviceEntryExtended(0F, DS2506, DEV_ovdr);

struct filetype DS1986U[] = {
	F_STANDARD,
  {"status", PROPERTY_LENGTH_SUBDIR, NULL, ft_subdir, fc_volatile, NO_READ_FUNCTION, NO_WRITE_FUNCTION, {v:NULL},},
  {"status/page", 32, &A2506s, ft_binary, fc_stable, FS_r_status, FS_w_status, {v:NULL},},
  {"memory", 8192, NULL, ft_binary, fc_stable, FS_r_memory, FS_w_memory, {v:NULL},},
  {"pages", PROPERTY_LENGTH_SUBDIR, NULL, ft_subdir, fc_volatile, NO_READ_FUNCTION, NO_WRITE_FUNCTION, {v:NULL},},
  {"pages/page", 32, &A2506, ft_binary, fc_stable, FS_r_page, FS_w_page, {v:NULL},},
};

DeviceEntryExtended(8F, DS1986U, DEV_ovdr);

/* ------- Functions ------------ */

/* DS2505 */
static int OW_w_status(BYTE * data, size_t size, off_t offset, struct parsedname *pn);

/* 2505 memory */
static int FS_r_memory(struct one_wire_query *owq)
{
	size_t pagesize = 32;
//    if ( OW_r_mem( buf, size, (size_t) offset, pn) ) return -EINVAL ;
	if (OWQ_readwrite_paged(owq, 0, pagesize, OW_r_mem_simple))
		return -EINVAL;
	return 0;
}

static int FS_r_page(struct one_wire_query *owq)
{
	size_t pagesize = 32;
	if (OWQ_readwrite_paged(owq, OWQ_pn(owq).extension, pagesize, OW_r_mem_simple))
		return -EINVAL;
	return 0;
}

static int FS_r_status(struct one_wire_query *owq)
{
	size_t pagesize = PN(owq)->selected_filetype->suglen;
	if (OWQ_readwrite_paged(owq, OWQ_pn(owq).extension, pagesize, OW_r_mem_crc16_AA))
		return -EINVAL;
	return 0;
}

static int FS_w_memory(struct one_wire_query *owq)
{
	if (OW_w_eprom_mem(OWQ_explode(owq)))
		return -EINVAL;
	return 0;
}

static int FS_w_status(struct one_wire_query *owq)
{
	if (OW_w_status(OWQ_explode(owq)))
		return -EINVAL;
	return 0;
}

static int FS_w_page(struct one_wire_query *owq)
{
	size_t pagesize = 32;
	if (OW_w_eprom_mem((BYTE *) OWQ_buffer(owq), OWQ_size(owq), OWQ_offset(owq) + OWQ_pn(owq).extension * pagesize, PN(owq)))
		return -EINVAL;
	return 0;
}

static int OW_w_status(BYTE * data, size_t size, off_t offset, struct parsedname *pn)
{
	BYTE p[6] = { _1W_WRITE_STATUS, LOW_HIGH_ADDRESS(offset), data[0] };
	int ret;
	struct transaction_log tfirst[] = {
		TRXN_START,
		TRXN_WR_CRC16(p, 4, 0),
		TRXN_PROGRAM,
		TRXN_READ1(p),
		TRXN_END,
	};

	if (size == 0)
		return 0;
	if (size == 1)
		return BUS_transaction(tfirst, pn) || (p[0] & (~data[0]));
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
