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
#include "ow_eds.h"

/* ------- Prototypes ----------- */

READ_FUNCTION(FS_r_page);
WRITE_FUNCTION(FS_w_page);
READ_FUNCTION(FS_r_mem);
WRITE_FUNCTION(FS_w_mem);
READ_FUNCTION(FS_r_tag);
READ_FUNCTION(FS_temperature);

READ_FUNCTION(FS_r_8) ;
WRITE_FUNCTION(FS_w_8) ;
READ_FUNCTION(FS_r_16) ;
WRITE_FUNCTION(FS_w_16) ;
READ_FUNCTION(FS_r_24) ;
WRITE_FUNCTION(FS_w_24) ;
READ_FUNCTION(FS_r_32) ;
WRITE_FUNCTION(FS_w_32) ;
READ_FUNCTION(FS_r_rtd) ;
WRITE_FUNCTION(FS_w_rtd) ;

static enum e_visibility EDS_visible(const struct parsedname * pn) ;


#define _EDS_WRITE_SCRATCHPAD 0x0F
#define _EDS_READ_SCRATCHPAD 0xAA
#define _EDS_COPY_SCRATCHPAD 0x55
#define _EDS_READ_MEMMORY_NO_CRC 0xF0
#define _EDS_READ_MEMORY_WITH_CRC 0xA5
#define _EDS_CLEAR_ALARMS 0x33

#define _EDS_PAGES 3
#define _EDS_PAGESIZE 32

#define _EDS0064   0x0001
#define _EDS0065   0x0002
#define _EDS0066   0x0004
#define _EDS0067   0x0008
#define _EDS0068   0x0010
#define _EDS0069   0x0020
#define _EDS0070   0x0040
#define _EDS0071   0x0080
#define _EDS0072   0x0100

#define _EDS_CHIPS_TEMPERATURE (_EDS0064|_EDS0065|_EDS0066|_EDS0067|_EDS0068|_EDS0069)
struct eds_types {
	char * name ;
	unsigned bit ;
} EDS_types[] = {
	{ "EDS0064", _EDS0064 },
	{ "EDS0065", _EDS0065 },
	{ "EDS0066", _EDS0066 },
	{ "EDS0067", _EDS0067 },
	{ "EDS0068", _EDS0068 },
	{ "EDS0069", _EDS0069,},
	{ "EDS0070", _EDS0070,},
	{ "EDS0071", _EDS0071,},
	{ "EDS0072", _EDS0072,},
} ;
#define N_eds_types (sizeof(EDS_types) / sizeof( struct eds_types))

/* EDS0071 locations */
#define _EDS0071_ID					0x1E // uint16
#define _EDS0071_Temp				0x22 // 24bit float
#define _EDS0071_RTD				0x25 // 24bit float
#define _EDS0071_Conversion			0x28 // uint24
#define _EDS0071_Calibration		0x30 // uint32
//#define _EDS0071_Relay_state		0x35 // byte
#define _EDS0071_Alarm_state		0x36 // byte
#define _EDS0071_Conversion_counter	0x38 // uint32
#define _EDS0071_Seconds_counter	0x3C // uint32
#define _EDS0071_Conditional_search	0x40 // byte
#define _EDS0071_free_byte			0x41 // byte
#define _EDS0071_Temp_hi			0x42 // 24bit float
#define _EDS0071_Temp_lo			0x45 // 24bit float
#define _EDS0071_RTD_hi				0x48 // 24bit float
#define _EDS0071_RTD_lo				0x4B // 24bit float
#define _EDS0071_Calibration_key	0x5C // byte
#define _EDS0071_RTD_delay			0x5D // byte
#define _EDS0071_Relay_function		0x5E // byte
#define _EDS0071_Relay_state		0x5F // byte

#define VISIBLE_EDS0071   VISIBLE

#define _EDS_TAG_LENGTH 8

/* ------- Structures ----------- */

struct aggregate AEDS = { _EDS_PAGES, ag_numbers, ag_separate, };
struct filetype EDS[] = {
	F_STANDARD,
	{"memory", _EDS_PAGES * _EDS_PAGESIZE, NON_AGGREGATE, ft_binary, fc_link, FS_r_mem, FS_w_mem, VISIBLE, NO_FILETYPE_DATA,},
	{"pages", PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir, fc_subdir, NO_READ_FUNCTION, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA,},
	{"pages/page", _EDS_PAGESIZE, &AEDS, ft_binary, fc_page, FS_r_page, FS_w_page, VISIBLE, NO_FILETYPE_DATA,},

	{"tag", _EDS_TAG_LENGTH, NON_AGGREGATE, ft_ascii, fc_stable, FS_r_tag, NO_WRITE_FUNCTION, INVISIBLE, NO_FILETYPE_DATA,},
	{"local_id", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_stable, FS_r_16, NO_WRITE_FUNCTION, VISIBLE, {u: _EDS0071_ID,}, },
	{"temperature", PROPERTY_LENGTH_TEMP, NON_AGGREGATE, ft_temperature, fc_volatile, FS_temperature, NO_WRITE_FUNCTION, EDS_visible, {i:_EDS_CHIPS_TEMPERATURE},},

	{"EDS0071", PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir, fc_subdir, NO_READ_FUNCTION, NO_WRITE_FUNCTION, VISIBLE_EDS0071, NO_FILETYPE_DATA,},
	{"EDS0071/temperature", PROPERTY_LENGTH_TEMP, NON_AGGREGATE, ft_temperature, fc_volatile, FS_r_rtd, NO_WRITE_FUNCTION, VISIBLE_EDS0071, {u: _EDS0071_Temp,}, },
	{"EDS0071/resistance", PROPERTY_LENGTH_FLOAT, NON_AGGREGATE, ft_float, fc_volatile, FS_r_rtd, NO_WRITE_FUNCTION, VISIBLE_EDS0071, {u: _EDS0071_RTD,}, },
	{"EDS0071/raw", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_volatile, FS_r_24, NO_WRITE_FUNCTION, INVISIBLE, {u: _EDS0071_Conversion,}, },
	{"EDS0071/calibration", PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir, fc_subdir, NO_READ_FUNCTION, NO_WRITE_FUNCTION, VISIBLE_EDS0071, NO_FILETYPE_DATA,},
	{"EDS0071/calibration/constant", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_stable, FS_r_32, NO_WRITE_FUNCTION, VISIBLE_EDS0071, {u: _EDS0071_Calibration,}, },
	{"EDS0071/calibration/key", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_stable, FS_r_8, FS_w_8, VISIBLE_EDS0071, {u: _EDS0071_Calibration_key,}, },
	{"EDS0071/counter", PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir, fc_subdir, NO_READ_FUNCTION, NO_WRITE_FUNCTION, VISIBLE_EDS0071, NO_FILETYPE_DATA,},
	{"EDS0071/counter/seconds", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_second, FS_r_32, NO_WRITE_FUNCTION, VISIBLE_EDS0071, {u: _EDS0071_Seconds_counter,}, },
	{"EDS0071/counter/samples", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_second, FS_r_32, NO_WRITE_FUNCTION, VISIBLE_EDS0071, {u: _EDS0071_Conversion_counter,}, },
	{"EDS0071/set_alarm", PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir, fc_subdir, NO_READ_FUNCTION, NO_WRITE_FUNCTION, VISIBLE_EDS0071, NO_FILETYPE_DATA,},
	{"EDS0071/set_alarm/temphi", PROPERTY_LENGTH_TEMP, NON_AGGREGATE, ft_temperature, fc_stable, FS_r_rtd, FS_w_rtd, VISIBLE_EDS0071, {u: _EDS0071_Temp_hi,}, },
	{"EDS0071/set_alarm/templow", PROPERTY_LENGTH_TEMP, NON_AGGREGATE, ft_temperature, fc_stable, FS_r_rtd, FS_w_rtd, VISIBLE_EDS0071, {u: _EDS0071_Temp_lo,}, },
	{"EDS0071/set_alarm/Rhi", PROPERTY_LENGTH_FLOAT, NON_AGGREGATE, ft_float, fc_stable, FS_r_rtd, FS_w_rtd, VISIBLE_EDS0071, {u: _EDS0071_RTD_hi,}, },
	{"EDS0071/set_alarm/Rlow", PROPERTY_LENGTH_FLOAT, NON_AGGREGATE, ft_float, fc_stable, FS_r_rtd, FS_w_rtd, VISIBLE_EDS0071, {u: _EDS0071_RTD_lo,}, },
	{"EDS0071/set_alarm/alarm_function", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_volatile, FS_r_8, FS_w_8, INVISIBLE, {u: _EDS0071_Conditional_search,}, },
	{"EDS0071/user_byte", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_volatile, FS_r_8, FS_w_8, VISIBLE_EDS0071, {u: _EDS0071_free_byte,}, },
	{"EDS0071/delay", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_volatile, FS_r_8, FS_w_8, VISIBLE_EDS0071, {u: _EDS0071_RTD_delay,}, },
	{"EDS0071/relay_function", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_volatile, FS_r_8, FS_w_8, INVISIBLE, {u: _EDS0071_Relay_function,}, },
	{"EDS0071/relay_state", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_volatile, FS_r_8, NO_WRITE_FUNCTION, INVISIBLE, {u: _EDS0071_Relay_state,}, },
	{"EDS0071/alarm", PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir, fc_subdir, NO_READ_FUNCTION, NO_WRITE_FUNCTION, VISIBLE_EDS0071, NO_FILETYPE_DATA,},
	{"EDS0071/alarm/state", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_volatile, FS_r_8, NO_WRITE_FUNCTION, INVISIBLE, {u: _EDS0071_Alarm_state,}, },
};

DeviceEntryExtended(7E, EDS, DEV_temp | DEV_alarm, NO_GENERIC_READ, NO_GENERIC_WRITE);

/* ------- Functions ------------ */
static GOOD_OR_BAD OW_w_mem(BYTE * data, size_t size, off_t offset, struct parsedname *pn) ;
static GOOD_OR_BAD OW_r_mem_small(BYTE * data, size_t size, off_t offset, struct parsedname * pn);

/* Persistent storage */
Make_SlaveSpecificTag(TAG, fc_persistent);	// cumulative

/* EDS memory */
// Only used to set visibility for specific 
static ZERO_OR_ERROR FS_r_tag(struct one_wire_query *owq)
{
	size_t size = _EDS_PAGESIZE ;
	char data[_EDS_PAGESIZE] ;
	struct parsedname * pn = PN(owq) ;
	UINT tag = 0x0000 ;

	if ( GOOD( Cache_Get_SlaveSpecific(&tag, sizeof(UINT), SlaveSpecificTag(TAG), pn)) ) {
		// Tag exists, find the name
		int tag_type = N_eds_types ;
		while ( --tag_type >= 0 ) {
			if (  tag == EDS_types[tag_type].bit ) {
				return OWQ_format_output_offset_and_size( EDS_types[tag_type].name, _EDS_TAG_LENGTH, owq ) ;
			}
		}
	} else if ( FS_r_sibling_binary(data,&size,"pages/page.0",owq) == 0 	) { // read device memory
		int tag_type = N_eds_types ;
		while ( --tag_type >= 0 ) {
			if ( strncasecmp( data, EDS_types[tag_type].name, _EDS_TAG_LENGTH ) == 0 ) {
				tag = EDS_types[tag_type].bit ;
				break ;
			}
		}
		if ( OWQ_format_output_offset_and_size( data, _EDS_TAG_LENGTH, owq ) ) {
			return -EINVAL ;
		}
		Cache_Add_SlaveSpecific(&tag, sizeof(UINT), SlaveSpecificTag(TAG), pn) ;
		return 0 ;
	}
	return -EINVAL ;
}

static ZERO_OR_ERROR FS_temperature(struct one_wire_query *owq)
{
	size_t size = _EDS_PAGESIZE ;
	char data[_EDS_PAGESIZE]  = { 0x00, 0x00, 0x00, } ;
	ZERO_OR_ERROR z_or_e = FS_r_sibling_binary(data,&size,"pages/page.1",owq) ; // read device memory

	OWQ_F(owq) = ( (_FLOAT)UT_uint16( (BYTE *) &data[2] ) ) / 16. ;
	return z_or_e ;
}

static ZERO_OR_ERROR FS_r_mem(struct one_wire_query *owq)
{
	size_t pagesize = 32;
	return GB_to_Z_OR_E(COMMON_OWQ_readwrite_paged(owq, 0, pagesize, COMMON_read_memory_F0)) ;
}

static enum e_visibility EDS_visible(const struct parsedname * pn) {
	UINT tag ;
	if ( BAD( Cache_Get_SlaveSpecific(&tag, sizeof(UINT), SlaveSpecificTag(TAG), pn)) ) {	// tag doesn't (yet) exist
		struct one_wire_query * owq = OWQ_create_from_path(pn->path) ; // for read
		size_t size = _EDS_TAG_LENGTH ;
		char data[size] ;
		if ( owq != NO_ONE_WIRE_QUERY) {
			FS_r_sibling_binary(data,&size,"tag",owq) ;
			OWQ_destroy(owq) ;
		}
	}
	if ( BAD( Cache_Get_SlaveSpecific(&tag, sizeof(UINT), SlaveSpecificTag(TAG), pn)) ) {	// tag doesn't (yet) exist
		LEVEL_DEBUG("Cannot check visibility tag type for this entry");
		return visible_now ; // assume visible
	}
	return ( tag & pn->selected_filetype->data.u ) ? visible_now : visible_not_now ;
}

static ZERO_OR_ERROR FS_r_page(struct one_wire_query *owq)
{
	return COMMON_offset_process( FS_r_mem, owq, OWQ_pn(owq).extension*_EDS_PAGESIZE) ;
}

static ZERO_OR_ERROR FS_w_mem(struct one_wire_query *owq)
{
	size_t size = OWQ_size(owq) ;
	off_t start = OWQ_offset(owq) ;
	BYTE * position = (BYTE *)OWQ_buffer(owq) ;
	size_t write_size = _EDS_PAGESIZE - (start % _EDS_PAGESIZE) ;
	
	while ( size > 0 ) {
		if ( write_size > size ) {
			write_size = size ;
		}
		RETURN_ERROR_IF_BAD( OW_w_mem(position, write_size, start, PN(owq)) ) ;
		position += write_size ;
		start += write_size ;
		size -= write_size ;
		write_size = _EDS_PAGESIZE ;
	}
	return 0;
}

static ZERO_OR_ERROR FS_w_page(struct one_wire_query *owq)
{
	return COMMON_offset_process( FS_w_mem, owq, OWQ_pn(owq).extension*_EDS_PAGESIZE) ;
}

/* read an 8 bit value from a register stored in filetype.data plus extension */
static ZERO_OR_ERROR FS_r_8(struct one_wire_query *owq)
{
	struct parsedname * pn = PN(owq) ;
	int bytes = 8/8 ;
	BYTE data[bytes] ;
	
	RETURN_ERROR_IF_BAD( OW_r_mem_small(data, bytes, pn->selected_filetype->data.u + bytes * pn->extension, pn ) );
	OWQ_U(owq) = data[0] ;
	return 0 ;
}

/* write an 8 bit value from a register stored in filetype.data plus extension */
static ZERO_OR_ERROR FS_w_8(struct one_wire_query *owq)
{
	struct parsedname * pn = PN(owq) ;
	int bytes = 8/8 ;
	BYTE data[bytes] ;
	
	data[0] = BYTE_MASK( OWQ_U(owq) ) ;
	return GB_to_Z_OR_E(OW_w_mem(data, bytes, pn->selected_filetype->data.u + bytes * pn->extension, pn ) ) ;
}

/* read a 16 bit value from a register stored in filetype.data */
static ZERO_OR_ERROR FS_r_16(struct one_wire_query *owq)
{
	struct parsedname * pn = PN(owq) ;
	int bytes = 16/8 ;
	BYTE data[bytes] ;
	
	RETURN_ERROR_IF_BAD( OW_r_mem_small(data, bytes, pn->selected_filetype->data.u + bytes * pn->extension, pn ) );
	OWQ_U(owq) = UT_int16(data) ;
	return 0 ;
}

/* write a 24 bit value from a register stored in filetype.data */
static ZERO_OR_ERROR FS_w_24(struct one_wire_query *owq)
{
	struct parsedname * pn = PN(owq) ;
	int bytes = 24/8 ;
	BYTE data[bytes] ;

	UT_uint16_to_bytes( OWQ_U(owq), data ) ;
	return GB_to_Z_OR_E( OW_w_mem(data, bytes, pn->selected_filetype->data.u + bytes * pn->extension, pn ) ) ;
}

/* read a 24 bit value from a register stored in filetype.data */
static ZERO_OR_ERROR FS_r_24(struct one_wire_query *owq)
{
	struct parsedname * pn = PN(owq) ;
	int bytes = 24/8 ;
	BYTE data[bytes] ;
	
	RETURN_ERROR_IF_BAD( OW_r_mem_small(data, bytes, pn->selected_filetype->data.u + bytes * pn->extension, pn ) );
	OWQ_U(owq) = UT_int24(data) ;
	return 0 ;
}

/* write a 16 bit value from a register stored in filetype.data */
static ZERO_OR_ERROR FS_w_16(struct one_wire_query *owq)
{
	struct parsedname * pn = PN(owq) ;
	int bytes = 16/8 ;
	BYTE data[bytes] ;

	UT_uint24_to_bytes( OWQ_U(owq), data ) ;
	return GB_to_Z_OR_E( OW_w_mem(data, bytes, pn->selected_filetype->data.u + bytes * pn->extension, pn ) ) ;
}

/* read a 32 bit value from a register stored in filetype.data */
static ZERO_OR_ERROR FS_r_32(struct one_wire_query *owq)
{
	struct parsedname * pn = PN(owq) ;
	int bytes = 32/8 ;
	BYTE data[bytes] ;
	
	RETURN_ERROR_IF_BAD( OW_r_mem_small(data, bytes, pn->selected_filetype->data.u + bytes * pn->extension, pn ) );
	OWQ_U(owq) = UT_int32(data) ;
	return 0 ;
}

/* write a 32 bit value from a register stored in filetype.data */
static ZERO_OR_ERROR FS_w_32(struct one_wire_query *owq)
{
	struct parsedname * pn = PN(owq) ;
	int bytes = 32/8 ;
	BYTE data[bytes] ;
	
	UT_uint32_to_bytes( OWQ_U(owq), data ) ;
	return GB_to_Z_OR_E( OW_w_mem(data, bytes, pn->selected_filetype->data.u + bytes * pn->extension, pn ) ) ;
}

/* read a 24 bit value from a register stored in filetype.data */
/* write as a signed float with resolution 1/2048th */
static ZERO_OR_ERROR FS_r_rtd(struct one_wire_query *owq)
{
	_FLOAT f ;

	RETURN_ERROR_IF_BAD( FS_r_24(owq) );

	if ( OWQ_U(owq) >= 0x800000 ) {
		f = 0x1000000 - OWQ_U(owq) ;
		f = -f ;
	} else {
		f = OWQ_U(owq) ;
	}
	OWQ_F(owq) = f / 2048. ;
	return 0 ;
}

/* write a 24 bit value from a register stored in filetype.data */
/* write as a signed float with resolution 1/2048th */
static ZERO_OR_ERROR FS_w_rtd(struct one_wire_query *owq)
{
	uint32_t big = 2048. * OWQ_F(owq) ;
	OWQ_U(owq) = big + 0x1000000 ; // overflow the 24bit range to handle negatives in 2's complement
	return FS_w_24(owq) ;
}

static GOOD_OR_BAD OW_w_mem(BYTE * data, size_t size, off_t offset, struct parsedname *pn)
{
	BYTE p[3 + 1 + _EDS_PAGESIZE + 2] = { _EDS_WRITE_SCRATCHPAD, LOW_HIGH_ADDRESS(offset), };
	int rest = _EDS_PAGESIZE - (offset & 0x1F);
	struct transaction_log tcopy[] = {
		TRXN_START,
		TRXN_WRITE(p, 3 + size),
		TRXN_END,
	};
	struct transaction_log tcopy_crc[] = {
		TRXN_START,
		TRXN_WR_CRC16(p, 3 + size, 0),
		TRXN_END,
	};
	struct transaction_log tread[] = {
		TRXN_START,
		TRXN_WR_CRC16(p, 3, 1 + rest),
		TRXN_COMPARE(&p[4], data, size),
		TRXN_END,
	};
	struct transaction_log twrite[] = {
		TRXN_START,
		TRXN_WRITE(p, 4),
		TRXN_END,
	};

	/* Copy to scratchpad -- use CRC16 if write to end of page, but don't force it */
	memcpy(&p[3], data, size);
	if ((offset + size) & 0x1F) {	/* to end of page */
		RETURN_BAD_IF_BAD(BUS_transaction(tcopy, pn)) ;
	} else {
		RETURN_BAD_IF_BAD(BUS_transaction(tcopy_crc, pn)) ;
	}

	/* Re-read scratchpad and compare */
	/* Note: location of data has now shifted down a byte for E/S register */
	p[0] = _EDS_READ_SCRATCHPAD;
	RETURN_BAD_IF_BAD(BUS_transaction(tread, pn)) ;

	/* write Scratchpad to SRAM */
	p[0] = _EDS_COPY_SCRATCHPAD;
	return BUS_transaction(twrite, pn) ;
}

/* Read a few bytes within a page */
static GOOD_OR_BAD OW_r_mem_small(BYTE * data, size_t size, off_t offset, struct parsedname * pn)
{
	BYTE p[3] = { _EDS_READ_MEMMORY_NO_CRC, LOW_HIGH_ADDRESS(offset), };
	struct transaction_log t[] = {
		TRXN_START,
		TRXN_WRITE3(p),
		TRXN_READ( data, size),
		TRXN_END,
	};

	return BUS_transaction(t, pn);
}
