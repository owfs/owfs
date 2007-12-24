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
#include "ow_1991.h"

/* ------- Prototypes ----------- */

READ_FUNCTION(FS_r_page);
WRITE_FUNCTION(FS_w_page);
READ_FUNCTION(FS_r_ident);
WRITE_FUNCTION(FS_w_ident);
READ_FUNCTION(FS_r_memory);
WRITE_FUNCTION(FS_w_memory);
WRITE_FUNCTION(FS_w_password);
WRITE_FUNCTION(FS_w_reset_password);
WRITE_FUNCTION(FS_w_change_password);

/* ------- Structures ----------- */

struct aggregate A1991 = { 3, ag_numbers, ag_separate, };
struct filetype DS1991[] = {
	F_STANDARD,
  {"settings",PROPERTY_LENGTH_SUBDIR, NULL, ft_subdir, fc_volatile,   NO_READ_FUNCTION, NO_WRITE_FUNCTION, {v:NULL},} ,
  {"settings/reset_password", 8, &A1991, ft_binary, fc_stable,   NO_READ_FUNCTION, FS_w_reset_password, {v:NULL},} ,
  {"settings/change_password", 8, &A1991, ft_binary, fc_stable,   NO_READ_FUNCTION, FS_w_change_password, {v:NULL},} ,
  {"settings/password", 8, &A1991, ft_binary, fc_stable,   NO_READ_FUNCTION, FS_w_password, {v:NULL},} ,
  {"settings/ident", 8, &A1991, ft_binary, fc_volatile,   FS_r_ident, FS_w_ident, {v:NULL},} ,
  {"settings/page", 48, &A1991, ft_binary, fc_volatile,   FS_r_page, FS_w_page, {v:NULL},} ,
  {"pages",PROPERTY_LENGTH_SUBDIR, NULL, ft_subdir, fc_volatile,   NO_READ_FUNCTION, NO_WRITE_FUNCTION, {v:NULL},} ,
  {"pages/page", 48, &A1991, ft_binary, fc_volatile,   FS_r_page, FS_w_page, {v:NULL},} ,
  {"pages/password", 8, &A1991, ft_binary, fc_stable,   NO_READ_FUNCTION, FS_w_password, {v:NULL},} ,
  {"pages/ident", 8, &A1991, ft_binary, fc_volatile,   FS_r_ident, FS_w_ident, {v:NULL},} ,
  {"memory", 144, NULL, ft_binary, fc_volatile,   FS_r_memory, FS_w_memory, {v:NULL},} ,
};

DeviceEntry(02, DS1991);

struct filetype DS1425[] = {
	F_STANDARD,
  {"settings",PROPERTY_LENGTH_SUBDIR, NULL, ft_subdir, fc_volatile,   NO_READ_FUNCTION, NO_WRITE_FUNCTION, {v:NULL},} ,
  {"settings/reset_password", 8, &A1991, ft_binary, fc_stable,   NO_READ_FUNCTION, FS_w_reset_password, {v:NULL},} ,
  {"settings/change_password", 8, &A1991, ft_binary, fc_stable,   NO_READ_FUNCTION, FS_w_change_password, {v:NULL},} ,
  {"settings/password", 8, &A1991, ft_binary, fc_stable,   NO_READ_FUNCTION, FS_w_password, {v:NULL},} ,
  {"settings/ident", 8, &A1991, ft_binary, fc_volatile,   FS_r_ident, FS_w_ident, {v:NULL},} ,
  {"settings/page", 48, &A1991, ft_binary, fc_volatile,   FS_r_page, FS_w_page, {v:NULL},} ,
  {"pages",PROPERTY_LENGTH_SUBDIR, NULL, ft_subdir, fc_volatile,   NO_READ_FUNCTION, NO_WRITE_FUNCTION, {v:NULL},} ,
  {"pages/page", 48, &A1991, ft_binary, fc_volatile,   FS_r_page, FS_w_page, {v:NULL},} ,
  {"pages/password", 8, &A1991, ft_binary, fc_stable,   NO_READ_FUNCTION, FS_w_password, {v:NULL},} ,
  {"pages/ident", 8, &A1991, ft_binary, fc_volatile,   FS_r_ident, FS_w_ident, {v:NULL},} ,
  {"memory", 144, NULL, ft_binary, fc_volatile,   FS_r_memory, FS_w_memory, {v:NULL},} ,
};

DeviceEntry(82, DS1425);

#define _1W_WRITE_SCRATCHPAD 0x96
#define _1W_READ_SCRATCHPAD 0x69
#define _1W_COPY_SCRATCHPAD 0x3C
#define _1W_WRITE_PASSWORD 0x5A
#define _1W_WRITE_SUBKEY 0x99
#define _1W_READ_SUBKEY 0x66

static BYTE global_passwd[3][8] = { "", "", "" };

/* ------- Functions ------------ */


static int OW_r_ident(BYTE * data, const size_t size, const off_t offset,
					  const struct parsedname *pn);
static int OW_w_ident(const BYTE * data, const size_t size,
					  const off_t offset, const struct parsedname *pn);
static int OW_w_reset_password(const BYTE * data, const size_t size,
							   const off_t offset,
							   const struct parsedname *pn);
static int OW_w_change_password(const BYTE * data, const size_t size,
								const off_t offset,
								const struct parsedname *pn);
static int OW_r_page(BYTE * data, const size_t size, const off_t offset,
					 const struct parsedname *pn);
static int OW_w_page(const BYTE * data, const size_t size,
					 const off_t offset, const struct parsedname *pn);
static int OW_r_memory(BYTE * data, const size_t size, const off_t offset,
					   const struct parsedname *pn);
static int OW_w_memory(const BYTE * data, const size_t size,
					   const off_t offset, const struct parsedname *pn);
static int OW_r_subkey(BYTE * data, const size_t size, const off_t offset,
					   const struct parsedname *pn, const int extension);
static int OW_w_subkey(const BYTE * data, const size_t size,
					   const off_t offset, const struct parsedname *pn,
					   const int extension);

/* array with magic bytes representing the Copy Scratch operations */
enum block { ALL = 0, IDENT, PASSWORD, DATA };
static const BYTE cp_array[9][8] = {
	{0x56, 0x56, 0x7F, 0x51, 0x57, 0x5D, 0x5A, 0x7F},	// 00-3F
	{0x9A, 0x9A, 0xB3, 0x9D, 0x64, 0x6E, 0x69, 0x4C},	// ident
	{0x9A, 0x9A, 0x4C, 0x62, 0x9B, 0x91, 0x69, 0x4C},	// passwd
	{0x9A, 0x65, 0xB3, 0x62, 0x9B, 0x6E, 0x96, 0x4C},	// 10-17
	{0x6A, 0x6A, 0x43, 0x6D, 0x6B, 0x61, 0x66, 0x43},	// 18-1F
	{0x95, 0x95, 0xBC, 0x92, 0x94, 0x9E, 0x99, 0xBC},	// 20-27
	{0x65, 0x9A, 0x4C, 0x9D, 0x64, 0x91, 0x69, 0xB3},	// 28-2F
	{0x65, 0x65, 0xB3, 0x9D, 0x64, 0x6E, 0x96, 0xB3},	// 30-37
	{0x65, 0x65, 0x4C, 0x62, 0x9B, 0x91, 0x96, 0xB3},	// 38-3F
};

#ifndef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif

static int FS_w_password(struct one_wire_query * owq)
{
    struct parsedname * pn = PN(owq) ;
    size_t s = OWQ_size(owq) ;

    if ( s > 8 ) s = 8 ;
    memset(global_passwd[pn->extension], 0, 8);
    memcpy(global_passwd[pn->extension], (BYTE *) OWQ_buffer(owq), s);
	//printf("Use password [%s] for subkey %d\n", global_passwd[pn->extension], pn->extension);
	return 0;
}

static int FS_w_reset_password(struct one_wire_query * owq)
{
    if (OW_w_reset_password((BYTE *) OWQ_buffer(owq), OWQ_size(owq), (size_t) OWQ_offset(owq), PN(owq)))
		return -EINVAL;
	return 0;
}

static int FS_w_change_password(struct one_wire_query * owq)
{
    if (OW_w_change_password((BYTE *) OWQ_buffer(owq), OWQ_size(owq), (size_t) OWQ_offset(owq), PN(owq)))
		return -EINVAL;
	return 0;
}

static int FS_r_page(struct one_wire_query * owq)
{
    if (OW_r_page((BYTE *) OWQ_buffer(owq), OWQ_size(owq), (size_t) OWQ_offset(owq), PN(owq)))
		return -EINVAL;
    return OWQ_size(owq);
}

static int FS_w_page(struct one_wire_query * owq)
{
    if (OW_w_page((BYTE *) OWQ_buffer(owq), OWQ_size(owq), (size_t) OWQ_offset(owq), PN(owq)))
		return -EINVAL;
	return 0;
}

static int FS_r_ident(struct one_wire_query * owq)
{
    if (OW_r_ident((BYTE *) OWQ_buffer(owq), OWQ_size(owq), (size_t) OWQ_offset(owq), PN(owq)))
		return -EINVAL;
    return OWQ_size(owq);
}

static int FS_w_ident(struct one_wire_query * owq)
{
    if (OWQ_offset(owq))
		return -EINVAL;
    if (OW_w_ident((BYTE *) OWQ_buffer(owq), OWQ_size(owq), (size_t) OWQ_offset(owq), PN(owq)))
		return -EINVAL;
	return 0;
}

static int FS_r_memory(struct one_wire_query * owq)
{
    if (OW_r_memory((BYTE *) OWQ_buffer(owq), OWQ_size(owq), (size_t) OWQ_offset(owq), PN(owq)))
		return -EINVAL;
    return OWQ_size(owq);
}

static int FS_w_memory(struct one_wire_query * owq)
{
    if (OW_w_memory((BYTE *) OWQ_buffer(owq), OWQ_size(owq), (size_t) OWQ_offset(owq), PN(owq)))
		return -EINVAL;
	return 0;
}

static int OW_w_reset_password(const BYTE * data, const size_t size,
							   const off_t offset,
							   const struct parsedname *pn)
{
	BYTE set_password[3] = { _1W_WRITE_PASSWORD, 0x00, 0x00 };
	BYTE passwd[8];
	BYTE ident[8];
	struct transaction_log tscratch[] = {
		TRXN_START,
        TRXN_WRITE3(set_password),
        TRXN_READ(ident, 8),
        TRXN_WRITE(ident, 8),
        TRXN_WRITE(ident, 8),
        TRXN_WRITE(passwd, 8),
        TRXN_END,
	};


	if (offset)
		return -EINVAL;

	memset(passwd, 0, 8);
	memcpy(passwd, data, MIN(size, 8));
	set_password[1] = pn->extension << 6;
	set_password[2] = ~(set_password[1]);
	if (BUS_transaction(tscratch, pn))
		return 1;
	/* Verification is done... now send ident + password
	 * Note: ALL saved data in subkey will be deleted during this operation
	 */

	memcpy(global_passwd[pn->extension], passwd, 8);
	return 0;
}

static int OW_w_subkey(const BYTE * data, const size_t size,
					   const off_t offset, const struct parsedname *pn,
					   const int extension)
{
	BYTE p[4] = { _1W_WRITE_SUBKEY, 0x00, 0x00, 0x00 };	// write subkey
	BYTE ident[8];
	struct transaction_log tscratch[] = {
		TRXN_START,
        TRXN_WRITE3(p),
        TRXN_READ(ident,8),
        TRXN_WRITE(global_passwd[extension], 8),
        TRXN_WRITE( &data[0x10], size - 0x10),
		TRXN_END,
	};

	if ((size <= 0x10) || (size > 0x40)) {
		return 1;
	}

	p[1] = extension << 6;
	p[1] |= (0x10 + offset);	// + 0x10 -> 0x3F
	p[2] = ~(p[1]);

	if (BUS_transaction(tscratch, pn))
		return 1;
	return 0;
}

static int OW_r_subkey(BYTE * data, const size_t size, const off_t offset,
					   const struct parsedname *pn, const int extension)
{
	BYTE p[3] = { _1W_READ_SUBKEY, 0x00, 0x00 };	// read subkey
	struct transaction_log tscratch[] = {
		TRXN_START,
        TRXN_WRITE3(p),
        TRXN_READ(data,8),
        TRXN_WRITE(global_passwd[extension], 8),
        TRXN_WRITE( &data[0x10 + offset], 0x30 - offset),
		TRXN_END,
	};

	//printf("OW_r_subkey\n");

	memset(data, 0, 0x40);
	memcpy(&data[8], global_passwd[extension], 8);
	if (offset >= 0x30)
		return -EINVAL;

	if (size != 0x40) {
		/* Only allow reading whole subkey right now */
		return 1;
	}

	p[1] = extension << 6;
	p[1] |= (offset + 0x10);
	p[2] = ~(p[1]);

	if (BUS_transaction(tscratch, pn))
		return 1;

	return 0;
}

static int OW_r_memory(BYTE * data, const size_t size, const off_t offset,
					   const struct parsedname *pn)
{
	BYTE all_data[0x40];
	int i, ret, nr_bytes;
	size_t left = size;

	//printf("OW_r_memory\n");

	if (offset)
		return 1;

	for (i = 0; (i < 3) && (left > 0); i++) {
		memset(all_data, 0, 0x40);
		ret = OW_r_subkey(all_data, 0x40, 0, pn, i);
		if (ret) {
			return 1;
		}
		nr_bytes = MIN(0x30, left);
		memcpy(&data[i * 0x30], &all_data[0x10], nr_bytes);
		left -= nr_bytes;
	}
	return 0;
}

static int OW_w_memory(const BYTE * data, const size_t size,
					   const off_t offset, const struct parsedname *pn)
{
	BYTE all_data[0x40];
	int i, ret, nr_bytes;
	size_t left = size;

	//printf("OW_w_memory\n");

	if (offset)
		return 1;

	for (i = 0; (i < 3) && (left > 0); i++) {
		memset(all_data, 0, 0x40);
		ret = OW_r_subkey(all_data, 0x40, 0, pn, i);
		if (ret) {
			return 1;
		}
		nr_bytes = MIN(0x30, left);
		memcpy(&all_data[0x10], &data[size - left], nr_bytes);

		ret = OW_w_subkey(all_data, 0x40, 0, pn, i);
		if (ret) {
			return 1;
		}

		left -= nr_bytes;
	}
	return 0;
}

// size and offset already bounds checked
static int OW_r_ident(BYTE * data, const size_t size, const off_t offset,
					  const struct parsedname *pn)
{
	BYTE all_data[0x40];
	//printf("OW_r_ident\n");

	if (OW_r_subkey(all_data, 0x40, 0, pn, pn->extension))
		return 1;

	memcpy(data, &all_data[offset], size);
	return 0;
}

static int OW_w_ident(const BYTE * data, const size_t size,
					  const off_t offset, const struct parsedname *pn)
{
	BYTE write_scratch[3] = { _1W_WRITE_SCRATCHPAD, 0x00, 0x00 };
	BYTE copy_scratch[3] = { _1W_COPY_SCRATCHPAD, 0x00, 0x00 };
	BYTE all_data[0x40];
	struct transaction_log tscratch[] = {
		TRXN_START,
        TRXN_WRITE3(write_scratch),
        TRXN_WRITE(all_data, 0x40),
		TRXN_END,
	};
	struct transaction_log tcopy[] = {
		TRXN_START,
        TRXN_WRITE3(copy_scratch),
        TRXN_WRITE( cp_array[IDENT], 8),
        TRXN_WRITE(global_passwd[pn->extension], 8),
        TRXN_END,
	};

	//printf("OW_w_ident\n");

	if (offset)
		return -EINVAL;

	write_scratch[1] = 0xC0 | offset;
	write_scratch[2] = ~(write_scratch[1]);
	memset(all_data, 0, 0x40);
	memcpy(all_data, data, MIN(size, 8));
	if (BUS_transaction(tscratch, pn))
		return 1;

	copy_scratch[1] = pn->extension << 6;
	copy_scratch[2] = ~(copy_scratch[1]);
	if (BUS_transaction(tcopy, pn))
		return 1;

	return 0;
}

static int OW_w_change_password(const BYTE * data, const size_t size,
								const off_t offset,
								const struct parsedname *pn)
{
	BYTE write_scratch[3] = { _1W_WRITE_SCRATCHPAD, 0x00, 0x00 };
	BYTE copy_scratch[3] = { _1W_COPY_SCRATCHPAD, 0x00, 0x00 };
	BYTE all_data[0x40];
	struct transaction_log tscratch[] = {
		TRXN_START,
        TRXN_WRITE3(write_scratch),
        TRXN_WRITE(all_data, 0x40),
        TRXN_END,
	};
	struct transaction_log tcopy[] = {
		TRXN_START,
        TRXN_WRITE3(copy_scratch),
        TRXN_WRITE( cp_array[PASSWORD], 8),
        TRXN_WRITE(global_passwd[pn->extension], 8),
		TRXN_END,
	};

	if (offset)
		return -EINVAL;

	write_scratch[1] = 0xC0 | offset;
	write_scratch[2] = ~(write_scratch[1]);
	memset(all_data, 0, 0x40);
	memcpy(&all_data[0x08], data, MIN(size, 8));
	if (BUS_transaction(tscratch, pn))
		return 1;

	copy_scratch[1] = pn->extension << 6;
	copy_scratch[2] = ~(copy_scratch[1]);

	if (BUS_transaction(tcopy, pn))
		return 1;
	memcpy(global_passwd[pn->extension], &all_data[0x08], 8);

	return 0;
}

static int OW_r_page(BYTE * data, const size_t size, const off_t offset,
					 const struct parsedname *pn)
{
	BYTE all_data[0x40];
	int ret;

	if (offset > 0x2F)
		return -EINVAL;

	ret = OW_r_subkey(all_data, 0x40, 0, pn, pn->extension);
	if (ret) {
		return 1;
	}
	memcpy(data, &all_data[0x10 + offset], size);
	return 0;
}

static int OW_w_page(const BYTE * data, const size_t size,
					 const off_t offset, const struct parsedname *pn)
{
	BYTE write_scratch[3] = { _1W_WRITE_SCRATCHPAD, 0xC0, BYTE_INVERSE(0xC0) };
	BYTE copy_scratch[3] = { _1W_COPY_SCRATCHPAD, 0x00, 0x00 };
	BYTE all_data[0x40];
	int i, nr_bytes;
	size_t left = size;
	struct transaction_log tscratch[] = {
		TRXN_START,
        TRXN_WRITE3(write_scratch),
        TRXN_WRITE(all_data, 0x40),
		TRXN_END,
	};
	struct transaction_log tcopy[] = {
		TRXN_START,
        TRXN_WRITE3(copy_scratch),
        TRXN_WRITE(global_passwd[pn->extension], 8),
		TRXN_END,
	};

	(void) offset;				// to suppress compiler warning
	if (size > 0x30) {
		//printf("size > 0x30\n");
		return 1;
	}
	memset(all_data, 0, 0x40);
	memcpy(&all_data[0x10], data, size);

	if (BUS_transaction(tscratch, pn))
		return 1;

	/*
	 * There are two possibilities to write memory.
	 *   Write byte 0x00->0x3F (including ident,password)
	 *   Write only 8 bytes at a time.
	 * Haven't decided if I should:
	 *   read page(48 bytes), overwrite data, save page
	 * or
	 *   write block (8 bytes)
	 */
	for (i = 0; (i < 8) && (left > 0); i++) {
		nr_bytes = MIN(left, 8);

		copy_scratch[1] = (pn->extension << 6);
		copy_scratch[2] = ~(copy_scratch[1]);
		tcopy[2].out = cp_array[DATA + i];
		if (BUS_transaction(tcopy, pn))
			return 1;
		left -= nr_bytes;
	}
	return 0;
}
