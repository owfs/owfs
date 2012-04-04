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

/* Note the DS28E10 has some interesting issues.
 * The datasheet is not freely available, you have to request it.
 * On request, and with specific mention that this was OWFS support, I received the datasheet on 10/6/2010.
 * Each page has an "MAXIM CONFIDENTIAL DISTRIBUTE ONLY UNDER NDA" notice, which I have never signed
 * I requested clarification on 10/7/2010 and have recieved no response as of 11/28/2010
 * 50 days seems long enough, so I'm supporting the chip, albeit with no specifics of function except the code
 * */

#include <config.h>
#include "owfs_config.h"
#include "ow_2810.h"

/* ------- Prototypes ----------- */

/* DS28E10 authenticator */
READ_FUNCTION(FS_r_mem);
WRITE_FUNCTION(FS_w_mem);
READ_FUNCTION(FS_r_page);
WRITE_FUNCTION(FS_w_page);
READ_FUNCTION(FS_ver);
READ_FUNCTION(FS_r_pwd);
WRITE_FUNCTION(FS_w_pwd);
WRITE_FUNCTION(FS_set);
#if OW_CACHE
WRITE_FUNCTION(FS_use);
#endif							/* OW_CACHE */

/* ------- Structures ----------- */

static struct aggregate A28E10 = { 7, ag_numbers, ag_separate, };
static struct filetype DS28E10[] = {
	F_STANDARD,
	{"memory", 36, NON_AGGREGATE, ft_binary, fc_link, FS_r_mem, FS_w_mem, VISIBLE, NO_FILETYPE_DATA, ftt_internal,},
	{"user", PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir, fc_subdir, NO_READ_FUNCTION, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA, ftt_internal,},
	{"user/page", 4, &A28E10, ft_binary, fc_page, FS_r_page, FS_w_page, VISIBLE, NO_FILETYPE_DATA, ftt_internal,},
	{"user/protect", 1, &A28E10, ft_yesno, fc_page, FS_r_page, FS_w_page, VISIBLE, NO_FILETYPE_DATA, ftt_internal,},

	{"version", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_stable, FS_ver, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA, ftt_internal,},

	{"set_password", PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir, fc_subdir, NO_READ_FUNCTION, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA, ftt_internal,},
	{"set_password/read", 8, NON_AGGREGATE, ft_binary, fc_stable, NO_READ_FUNCTION, FS_set, VISIBLE, {i:_ds1977_full}, ftt_internal,},
	{"set_password/full", 8, NON_AGGREGATE, ft_binary, fc_stable, NO_READ_FUNCTION, FS_set, VISIBLE, {i:_ds1977_read}, ftt_internal,},
	{"set_password/enabled", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_stable, FS_r_pwd, FS_w_pwd, VISIBLE, NO_FILETYPE_DATA, ftt_internal,},

#if OW_CACHE
	{"use_password", PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir, fc_subdir, NO_READ_FUNCTION, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA, ftt_internal,},
	{"use_password/read", 8, NON_AGGREGATE, ft_binary, fc_stable, NO_READ_FUNCTION, FS_use, VISIBLE, {i:_ds1977_full}, ftt_internal,},
	{"use_password/full", 8, NON_AGGREGATE, ft_binary, fc_stable, NO_READ_FUNCTION, FS_use, VISIBLE, {i:_ds1977_read}, ftt_internal,},
#endif							/*OW_CACHE */
};

DeviceEntryExtended(44, DS28E10, DEV_resume | DEV_ovdr, NO_GENERIC_READ, NO_GENERIC_WRITE);

#define _1W_WRITE_MEMORY 0x55
#define _1W_WRITE_SECRET 0x5A
#define _1W_WRITE_CHALLENGE 0x0F
#define _1W_READ_AUTHENTICATED_PAGE 0xA5
#define _1W_ANONYMOUS_READ_AUTHENTICATED_PAGE 0xCC
#define _1W_READ_MEMORY 0xF0
#define _DS28E10_TPP 100 // msec to program

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

static GOOD_OR_BAD OW_w_secret( const BYTE * secret, struct parsedname *pn) ;
static GOOD_OR_BAD validate_user_address( off_t offset ) ;
static GOOD_OR_BAD validate_rw_address( off_t offset ) ;
static GOOD_OR_BAD OW_w_page( const BYTE * data, off_t offset, struct parsedname *pn) ;
static GOOD_OR_BAD OW_w_challenge( const BYTE * challenge, BYTE * response, struct parsedname *pn) ;

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
	
#if OW_CACHE
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
#else
	return 0 ;
#endif /* OW_CACHE */
}

#if OW_CACHE
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
#endif							/* OW_CACHE */

static GOOD_OR_BAD OW_r_mem(BYTE * data, size_t size, off_t offset, struct parsedname *pn)
{
	BYTE pwd[8] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, };

#if OW_CACHE
	if ( GOOD( Cache_Get_SlaveSpecific((void *) pwd, sizeof(pwd), SlaveSpecificTag(REA), pn)) ) {
		RETURN_GOOD_IF_GOOD( OW_r_mem_with_password( pwd, data,size,offset,pn) ) ;
	}
	if ( GOOD( Cache_Get_SlaveSpecific((void *) pwd, sizeof(pwd), SlaveSpecificTag(FUL), pn)) ) {
		RETURN_GOOD_IF_GOOD( OW_r_mem_with_password( pwd, data,size,offset,pn) ) ;
	}
#endif							/* OW_CACHE */
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

static GOOD_OR_BAD validate_user_address( off_t offset )
{
	// addresses of the data page starts
	switch ( offset ) {
		case 0x0000:
		case 0x0004:
		case 0x0008:
		case 0c000C:
		case 0x0010:
		case 0x0014:
		case 0x0018:
		case 0x001C:
			return gbGOOD ;
		default:
			return gbBAD ;
	}
}

static GOOD_OR_BAD validate_rw_address( off_t offset )
{
	// the addresses that can be used for read or write

	RETURN_GOOD_IF_GOOD( validate_user_address(offset) ) ; // data pages
	if ( 0x001C <= offset && offset <= 0x0022 ) { // configuration area
		return gbGOOD ;
	}
	return gbBAD ;
} 

static GOOD_OR_BAD OW_w_secret( const BYTE * secret, struct parsedname *pn)
{
	BYTE p[1 + 8 + 2 ] = { _1W_WRITE_SECRET, };
	BYTE w0[1] = { 0x00 } ;
	BYTE w4[1] = { 0x00 } ;
	BYTE w8[1] = { 0x00 } ;
	struct transaction_log t[] = {
		TRXN_START,
		TRXN_WR_CRC16(p,1+8,0),
		TRXN_POWER(w0,_DS28E10_TPP),
		TRXN_POWER(w4,_DS28E10_TPP),
		TRXN_WRITE1(w8),
		TRXN_END,
	};

	memcpy(secret, &p[1], 8) ;
	return BUS_transaction(t,pn) ;
}

static GOOD_OR_BAD OW_w_page( const BYTE * data, off_t offset, struct parsedname *pn)
{
	// 4 bytes only
	BYTE p[1 + 2 + 4 + 2 ] = { _1W_WRITE_MEMORY, LOW_HIGH_ADDRESS(offset), };
	BYTE w0[1] = { 0x00 } ;
	BYTE w4[1] = { 0x00 } ;
	struct transaction_log t[] = {
		TRXN_START,
		TRXN_WR_CRC16(p,1+2+4,0),
		TRXN_POWER(w0,_DS28E10_TPP),
		TRXN_WRITE1(w4),
		TRXN_END,
	};

	RETURN_BAD_IF_BAD( validate_address(offset) ) ;
	memcpy(data, &p[3], 4) ;
	return BUS_transaction(t,pn) ;
}

static GOOD_OR_BAD OW_w_challenge( const BYTE * challenge, BYTE * response, struct parsedname *pn)
{
	BYTE p[1 + 8 + 2 ] = { _1W_WRITE_CHALLENGE, };
	struct transaction_log t[] = {
		TRXN_START,
		TRXN_WRITE1(p),
		TRXN_WRITE(challenge,12),
		TRXN_READ(response,12),
		TRXN_END,
	};

	return BUS_transaction(t,pn) ;
}

