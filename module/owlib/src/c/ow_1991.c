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

WRITE_FUNCTION(FS_password);
WRITE_FUNCTION(FS_reset);
READ_FUNCTION(FS_r_subkey);
WRITE_FUNCTION(FS_w_subkey);
READ_FUNCTION(FS_r_id);
WRITE_FUNCTION(FS_w_id);

#define _DS1991_PAGES	3
#define _DS1991_PAGE_LENGTH 0x40

#define _DS1991_ID_START   0x00
#define _DS1991_ID_LENGTH   8

#define _DS1991_PASSWORD_START   (_DS1991_ID_START + _DS1991_ID_LENGTH )
#define _DS1991_PASSWORD_LENGTH   8

#define _DS1991_DATA_START   ( _DS1991_PASSWORD_START + _DS1991_PASSWORD_LENGTH )
#define _DS1991_DATA_LENGTH   (_DS1991_PAGE_LENGTH - _DS1991_DATA_START)


BYTE subkey_byte[3] = { (0<<6), (1<<6), (2<<6), } ;


/* ------- Structures ----------- */

static struct aggregate A1991_password = { 0, ag_letters, ag_sparse, };
static struct filetype DS1991[] = {
	F_STANDARD,

	{"subkey0", PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir, fc_subdir, NO_READ_FUNCTION, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA, },
	{"subkey0/password", _DS1991_ID_LENGTH, &A1991_password, ft_binary, fc_stable, NO_READ_FUNCTION, FS_password, VISIBLE,  {.i=0,}, },
	{"subkey0/reset", PROPERTY_LENGTH_YESNO, &A1991_password, ft_yesno, fc_stable, NO_READ_FUNCTION, FS_reset, VISIBLE,  {.i=0,}, },
	{"subkey0/secure_data", _DS1991_DATA_LENGTH, &A1991_password, ft_binary, fc_stable, FS_r_subkey, FS_w_subkey, VISIBLE,  {.i=0,}, },
	{"subkey0/id", _DS1991_ID_LENGTH, &A1991_password, ft_binary, fc_stable, FS_r_id, FS_w_id, VISIBLE,  {.i=0,}, },

	{"subkey1", PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir, fc_subdir, NO_READ_FUNCTION, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA, },
	{"subkey1/password", _DS1991_ID_LENGTH, &A1991_password, ft_binary, fc_stable, NO_READ_FUNCTION, FS_password, VISIBLE,  {.i=1,}, },
	{"subkey1/reset", PROPERTY_LENGTH_YESNO, &A1991_password, ft_yesno, fc_stable, NO_READ_FUNCTION, FS_reset, VISIBLE,  {.i=1,}, },
	{"subkey1/secure_data", _DS1991_DATA_LENGTH, &A1991_password, ft_binary, fc_stable, FS_r_subkey, FS_w_subkey, VISIBLE,  {.i=1,}, },
	{"subkey1/id", _DS1991_ID_LENGTH, &A1991_password, ft_binary, fc_stable, FS_r_id, FS_w_id, VISIBLE,  {.i=1,}, },

	{"subkey2", PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir, fc_subdir, NO_READ_FUNCTION, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA, },
	{"subkey2/password", _DS1991_ID_LENGTH, &A1991_password, ft_binary, fc_stable, NO_READ_FUNCTION, FS_password, VISIBLE,  {.i=2,}, },
	{"subkey2/reset", PROPERTY_LENGTH_YESNO, &A1991_password, ft_yesno, fc_stable, NO_READ_FUNCTION, FS_reset, VISIBLE,  {.i=2,}, },
	{"subkey2/secure_data", _DS1991_DATA_LENGTH, &A1991_password, ft_binary, fc_stable, FS_r_subkey, FS_w_subkey, VISIBLE,  {.i=2,}, },
	{"subkey2/id", _DS1991_ID_LENGTH, &A1991_password, ft_binary, fc_stable, FS_r_id, FS_w_id, VISIBLE,  {.i=2,}, },
};

DeviceEntry(02, DS1991, NO_GENERIC_READ, NO_GENERIC_WRITE);

static struct filetype DS1425[] = {
	F_STANDARD,

	{"subkey0", PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir, fc_subdir, NO_READ_FUNCTION, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA, },
	{"subkey0/password", _DS1991_ID_LENGTH, &A1991_password, ft_binary, fc_stable, NO_READ_FUNCTION, FS_password, VISIBLE,  {.i=0,}, },
	{"subkey0/reset", PROPERTY_LENGTH_YESNO, &A1991_password, ft_yesno, fc_stable, NO_READ_FUNCTION, FS_reset, VISIBLE,  {.i=0,}, },
	{"subkey0/secure_data", _DS1991_DATA_LENGTH, &A1991_password, ft_binary, fc_stable, FS_r_subkey, FS_w_subkey, VISIBLE,  {.i=0,}, },
	{"subkey0/id", _DS1991_ID_LENGTH, &A1991_password, ft_binary, fc_stable, FS_r_id, FS_w_id, VISIBLE,  {.i=0,}, },

	{"subkey1", PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir, fc_subdir, NO_READ_FUNCTION, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA, },
	{"subkey1/password", _DS1991_ID_LENGTH, &A1991_password, ft_binary, fc_stable, NO_READ_FUNCTION, FS_password, VISIBLE,  {.i=1,}, },
	{"subkey1/reset", PROPERTY_LENGTH_YESNO, &A1991_password, ft_yesno, fc_stable, NO_READ_FUNCTION, FS_reset, VISIBLE,  {.i=1,}, },
	{"subkey1/secure_data", _DS1991_DATA_LENGTH, &A1991_password, ft_binary, fc_stable, FS_r_subkey, FS_w_subkey, VISIBLE,  {.i=1,}, },
	{"subkey1/id", _DS1991_ID_LENGTH, &A1991_password, ft_binary, fc_stable, FS_r_id, FS_w_id, VISIBLE,  {.i=1,}, },

	{"subkey2", PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir, fc_subdir, NO_READ_FUNCTION, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA, },
	{"subkey2/password", _DS1991_ID_LENGTH, &A1991_password, ft_binary, fc_stable, NO_READ_FUNCTION, FS_password, VISIBLE,  {.i=2,}, },
	{"subkey2/reset", PROPERTY_LENGTH_YESNO, &A1991_password, ft_yesno, fc_stable, NO_READ_FUNCTION, FS_reset, VISIBLE,  {.i=2,}, },
	{"subkey2/secure_data", _DS1991_DATA_LENGTH, &A1991_password, ft_binary, fc_stable, FS_r_subkey, FS_w_subkey, VISIBLE,  {.i=2,}, },
	{"subkey2/id", _DS1991_ID_LENGTH, &A1991_password, ft_binary, fc_stable, FS_r_id, FS_w_id, VISIBLE,  {.i=2,}, },
};

DeviceEntry(82, DS1425, NO_GENERIC_READ, NO_GENERIC_WRITE);

#define _1W_WRITE_SCRATCHPAD 0x96
#define _1W_READ_SCRATCHPAD 0x69
#define _1W_COPY_SCRATCHPAD 0x3C
#define _1W_WRITE_PASSWORD 0x5A
#define _1W_WRITE_SUBKEY 0x99
#define _1W_READ_SUBKEY 0x66

/* ------- Functions ------------ */


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

// write a new password without clearing data
// done with a scratchpad copy
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

// write to secure data area
// needs the password encoded in the extension
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

// read from secure data area
// needs the password encoded in the extension
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

// read id
// needs a dummy password encoded in the extension
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

// write new id
// needs the password encoded in the extension
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
	BYTE subkey_addr = subkey_byte[ subkey ] + _DS1991_DATA_START ;
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
		TRXN_READ(data, size),
		TRXN_END,
	};
	
	return BUS_transaction(t, pn) ;
}

static GOOD_OR_BAD OW_write_subkey( int subkey, BYTE * password, size_t size, off_t offset, BYTE * data, const struct parsedname * pn )
{
	BYTE subkey_addr = subkey_byte[ subkey ] + offset ;
	BYTE write_sbk[] = { _1W_WRITE_SUBKEY, subkey_addr, BYTE_INVERSE(subkey_addr), } ;
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

static GOOD_OR_BAD OW_write_id( int subkey, BYTE * password, BYTE * id, const struct parsedname * pn )
{
	BYTE scratch_addr = (3<<6) + _DS1991_ID_START ;
	BYTE write_scratch[] = { _1W_WRITE_SCRATCHPAD, scratch_addr, BYTE_INVERSE(scratch_addr), } ;
	struct transaction_log tcopy[] = {
		TRXN_START,
		TRXN_WRITE3(write_scratch),
		TRXN_WRITE(id, _DS1991_ID_LENGTH),
		TRXN_END,
	};
	
	BYTE copy_addr = subkey_byte[ subkey ] ;
	BYTE copy_scratch[] = { _1W_COPY_SCRATCHPAD, copy_addr, BYTE_INVERSE(copy_addr), } ;
	struct transaction_log twrite[] = {
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

	
