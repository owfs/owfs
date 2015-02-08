/*
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
#include "ow_1991.h"

/* ------- Prototypes ----------- */

READ_FUNCTION(FS_r_page);
WRITE_FUNCTION(FS_w_page);
READ_FUNCTION(FS_r_ident);
WRITE_FUNCTION(FS_w_ident);
READ_FUNCTION(FS_r_memory);
WRITE_FUNCTION(FS_w_memory);
WRITE_FUNCTION(FS_w_password);
WRITE_FUNCTION(FS_password);
WRITE_FUNCTION(FS_reset);
READ_FUNCTION(FS_r_subkey);
WRITE_FUNCTION(FS_w_subkey);
READ_FUNCTION(FS_r_id);
WRITE_FUNCTION(FS_w_id);
WRITE_FUNCTION(FS_w_reset_password);
WRITE_FUNCTION(FS_w_change_password);

#define _DS1991_PAGES	3
#define _DS1991_DATA_START   0x10
#define _DS1991_PADE_LENGTH 0x40
#define _DS1991_PAGESIZE   (_DS1991_PADE_LENGTH-_DS1991_DATA_START)
#define _DS1991_ID_START   0x00
#define _DS1991_PWD_START   0x08
#define _DS1991_PASSWORD_START   0x08

#define _DS1991_PASSWORD_LENGTH   8
#define _DS1991_ID_LENGTH   8

BYTE subkey_byte[3] = { 0x00, 0x40, 0x80, } ;


/* ------- Structures ----------- */

static struct aggregate A1991_password = { 0, ag_letters, ag_sparse, };
static struct aggregate A1991 = { _DS1991_PAGES, ag_numbers, ag_separate, };
static struct filetype DS1991[] = {
	F_STANDARD,
	{"memory", 144, NON_AGGREGATE, ft_binary, fc_link, FS_r_memory, FS_w_memory, INVISIBLE, NO_FILETYPE_DATA, },
	{"pages", PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir, fc_subdir, NO_READ_FUNCTION, NO_WRITE_FUNCTION, INVISIBLE, NO_FILETYPE_DATA, },
	{"pages/page", 48, &A1991, ft_binary, fc_page, FS_r_page, FS_w_page, INVISIBLE, NO_FILETYPE_DATA, },
	{"pages/password", 8, &A1991, ft_binary, fc_stable, NO_READ_FUNCTION, FS_w_password, INVISIBLE, NO_FILETYPE_DATA, },
	{"pages/ident", 8, &A1991, ft_binary, fc_volatile, FS_r_ident, FS_w_ident, INVISIBLE, NO_FILETYPE_DATA, },

	{"subkey0", PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir, fc_subdir, NO_READ_FUNCTION, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA, },
	{"subkey0/password", _DS1991_ID_LENGTH, &A1991_password, ft_binary, fc_stable, NO_READ_FUNCTION, FS_password, VISIBLE,  {.i=0,}, },
	{"subkey0/reset", PROPERTY_LENGTH_YESNO, &A1991_password, ft_yesno, fc_stable, NO_READ_FUNCTION, FS_reset, VISIBLE,  {.i=0,}, },
	{"subkey0/secure_data", _DS1991_PAGESIZE, &A1991_password, ft_binary, fc_stable, FS_r_subkey, FS_w_subkey, VISIBLE,  {.i=0,}, },
	{"subkey0/id", _DS1991_ID_LENGTH, &A1991_password, ft_binary, fc_stable, FS_r_id, FS_w_id, VISIBLE,  {.i=0,}, },

	{"subkey1", PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir, fc_subdir, NO_READ_FUNCTION, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA, },
	{"subkey1/password", _DS1991_ID_LENGTH, &A1991_password, ft_binary, fc_stable, NO_READ_FUNCTION, FS_password, VISIBLE,  {.i=1,}, },
	{"subkey1/reset", PROPERTY_LENGTH_YESNO, &A1991_password, ft_yesno, fc_stable, NO_READ_FUNCTION, FS_reset, VISIBLE,  {.i=1,}, },
	{"subkey1/secure_data", _DS1991_PAGESIZE, &A1991_password, ft_binary, fc_stable, FS_r_subkey, FS_w_subkey, VISIBLE,  {.i=1,}, },
	{"subkey1/id", _DS1991_ID_LENGTH, &A1991_password, ft_binary, fc_stable, FS_r_id, FS_w_id, VISIBLE,  {.i=1,}, },

	{"subkey2", PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir, fc_subdir, NO_READ_FUNCTION, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA, },
	{"subkey2/password", _DS1991_ID_LENGTH, &A1991_password, ft_binary, fc_stable, NO_READ_FUNCTION, FS_password, VISIBLE,  {.i=2,}, },
	{"subkey2/reset", PROPERTY_LENGTH_YESNO, &A1991_password, ft_yesno, fc_stable, NO_READ_FUNCTION, FS_reset, VISIBLE,  {.i=2,}, },
	{"subkey2/secure_data", _DS1991_PAGESIZE, &A1991_password, ft_binary, fc_stable, FS_r_subkey, FS_w_subkey, VISIBLE,  {.i=2,}, },
	{"subkey2/id", _DS1991_ID_LENGTH, &A1991_password, ft_binary, fc_stable, FS_r_id, FS_w_id, VISIBLE,  {.i=2,}, },

	{"settings", PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir, fc_subdir, NO_READ_FUNCTION, NO_WRITE_FUNCTION, INVISIBLE, NO_FILETYPE_DATA, },
	{"settings/reset_password", 8, &A1991, ft_binary, fc_stable, NO_READ_FUNCTION, FS_w_reset_password, INVISIBLE, NO_FILETYPE_DATA, },
	{"settings/change_password", 8, &A1991, ft_binary, fc_stable, NO_READ_FUNCTION, FS_w_change_password, INVISIBLE, NO_FILETYPE_DATA, },
	{"settings/page", 48, &A1991, ft_binary, fc_volatile, FS_r_page, FS_w_page, INVISIBLE, NO_FILETYPE_DATA, },
};

DeviceEntry(02, DS1991, NO_GENERIC_READ, NO_GENERIC_WRITE);

static struct filetype DS1425[] = {
	F_STANDARD,
	{"memory", 144, NON_AGGREGATE, ft_binary, fc_link, FS_r_memory, FS_w_memory, INVISIBLE, NO_FILETYPE_DATA, },
	{"pages", PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir, fc_subdir, NO_READ_FUNCTION, NO_WRITE_FUNCTION, INVISIBLE, NO_FILETYPE_DATA, },
	{"pages/page", 48, &A1991, ft_binary, fc_page, FS_r_page, FS_w_page, INVISIBLE, NO_FILETYPE_DATA, },
	{"pages/password", 8, &A1991, ft_binary, fc_stable, NO_READ_FUNCTION, FS_w_password, INVISIBLE, NO_FILETYPE_DATA, },
	{"pages/ident", 8, &A1991, ft_binary, fc_volatile, FS_r_ident, FS_w_ident, INVISIBLE, NO_FILETYPE_DATA, },

	{"subkey0", PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir, fc_subdir, NO_READ_FUNCTION, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA, },
	{"subkey0/password", _DS1991_ID_LENGTH, &A1991_password, ft_binary, fc_stable, NO_READ_FUNCTION, FS_password, VISIBLE,  {.i=0,}, },
	{"subkey0/reset", PROPERTY_LENGTH_YESNO, &A1991_password, ft_yesno, fc_stable, NO_READ_FUNCTION, FS_reset, VISIBLE,  {.i=0,}, },
	{"subkey0/secure_data", _DS1991_PAGESIZE, &A1991_password, ft_binary, fc_stable, FS_r_subkey, FS_w_subkey, VISIBLE,  {.i=0,}, },
	{"subkey0/id", _DS1991_ID_LENGTH, &A1991_password, ft_binary, fc_stable, FS_r_id, FS_w_id, VISIBLE,  {.i=0,}, },

	{"subkey1", PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir, fc_subdir, NO_READ_FUNCTION, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA, },
	{"subkey1/password", _DS1991_ID_LENGTH, &A1991_password, ft_binary, fc_stable, NO_READ_FUNCTION, FS_password, VISIBLE,  {.i=1,}, },
	{"subkey1/reset", PROPERTY_LENGTH_YESNO, &A1991_password, ft_yesno, fc_stable, NO_READ_FUNCTION, FS_reset, VISIBLE,  {.i=1,}, },
	{"subkey1/secure_data", _DS1991_PAGESIZE, &A1991_password, ft_binary, fc_stable, FS_r_subkey, FS_w_subkey, VISIBLE,  {.i=1,}, },
	{"subkey1/id", _DS1991_ID_LENGTH, &A1991_password, ft_binary, fc_stable, FS_r_id, FS_w_id, VISIBLE,  {.i=1,}, },

	{"subkey2", PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir, fc_subdir, NO_READ_FUNCTION, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA, },
	{"subkey2/password", _DS1991_ID_LENGTH, &A1991_password, ft_binary, fc_stable, NO_READ_FUNCTION, FS_password, VISIBLE,  {.i=2,}, },
	{"subkey2/reset", PROPERTY_LENGTH_YESNO, &A1991_password, ft_yesno, fc_stable, NO_READ_FUNCTION, FS_reset, VISIBLE,  {.i=2,}, },
	{"subkey2/secure_data", _DS1991_PAGESIZE, &A1991_password, ft_binary, fc_stable, FS_r_subkey, FS_w_subkey, VISIBLE,  {.i=2,}, },
	{"subkey2/id", _DS1991_ID_LENGTH, &A1991_password, ft_binary, fc_stable, FS_r_id, FS_w_id, VISIBLE,  {.i=2,}, },

	{"settings", PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir, fc_subdir, NO_READ_FUNCTION, NO_WRITE_FUNCTION, INVISIBLE, NO_FILETYPE_DATA, },
	{"settings/reset_password", 8, &A1991, ft_binary, fc_stable, NO_READ_FUNCTION, FS_w_reset_password, INVISIBLE, NO_FILETYPE_DATA, },
	{"settings/change_password", 8, &A1991, ft_binary, fc_stable, NO_READ_FUNCTION, FS_w_change_password, INVISIBLE, NO_FILETYPE_DATA, },
	{"settings/page", 48, &A1991, ft_binary, fc_volatile, FS_r_page, FS_w_page, INVISIBLE, NO_FILETYPE_DATA, },
};

DeviceEntry(82, DS1425, NO_GENERIC_READ, NO_GENERIC_WRITE);

#define _1W_WRITE_SCRATCHPAD 0x96
#define _1W_READ_SCRATCHPAD 0x69
#define _1W_COPY_SCRATCHPAD 0x3C
#define _1W_WRITE_PASSWORD 0x5A
#define _1W_WRITE_SUBKEY 0x99
#define _1W_READ_SUBKEY 0x66

static BYTE global_passwd[_DS1991_PAGES][8] = { "", "", "" };

/* ------- Functions ------------ */


static GOOD_OR_BAD OW_r_ident(BYTE * data, const size_t size, const off_t offset, const struct parsedname *pn);
static GOOD_OR_BAD OW_w_ident(const BYTE * data, const size_t size, const off_t offset, const struct parsedname *pn);
static GOOD_OR_BAD OW_w_reset_password(const BYTE * data, const size_t size, const off_t offset, const struct parsedname *pn);
static GOOD_OR_BAD OW_w_change_password(const BYTE * data, const size_t size, const off_t offset, const struct parsedname *pn);
static GOOD_OR_BAD OW_r_page(BYTE * data, const size_t size, const off_t offset, const struct parsedname *pn);
static GOOD_OR_BAD OW_w_page(const BYTE * data, const size_t size, const off_t offset, const struct parsedname *pn);
static GOOD_OR_BAD OW_r_memory(BYTE * data, const size_t size, const off_t offset, const struct parsedname *pn);
static GOOD_OR_BAD OW_w_memory(const BYTE * data, const size_t size, const off_t offset, const struct parsedname *pn);
static GOOD_OR_BAD OW_r_subkey(BYTE * data, const size_t size, const off_t offset, const struct parsedname *pn, const int extension);
static GOOD_OR_BAD OW_w_subkey(const BYTE * data, const size_t size, const off_t offset, const struct parsedname *pn, const int extension);
static GOOD_OR_BAD OW_password( int subkey, const BYTE * new_id, const BYTE * new_password, const struct parsedname * pn ) ;
static GOOD_OR_BAD OW_read_subkey( int subkey, BYTE * password, size_t size, off_t offset, BYTE * data, const struct parsedname * pn ) ;
static GOOD_OR_BAD OW_write_subkey( int subkey, BYTE * password, size_t size, off_t offset, BYTE * data, const struct parsedname * pn ) ;
static GOOD_OR_BAD OW_read_id( int subkey, BYTE * id, const struct parsedname * pn ) ;
static GOOD_OR_BAD OW_write_id( int subkey, BYTE * password, BYTE * id, const struct parsedname * pn ) ;
static GOOD_OR_BAD OW_write_password( int subkey, BYTE * old_password, BYTE * new_password, const struct parsedname * pn ) ;

static GOOD_OR_BAD ToPassword( char * text, BYTE * psw ) ;


/* array with magic bytes representing the Copy Scratch operations */
enum block { block_ALL = 0, block_IDENT, block_PASSWORD, block_DATA };
#define _DS1991_COPY_BLOCK_LENGTH	8
#define _DS1991_COPY_BLOCK_ITEMS	9

static const BYTE cp_array[_DS1991_COPY_BLOCK_ITEMS][_DS1991_COPY_BLOCK_LENGTH] = {
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

static ZERO_OR_ERROR FS_w_password(struct one_wire_query *owq)
{
	struct parsedname *pn = PN(owq);
	size_t s = OWQ_size(owq);

	if (s > 8) {
		s = 8;
	}
	memset(global_passwd[pn->extension], 0, 8);
	memcpy(global_passwd[pn->extension], (BYTE *) OWQ_buffer(owq), s);
	//printf("Use password [%s] for subkey %d\n", global_passwd[pn->extension], pn->extension);
	return 0;
}

// reset the subkey area with a new password
// clear all data
// put default ID in it.
static ZERO_OR_ERROR FS_reset(struct one_wire_query *owq)
{
	struct parsedname *pn = PN(owq);
	int subkey = pn->selected_filetype->data.i ;
	BYTE new_password[_DS1991_PASSWORD_LENGTH] ;
	BYTE new_id[_DS1991_PASSWORD_LENGTH + 1 ] ;
	
	if ( OWQ_Y(owq) == 0 ) {
		// Not true request
		return 0 ;
	}
		
	if ( BAD(ToPassword( pn->sparse_name, new_password ) ) ) {
		return -EINVAL ;
	}
	
	snprintf( (char *) new_id, _DS1991_ID_LENGTH + 1, "Subkey %.1d", subkey  ) ;
	
	return GB_to_Z_OR_E(OW_password(subkey, new_id, new_password, pn)) ;
}

static ZERO_OR_ERROR FS_password(struct one_wire_query *owq)
{
	struct parsedname *pn = PN(owq);
	int subkey = pn->selected_filetype->data.i ;
	BYTE new_password[_DS1991_PASSWORD_LENGTH] ;
	BYTE old_password[_DS1991_PASSWORD_LENGTH] ;
	
	if ( OWQ_offset(owq) != 0 ) {
		return -EINVAL ;
	}
	
	if ( OWQ_size(owq) != _DS1991_PASSWORD_LENGTH ) {
		return -EINVAL ;
	}
	
	memcpy( new_password, (BYTE *)OWQ_buffer(owq) , _DS1991_PASSWORD_LENGTH ) ;
	if ( BAD(ToPassword( pn->sparse_name, old_password ) ) ) {
		return -EINVAL ;
	}
	
	return GB_to_Z_OR_E(OW_write_password(subkey, old_password, new_password, pn)) ;
}

static ZERO_OR_ERROR FS_w_subkey(struct one_wire_query *owq)
{
	struct parsedname *pn = PN(owq);
	int subkey = pn->selected_filetype->data.i ;
	BYTE password[_DS1991_PASSWORD_LENGTH] ;
	
	if ( BAD(ToPassword( pn->sparse_name, password ) ) ) {
		return -EINVAL ;
	}

	if ( BAD (OW_write_subkey( subkey, password, OWQ_size(owq), OWQ_offset(owq)+_DS1991_DATA_START, (BYTE *) OWQ_buffer(owq), pn )) ) {
		return -EINVAL ;
	}

	return 0 ;
}

static ZERO_OR_ERROR FS_r_subkey(struct one_wire_query *owq)
{
	struct parsedname *pn = PN(owq);
	int subkey = pn->selected_filetype->data.i ;
	BYTE password[_DS1991_PASSWORD_LENGTH] ;
	
	if ( BAD(ToPassword( pn->sparse_name, password ) ) ) {
		return -EINVAL ;
	}

	if ( BAD (OW_read_subkey( subkey, password, OWQ_size(owq), OWQ_offset(owq)+_DS1991_DATA_START, (BYTE *) OWQ_buffer(owq), pn )) ) {
		return -EINVAL ;
	}
	OWQ_length(owq) = OWQ_size(owq) ;

	return 0 ;
}

static ZERO_OR_ERROR FS_r_id(struct one_wire_query *owq)
{
	struct parsedname *pn = PN(owq);
	int subkey = pn->selected_filetype->data.i ;
	BYTE id[_DS1991_ID_LENGTH] ;

	if ( BAD (OW_read_id( subkey, id, pn )) ) {
		return -EINVAL ;
	}

	return OWQ_format_output_offset_and_size( (const char *) id, _DS1991_ID_LENGTH, owq ) ;
}

static ZERO_OR_ERROR FS_w_id(struct one_wire_query *owq)
{
	struct parsedname *pn = PN(owq);
	int subkey = pn->selected_filetype->data.i ;
	BYTE password[_DS1991_PASSWORD_LENGTH] ;
	BYTE id[_DS1991_ID_LENGTH] ;
	
	if ( BAD(ToPassword( pn->sparse_name, password ) ) ) {
		return -EINVAL ;
	}

	if ( OWQ_offset(owq) != 0 && OWQ_size(owq) != _DS1991_ID_LENGTH ) {
		// Fill id  with existing id
		if ( BAD (OW_read_id( subkey, id, pn )) ) {
			return -EINVAL ;
		}
	}
	
	memcpy( &id[OWQ_offset(owq)], OWQ_buffer(owq), OWQ_size(owq) ) ;
	
	if ( BAD (OW_write_id( subkey, password, id, pn )) ) {
		return -EINVAL ;
	}
	
	return 0 ;
}

static ZERO_OR_ERROR FS_w_reset_password(struct one_wire_query *owq)
{
	return GB_to_Z_OR_E(OW_w_reset_password((BYTE *) OWQ_buffer(owq), OWQ_size(owq), (size_t) OWQ_offset(owq), PN(owq))) ;
}

static ZERO_OR_ERROR FS_w_change_password(struct one_wire_query *owq)
{
	return GB_to_Z_OR_E(OW_w_change_password((BYTE *) OWQ_buffer(owq), OWQ_size(owq), (size_t) OWQ_offset(owq), PN(owq))) ;
}

static ZERO_OR_ERROR FS_r_page(struct one_wire_query *owq)
{
	return GB_to_Z_OR_E(OW_r_page((BYTE *) OWQ_buffer(owq), OWQ_size(owq), (size_t) OWQ_offset(owq), PN(owq))) ;
}

static ZERO_OR_ERROR FS_w_page(struct one_wire_query *owq)
{
	return GB_to_Z_OR_E(OW_w_page((BYTE *) OWQ_buffer(owq), OWQ_size(owq), (size_t) OWQ_offset(owq), PN(owq))) ;
}

static ZERO_OR_ERROR FS_r_ident(struct one_wire_query *owq)
{
	return GB_to_Z_OR_E(OW_r_ident((BYTE *) OWQ_buffer(owq), OWQ_size(owq), (size_t) OWQ_offset(owq), PN(owq))) ;
}

static ZERO_OR_ERROR FS_w_ident(struct one_wire_query *owq)
{
	if (OWQ_offset(owq)) {
		return -EINVAL;
	}
	return GB_to_Z_OR_E(OW_w_ident((BYTE *) OWQ_buffer(owq), OWQ_size(owq), (size_t) OWQ_offset(owq), PN(owq))) ;
}

static ZERO_OR_ERROR FS_r_memory(struct one_wire_query *owq)
{
	OWQ_length(owq) = OWQ_size(owq) ;
	return GB_to_Z_OR_E(OW_r_memory((BYTE *) OWQ_buffer(owq), OWQ_size(owq), (size_t) OWQ_offset(owq), PN(owq))) ;
}

static ZERO_OR_ERROR FS_w_memory(struct one_wire_query *owq)
{
	return GB_to_Z_OR_E(OW_w_memory((BYTE *) OWQ_buffer(owq), OWQ_size(owq), (size_t) OWQ_offset(owq), PN(owq))) ;
}

static GOOD_OR_BAD OW_w_reset_password(const BYTE * data, const size_t size, const off_t offset, const struct parsedname *pn)
{
	BYTE set_password[_DS1991_PAGES] = { _1W_WRITE_PASSWORD, 0x00, 0x00 };
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


	if (offset) {
		return gbBAD;
	}

	memset(passwd, 0, 8);
	memcpy(passwd, data, MIN(size, 8));
	set_password[1] = pn->extension << 6;
	set_password[2] = ~(set_password[1]);
	RETURN_BAD_IF_BAD(BUS_transaction(tscratch, pn)) ;

	/* Verification is done... now send ident + password
	 * Note: ALL saved data in subkey will be deleted during this operation
	 */

	memcpy(global_passwd[pn->extension], passwd, 8);
	return gbGOOD;
}

static GOOD_OR_BAD OW_w_subkey(const BYTE * data, const size_t size, const off_t offset, const struct parsedname *pn, const int extension)
{
	BYTE p[4] = { _1W_WRITE_SUBKEY, 0x00, 0x00, 0x00 };	// write subkey
	BYTE ident[8];
	struct transaction_log tscratch[] = {
		TRXN_START,
		TRXN_WRITE3(p),
		TRXN_READ(ident, 8),
		TRXN_WRITE(global_passwd[extension], 8),
		TRXN_WRITE(&data[0x10], size - 0x10),
		TRXN_END,
	};

	if ((size <= 0x10) || (size > 0x40)) {
		return gbBAD;
	}

	p[1] = extension << 6;
	p[1] |= (0x10 + offset);	// + 0x10 -> 0x3F
	p[2] = ~(p[1]);

	return BUS_transaction(tscratch, pn) ;
}

static GOOD_OR_BAD OW_r_subkey(BYTE * data, const size_t size, const off_t offset, const struct parsedname *pn, const int extension)
{
	BYTE p[_DS1991_PAGES] = { _1W_READ_SUBKEY, 0x00, 0x00 };	// read subkey
	struct transaction_log tscratch[] = {
		TRXN_START,
		TRXN_WRITE3(p),
		TRXN_READ(data, 8),
		TRXN_WRITE(global_passwd[extension], 8),
		TRXN_WRITE(&data[0x10 + offset], 0x30 - offset),
		TRXN_END,
	};

	memset(data, 0, 0x40);
	memcpy(&data[8], global_passwd[extension], 8);
	if (offset >= 0x30) {
		return gbBAD;
	}

	if (size != 0x40) {
		/* Only allow reading whole subkey right now */
		return gbBAD;
	}

	p[1] = extension << 6;
	p[1] |= (offset + 0x10);
	p[2] = ~(p[1]);

	return BUS_transaction(tscratch, pn) ;
}

static GOOD_OR_BAD OW_r_memory(BYTE * data, const size_t size, const off_t offset, const struct parsedname *pn)
{
	BYTE all_data[0x40];
	int i, nr_bytes;
	size_t left = size;

	if (offset) {
		return gbBAD;
	}

	for (i = 0; (i < _DS1991_PAGES) && (left > 0); i++) {
		memset(all_data, 0, 0x40);
		RETURN_BAD_IF_BAD( OW_r_subkey(all_data, 0x40, 0, pn, i) );
		nr_bytes = MIN(0x30, left);
		memcpy(&data[i * 0x30], &all_data[0x10], nr_bytes);
		left -= nr_bytes;
	}
	return gbGOOD;
}

static GOOD_OR_BAD OW_w_memory(const BYTE * data, const size_t size, const off_t offset, const struct parsedname *pn)
{
	BYTE all_data[0x40];
	int i, nr_bytes;
	size_t left = size;

	if (offset) {
		return gbBAD;
	}

	for (i = 0; (i < _DS1991_PAGES) && (left > 0); i++) {
		memset(all_data, 0, 0x40);
		RETURN_BAD_IF_BAD( OW_r_subkey(all_data, 0x40, 0, pn, i) );
		nr_bytes = MIN(0x30, left);
		memcpy(&all_data[0x10], &data[size - left], nr_bytes);
		RETURN_BAD_IF_BAD( OW_w_subkey(all_data, 0x40, 0, pn, i) );
		left -= nr_bytes;
	}
	return gbGOOD;
}

// size and offset already bounds checked
static GOOD_OR_BAD OW_r_ident(BYTE * data, const size_t size, const off_t offset, const struct parsedname *pn)
{
	BYTE all_data[0x40];

	RETURN_BAD_IF_BAD(OW_r_subkey(all_data, 0x40, 0, pn, pn->extension)) ;

	memcpy(data, &all_data[offset], size);
	return gbGOOD;
}

static GOOD_OR_BAD OW_w_ident(const BYTE * data, const size_t size, const off_t offset, const struct parsedname *pn)
{
	BYTE write_scratch[_DS1991_PAGES] = { _1W_WRITE_SCRATCHPAD, 0x00, 0x00 };
	BYTE copy_scratch[_DS1991_PAGES] = { _1W_COPY_SCRATCHPAD, 0x00, 0x00 };
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
		TRXN_WRITE(cp_array[block_IDENT], 8),
		TRXN_WRITE(global_passwd[pn->extension], 8),
		TRXN_END,
	};

	write_scratch[1] = 0xC0 + offset;
	write_scratch[2] = ~(write_scratch[1]);
	memset(all_data, 0, 0x40);
	memcpy(all_data, data, MIN(size, 8));
	RETURN_BAD_IF_BAD(BUS_transaction(tscratch, pn)) ;

	copy_scratch[1] = pn->extension << 6;
	copy_scratch[2] = ~(copy_scratch[1]);
	RETURN_BAD_IF_BAD(BUS_transaction(tcopy, pn)) ;

	return gbGOOD;
}

static GOOD_OR_BAD OW_w_change_password(const BYTE * data, const size_t size, const off_t offset, const struct parsedname *pn)
{
	BYTE write_scratch[_DS1991_PAGES] = { _1W_WRITE_SCRATCHPAD, 0x00, 0x00 };
	BYTE copy_scratch[_DS1991_PAGES] = { _1W_COPY_SCRATCHPAD, 0x00, 0x00 };
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
		TRXN_WRITE(cp_array[block_PASSWORD], 8),
		TRXN_WRITE(global_passwd[pn->extension], 8),
		TRXN_END,
	};

	write_scratch[1] = 0xC0 + offset;
	write_scratch[2] = ~(write_scratch[1]);
	memset(all_data, 0, 0x40);
	memcpy(&all_data[0x08], data, MIN(size, 8));
	RETURN_BAD_IF_BAD(BUS_transaction(tscratch, pn)) ;

	copy_scratch[1] = pn->extension << 6;
	copy_scratch[2] = ~(copy_scratch[1]);

	RETURN_BAD_IF_BAD(BUS_transaction(tcopy, pn)) ;
	memcpy(global_passwd[pn->extension], &all_data[0x08], 8);

	return gbGOOD;
}

static GOOD_OR_BAD OW_r_page(BYTE * data, const size_t size, const off_t offset, const struct parsedname *pn)
{
	BYTE all_data[0x40];

	if (offset > 0x2F) {
		return gbBAD;
	}

	RETURN_BAD_IF_BAD( OW_r_subkey(all_data, 0x40, 0, pn, pn->extension) );
	memcpy(data, &all_data[0x10 + offset], size);
	return gbGOOD;
}

static GOOD_OR_BAD OW_w_page(const BYTE * data, const size_t size, const off_t offset, const struct parsedname *pn)
{
	BYTE write_scratch[_DS1991_PAGES] = { _1W_WRITE_SCRATCHPAD, 0xC0, BYTE_INVERSE(0xC0) };
	BYTE copy_scratch[_DS1991_PAGES] = { _1W_COPY_SCRATCHPAD, 0x00, 0x00 };
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
		return gbBAD;
	}
	memset(all_data, 0, 0x40);
	memcpy(&all_data[0x10], data, size);

	RETURN_BAD_IF_BAD(BUS_transaction(tscratch, pn)) ;

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
		tcopy[2].out = cp_array[block_IDENT+i];
		RETURN_BAD_IF_BAD(BUS_transaction(tcopy, pn)) ;
		left -= nr_bytes;
	}
	return gbGOOD;
}

static GOOD_OR_BAD OW_password( int subkey, const BYTE * new_id, const BYTE * new_password, const struct parsedname * pn )
{
	BYTE subkey_addr = subkey_byte[ subkey ] ;
	BYTE write_pwd[] = { _1W_WRITE_PASSWORD, subkey_addr, BYTE_INVERSE(subkey_addr), } ;
	BYTE old_id[_DS1991_ID_LENGTH] ;
	struct transaction_log t[] = {
		TRXN_START,
		TRXN_WRITE3(write_pwd),
		TRXN_READ(old_id, _DS1991_ID_LENGTH),
		TRXN_WRITE(old_id, _DS1991_ID_LENGTH),
		TRXN_WRITE(new_id, _DS1991_ID_LENGTH),
		TRXN_WRITE(new_password, _DS1991_PASSWORD_LENGTH),
		TRXN_END,
	};
	
	return BUS_transaction(t, pn) ;
}


static GOOD_OR_BAD OW_read_id( int subkey, BYTE * id, const struct parsedname * pn )
{
	BYTE subkey_addr = subkey_byte[ subkey ] ;
	BYTE write_sbk[] = { _1W_WRITE_SUBKEY, subkey_addr, BYTE_INVERSE(subkey_addr), } ;
	struct transaction_log t[] = {
		TRXN_START,
		TRXN_WRITE3(write_sbk),
		TRXN_READ(id, _DS1991_ID_LENGTH),
		TRXN_END,
	};
	
	return BUS_transaction(t, pn) ;
}

static GOOD_OR_BAD OW_read_subkey( int subkey, BYTE * password, size_t size, off_t offset, BYTE * data, const struct parsedname * pn )
{
	BYTE subkey_addr = subkey_byte[ subkey ] + offset ;
	BYTE write_sbk[] = { _1W_READ_SUBKEY, subkey_addr, BYTE_INVERSE(subkey_addr), } ;
	BYTE old_id[_DS1991_ID_LENGTH] ;
	struct transaction_log t[] = {
		TRXN_START,
		TRXN_WRITE3(write_sbk),
		TRXN_READ(old_id, _DS1991_ID_LENGTH),
		TRXN_WRITE(password, _DS1991_PASSWORD_LENGTH),
		TRXN_WRITE(data, size),
		TRXN_END,
	};
	
	return BUS_transaction(t, pn) ;
}

static GOOD_OR_BAD OW_write_subkey( int subkey, BYTE * password, size_t size, off_t offset, BYTE * data, const struct parsedname * pn )
{
	BYTE subkey_addr = subkey_byte[ subkey ] + offset ;
	BYTE write_sbk[] = { _1W_READ_SUBKEY, subkey_addr, BYTE_INVERSE(subkey_addr), } ;
	BYTE old_id[_DS1991_ID_LENGTH] ;
	struct transaction_log t[] = {
		TRXN_START,
		TRXN_WRITE3(write_sbk),
		TRXN_READ(old_id, _DS1991_ID_LENGTH),
		TRXN_WRITE(password, _DS1991_PASSWORD_LENGTH),
		TRXN_READ(data, size),
		TRXN_END,
	};
	
	return BUS_transaction(t, pn) ;
}

static GOOD_OR_BAD OW_write_id( int subkey, BYTE * password, BYTE * id, const struct parsedname * pn )
{
	BYTE scratch_addr = 0x0C + _DS1991_ID_START ;
	BYTE write_scratch[] = { _1W_WRITE_SCRATCHPAD, scratch_addr, BYTE_INVERSE(scratch_addr), } ;
	BYTE copy_addr = subkey_byte[ subkey ] ;
	BYTE copy_scratch[] = { _1W_COPY_SCRATCHPAD, copy_addr, BYTE_INVERSE(copy_addr), } ;
	struct transaction_log twrite[] = {
		TRXN_START,
		TRXN_WRITE3(write_scratch),
		TRXN_WRITE(id, _DS1991_ID_LENGTH),
		TRXN_END,
	};
	struct transaction_log tcopy[] = {
		TRXN_START,
		TRXN_WRITE3(copy_scratch),
		TRXN_WRITE(cp_array[0], _DS1991_COPY_BLOCK_LENGTH),
		TRXN_WRITE(password, _DS1991_PASSWORD_LENGTH),
		TRXN_END,
	};
	
	RETURN_BAD_IF_BAD( BUS_transaction(twrite,pn) ) ;
	return BUS_transaction(tcopy, pn) ;
}

static GOOD_OR_BAD OW_write_password( int subkey, BYTE * old_password, BYTE * new_password, const struct parsedname * pn )
{
	BYTE scratch_addr = 0x0C + _DS1991_PASSWORD_START ;
	BYTE write_scratch[] = { _1W_WRITE_SCRATCHPAD, scratch_addr, BYTE_INVERSE(scratch_addr), } ;
	BYTE copy_addr = subkey_byte[ subkey ] ;
	BYTE copy_scratch[] = { _1W_COPY_SCRATCHPAD, copy_addr, BYTE_INVERSE(copy_addr), } ;
	struct transaction_log twrite[] = {
		TRXN_START,
		TRXN_WRITE3(write_scratch),
		TRXN_WRITE(new_password, _DS1991_PASSWORD_LENGTH),
		TRXN_END,
	};
	struct transaction_log tcopy[] = {
		TRXN_START,
		TRXN_WRITE3(copy_scratch),
		TRXN_WRITE(cp_array[1], _DS1991_COPY_BLOCK_LENGTH),
		TRXN_WRITE(old_password, _DS1991_PASSWORD_LENGTH),
		TRXN_END,
	};
	
	RETURN_BAD_IF_BAD( BUS_transaction(twrite,pn) ) ;
	return BUS_transaction(tcopy, pn) ;
}

static GOOD_OR_BAD ToPassword( char * text, BYTE * psw )
{
	unsigned int  text_length = _DS1991_PASSWORD_LENGTH * 2 ;
	char convert_text[ text_length + 1 ] ;
	
	memset( convert_text, '0', text_length ) ;
	convert_text[text_length] = '\0' ;
	
	if ( text == NULL ) {
		return gbBAD ;
	}
	
	if ( strlen( text ) > text_length ) {
		LEVEL_DEBUG("Password extension <%s> longer than %d bytes" , text, _DS1991_PASSWORD_LENGTH ) ;
		return gbBAD ;
	}
	
	strcpy( & convert_text[ text_length - strlen(text) ] , text ) ;
	string2bytes( convert_text, psw, _DS1991_PASSWORD_LENGTH ) ;
	return gbGOOD ;
}

	
