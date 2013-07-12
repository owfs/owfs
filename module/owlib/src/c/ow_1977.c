/*
$Id$
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
#include "ow_1977.h"

/* ------- Prototypes ----------- */

/* DS1977 counter */
READ_FUNCTION(FS_r_mem);
WRITE_FUNCTION(FS_w_mem);
READ_FUNCTION(FS_r_page);
WRITE_FUNCTION(FS_w_page);
READ_FUNCTION(FS_ver);
READ_FUNCTION(FS_r_pwd);
WRITE_FUNCTION(FS_w_pwd);
WRITE_FUNCTION(FS_set);
WRITE_FUNCTION(FS_use);

/* ------- Structures ----------- */

enum _ds1977_passwords { _ds1977_full, _ds1977_read, _ds1977_control } ;
off_t _ds1977_pwd_loc[] = { 0x7FC0, 0x7FC8, 0x7FD0, } ;

static struct aggregate A1977 = { 511, ag_numbers, ag_separate, };
static struct filetype DS1977[] = {
	F_STANDARD,
	{"memory", 32704, NON_AGGREGATE, ft_binary, fc_link, FS_r_mem, FS_w_mem, VISIBLE, NO_FILETYPE_DATA, },
	{"pages", PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir, fc_subdir, NO_READ_FUNCTION, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA, },
	{"pages/page", 64, &A1977, ft_binary, fc_page, FS_r_page, FS_w_page, VISIBLE, NO_FILETYPE_DATA, },

	{"version", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_stable, FS_ver, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA, },

	{"set_password", PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir, fc_subdir, NO_READ_FUNCTION, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA, },
	{"set_password/read", 8, NON_AGGREGATE, ft_binary, fc_stable, NO_READ_FUNCTION, FS_set, VISIBLE, {i:_ds1977_full}, },
	{"set_password/full", 8, NON_AGGREGATE, ft_binary, fc_stable, NO_READ_FUNCTION, FS_set, VISIBLE, {i:_ds1977_read}, },
	{"set_password/enabled", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_stable, FS_r_pwd, FS_w_pwd, VISIBLE, NO_FILETYPE_DATA, },

	{"use_password", PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir, fc_subdir, NO_READ_FUNCTION, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA, },
	{"use_password/read", 8, NON_AGGREGATE, ft_binary, fc_stable, NO_READ_FUNCTION, FS_use, VISIBLE, {i:_ds1977_full}, },
	{"use_password/full", 8, NON_AGGREGATE, ft_binary, fc_stable, NO_READ_FUNCTION, FS_use, VISIBLE, {i:_ds1977_read}, },
};

DeviceEntryExtended(37, DS1977, DEV_resume | DEV_ovdr, NO_GENERIC_READ, NO_GENERIC_WRITE);

#define _1W_WRITE_SCRATCHPAD 0x0F
#define _1W_READ_SCRATCHPAD 0xAA
#define _1W_COPY_SCRATCHPAD_WITH_PASSWORD 0x99
#define _1W_READ_MEMORY_WITH_PASSWORD 0xC3
#define _1W_VERIFY_PASSWORD 0x69
#define _1W_READ_VERSION 0xCC

#define _DS1977_PASSWORD_OK  0xAA

/* Persistent storage */
Make_SlaveSpecificTag(REA, fc_persistent);
Make_SlaveSpecificTag(FUL, fc_persistent);

/* ------- Functions ------------ */

/* DS2423 */
static GOOD_OR_BAD OW_w_mem( BYTE * data, size_t size, off_t offset, struct parsedname *pn) ;
static GOOD_OR_BAD OW_r_mem(BYTE * data, size_t size, off_t offset, struct parsedname *pn);
static GOOD_OR_BAD OW_version(UINT * u, struct parsedname *pn);
static GOOD_OR_BAD OW_verify(BYTE * pwd, off_t offset, struct parsedname *pn);
static GOOD_OR_BAD OW_r_mem_with_password( BYTE * pwd, BYTE * data, size_t size, off_t offset, struct parsedname *pn) ;

/* 1977 password */
static ZERO_OR_ERROR FS_r_page(struct one_wire_query *owq)
{
	size_t pagesize = 64;
	return COMMON_offset_process( FS_r_mem, owq, OWQ_pn(owq).extension*pagesize) ;
}

static ZERO_OR_ERROR FS_w_page(struct one_wire_query *owq)
{
	size_t pagesize = 64;
	return COMMON_offset_process( FS_w_mem, owq, OWQ_pn(owq).extension*pagesize) ;
}

static ZERO_OR_ERROR FS_r_mem(struct one_wire_query *owq)
{
	size_t pagesize = 64;
	return GB_to_Z_OR_E(COMMON_readwrite_paged(owq, 0, pagesize, OW_r_mem)) ;
}

static ZERO_OR_ERROR FS_w_mem(struct one_wire_query *owq)
{
	size_t pagesize = 64;
	return GB_to_Z_OR_E(COMMON_readwrite_paged(owq, 0, pagesize, OW_w_mem)) ;
}

static ZERO_OR_ERROR FS_ver(struct one_wire_query *owq)
{
	return GB_to_Z_OR_E( OW_version(&OWQ_U(owq), PN(owq)) );
}

static ZERO_OR_ERROR FS_r_pwd(struct one_wire_query *owq)
{
	BYTE p;
	RETURN_ERROR_IF_BAD( OW_r_mem(&p, 1, _ds1977_pwd_loc[_ds1977_control], PN(owq)) ) ;
	OWQ_Y(owq) = (p == _DS1977_PASSWORD_OK);
	return 0;
}

static ZERO_OR_ERROR FS_w_pwd(struct one_wire_query *owq)
{
	BYTE p = OWQ_Y(owq) ? 0x00 : _DS1977_PASSWORD_OK;
	return GB_to_Z_OR_E( OW_w_mem(&p, 1, _ds1977_pwd_loc[_ds1977_control], PN(owq)) );
}

static ZERO_OR_ERROR FS_set(struct one_wire_query *owq)
{
	struct parsedname * pn = PN(owq) ;
	if (OWQ_size(owq) < 8) {
		return -ERANGE;
	}

	/* Write */
	RETURN_ERROR_IF_BAD( OW_w_mem((BYTE *) OWQ_buffer(owq), 8, _ds1977_pwd_loc[pn->selected_filetype->data.i],pn) ) ;

	/* Verify */
	RETURN_ERROR_IF_BAD(OW_verify((BYTE *) OWQ_buffer(owq), _ds1977_pwd_loc[pn->selected_filetype->data.i], pn) ) ;
	
	switch ( pn->selected_filetype->data.i) {
		case _ds1977_full:
			Cache_Add_SlaveSpecific((BYTE *) OWQ_buffer(owq), 8, SlaveSpecificTag(FUL) , pn) ;
			break ;
		case _ds1977_read:
		default:
			Cache_Add_SlaveSpecific((BYTE *) OWQ_buffer(owq), 8, SlaveSpecificTag(REA) , pn) ;
			break ;
	}
	return FS_use(owq);
}

static ZERO_OR_ERROR FS_use(struct one_wire_query *owq)
{
	struct parsedname * pn = PN(owq) ;
	if (OWQ_size(owq) < 8) {
		return -ERANGE;
	}

	switch ( pn->selected_filetype->data.i) {
		case _ds1977_full:
			return Cache_Add_SlaveSpecific((BYTE *) OWQ_buffer(owq), 8, SlaveSpecificTag(FUL) , pn)==0 ? 0 : -EINVAL ;
		case _ds1977_read:
			return Cache_Add_SlaveSpecific((BYTE *) OWQ_buffer(owq), 8, SlaveSpecificTag(REA) , pn)==0 ? 0 : -EINVAL ;
	}
	return -EINVAL;
}

static GOOD_OR_BAD OW_r_mem(BYTE * data, size_t size, off_t offset, struct parsedname *pn)
{
	BYTE pwd[8] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, };

	if ( GOOD( Cache_Get_SlaveSpecific((void *) pwd, sizeof(pwd), SlaveSpecificTag(REA), pn)) ) {
		RETURN_GOOD_IF_GOOD( OW_r_mem_with_password( pwd, data,size,offset,pn) ) ;
	}
	if ( GOOD( Cache_Get_SlaveSpecific((void *) pwd, sizeof(pwd), SlaveSpecificTag(FUL), pn)) ) {
		RETURN_GOOD_IF_GOOD( OW_r_mem_with_password( pwd, data,size,offset,pn) ) ;
	}
	return OW_r_mem_with_password(pwd, data, size, offset, pn);
}

static GOOD_OR_BAD OW_version(UINT * u, struct parsedname *pn)
{
	BYTE p[] = { _1W_READ_VERSION, 0x00, 0x00 };
	struct transaction_log t[] = {
		TRXN_START,
		TRXN_MODIFY(p,p,3),
		TRXN_COMPARE(&p[1],&p[2],1),
		TRXN_END,
	} ;
	
	RETURN_BAD_IF_BAD( BUS_transaction(t,pn) ) ;
	u[0] = p[1];
	return gbGOOD;
}

static GOOD_OR_BAD OW_w_mem( BYTE * data, size_t size, off_t offset, struct parsedname *pn)
{
	BYTE p[1 + 2 + 1+ 64 + 2] = { _1W_WRITE_SCRATCHPAD, LOW_HIGH_ADDRESS(offset), };
	BYTE passwd[8] ; // altered
	size_t rest = 64 - (offset & 0x3F);
	BYTE post[1] ;
	struct transaction_log t_nocrc[] = {
		TRXN_START,
		TRXN_WRITE(p,3+size),
		TRXN_END,
	};
	struct transaction_log t_crc[] = {
		TRXN_START,
		TRXN_WR_CRC16(p,3+size,0),
		TRXN_END,
	};
	struct transaction_log t_read[] = {
		TRXN_START,
		TRXN_WR_CRC16(p,1,3+rest),
		TRXN_COMPARE(&p[4],data,size),
		TRXN_END,
	};
	struct transaction_log t_copy[] = {
		TRXN_START,
		TRXN_WRITE(p,4),
		TRXN_MODIFY(passwd,passwd, 8-1),
		TRXN_POWER(&passwd[7],10), // 10ms
		TRXN_READ1(post),
		TRXN_END,
	};

	// set up transfer
	if ( size>rest ) {
		return gbBAD ;
	}
	memcpy(&p[3],data,size) ;
	
	// Write to scratchpad (possibly with CRC16)
	if ( size==rest ) {
		RETURN_BAD_IF_BAD( BUS_transaction( t_crc, pn ) ) ;
	} else {
		RETURN_BAD_IF_BAD( BUS_transaction( t_nocrc, pn ) ) ;
	}
	
	// Read back from scratch pad to prime next step and confirm data
	/* Note that we tacitly shift the data one byte down for the E/S byte */
	p[0] = _1W_READ_SCRATCHPAD ;
	RETURN_BAD_IF_BAD( BUS_transaction( t_read, pn ) ) ;

	// Copy scratchpad to memory
	if ( BAD( Cache_Get_SlaveSpecific((void *) passwd, 8, SlaveSpecificTag(FUL), pn)) ) {	/* Full passwd */
		memset( passwd, 0xFF, 8 ) ;
	}
	p[0] = _1W_COPY_SCRATCHPAD_WITH_PASSWORD ;
	RETURN_BAD_IF_BAD( BUS_transaction( t_copy, pn ) ) ;
	return (post[0]==0xFF) ? gbBAD : gbGOOD ;
}

static GOOD_OR_BAD OW_r_mem_with_password( BYTE * pwd, BYTE * data, size_t size, off_t offset, struct parsedname *pn)
{
	BYTE p[1 + 2 + 64 + 2] = { _1W_READ_MEMORY_WITH_PASSWORD, LOW_HIGH_ADDRESS(offset), };
	BYTE passwd[8] ; // altered
	size_t rest = 64 - (offset & 0x3F);
	struct transaction_log t[] = {
		TRXN_START,
		TRXN_WRITE(p,3),
		TRXN_MODIFY(passwd,passwd, 8-1),
		TRXN_POWER(&passwd[7],5), // 5ms
		TRXN_READ(&p[3],rest+2),
		{ p, NULL, 3+rest+2, trxn_crc16, },
		TRXN_END,
	};

	// set up transfer
	if ( size>rest ) {
		return gbBAD ;
	}
	memcpy(passwd,pwd,8) ;
	RETURN_BAD_IF_BAD( BUS_transaction(t,pn) ) ;

	memcpy(data,&p[3],size) ;
	return gbGOOD ;
}

static GOOD_OR_BAD OW_verify(BYTE * pwd, off_t offset, struct parsedname *pn)
{
	BYTE p[1 + 2] = { _1W_READ_MEMORY_WITH_PASSWORD, LOW_HIGH_ADDRESS(offset), };
	BYTE passwd[8] ; // altered
	BYTE post[1] ;
	struct transaction_log t[] = {
		TRXN_START,
		TRXN_WRITE(p,3),
		TRXN_MODIFY(passwd,passwd, 8-1),
		TRXN_POWER(&passwd[7],5), // 5ms
		TRXN_READ1(post),
		TRXN_END,
	};

	memcpy(passwd,pwd,8) ;
	RETURN_BAD_IF_BAD( BUS_transaction(t,pn) ) ;

	return post[0]==0xFF ? gbBAD : gbGOOD ;
}
