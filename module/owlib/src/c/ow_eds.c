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
READ_FUNCTION(FS_r_memory);
WRITE_FUNCTION(FS_w_memory);
READ_FUNCTION(FS_r_tag);
READ_FUNCTION(FS_temperature);

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
} ;
#define N_eds_types (sizeof(EDS_types) / sizeof( struct eds_types))

#define _EDS_TAG_LENGTH 8

/* ------- Structures ----------- */

struct aggregate AEDS = { _EDS_PAGES, ag_numbers, ag_separate, };
struct filetype EDS[] = {
	F_STANDARD,
	{"tag", _EDS_TAG_LENGTH, NON_AGGREGATE, ft_ascii, fc_stable, FS_r_tag, NO_WRITE_FUNCTION, INVISIBLE, NO_FILETYPE_DATA,},
	{"memory", _EDS_PAGES * _EDS_PAGESIZE, NON_AGGREGATE, ft_binary, fc_stable, FS_r_memory, FS_w_memory, VISIBLE, NO_FILETYPE_DATA,},
	{"pages", PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir, fc_volatile, NO_READ_FUNCTION, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA,},
	{"pages/page", _EDS_PAGESIZE, &AEDS, ft_binary, fc_stable, FS_r_page, FS_w_page, VISIBLE, NO_FILETYPE_DATA,},
	{"memory", _EDS_PAGES * _EDS_PAGESIZE, NON_AGGREGATE, ft_binary, fc_stable, FS_r_memory, FS_w_memory, VISIBLE, NO_FILETYPE_DATA,},
	{"temperature", PROPERTY_LENGTH_TEMP, NON_AGGREGATE, ft_temperature, fc_volatile, FS_temperature, NO_WRITE_FUNCTION, EDS_visible, {i:_EDS_CHIPS_TEMPERATURE},},
};

DeviceEntryExtended(7E, EDS, DEV_temp | DEV_alarm);

/* ------- Functions ------------ */
static GOOD_OR_BAD OW_w_mem(BYTE * data, size_t size, off_t offset, struct parsedname *pn) ;

/* Persistent storage */
//static struct internal_prop ip_cum = { "CUM", fc_persistent };
MakeInternalProp(TAG, fc_persistent);	// cumulative

/* EDS memory */
// Only used to set visibility for specific 
static ZERO_OR_ERROR FS_r_tag(struct one_wire_query *owq)
{
	size_t size = _EDS_PAGESIZE ;
	BYTE data[_EDS_PAGESIZE] ;
	struct parsedname * pn = PN(owq) ;
	UINT tag = 0x0000 ;

	if (Cache_Get_Internal_Strict(&tag, sizeof(UINT), InternalProp(TAG), pn) == 0 ) {
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
			if ( strncasecmp( (char *) data, EDS_types[tag_type].name, _EDS_TAG_LENGTH ) == 0 ) {
				tag = EDS_types[tag_type].bit ;
				break ;
			}
		}
		if ( OWQ_format_output_offset_and_size( (char *) data, _EDS_TAG_LENGTH, owq ) ) {
			return -EINVAL ;
		}
		Cache_Add_Internal(&tag, sizeof(UINT), InternalProp(TAG), pn) ;
		return 0 ;
	}
	return -EINVAL ;
}

static ZERO_OR_ERROR FS_temperature(struct one_wire_query *owq)
{
	size_t size = _EDS_PAGESIZE ;
	BYTE data[_EDS_PAGESIZE]  = { 0, 0, 0, } ;
	ZERO_OR_ERROR z_or_e = FS_r_sibling_binary(data,&size,"pages/page.1",owq) ; // read device memory

	OWQ_F(owq) = ( (_FLOAT)UT_uint16(&data[2]) ) / 16. ;
	return z_or_e ;
}

static ZERO_OR_ERROR FS_r_memory(struct one_wire_query *owq)
{
	size_t pagesize = 32;
	return GB_to_Z_OR_E(COMMON_OWQ_readwrite_paged(owq, 0, pagesize, COMMON_read_memory_F0)) ;
}

static enum e_visibility EDS_visible(const struct parsedname * pn) {
	UINT tag ;
	if (Cache_Get_Internal_Strict(&tag, sizeof(UINT), InternalProp(TAG), pn) != 0 ) {	// tag doesn't (yet) exist
		struct one_wire_query * owq = OWQ_create_from_path(pn->path) ; // for read
		size_t size = _EDS_TAG_LENGTH ;
		BYTE data[size] ;
		if ( owq != NULL) {
			FS_r_sibling_binary(data,&size,"tag",owq) ;
			OWQ_destroy(owq) ;
		}
	}
	if (Cache_Get_Internal_Strict(&tag, sizeof(UINT), InternalProp(TAG), pn) != 0 ) {	// tag doesn't (yet) exist
		LEVEL_DEBUG("Cannot check visibility tag type for this entry");
		return visible_now ; // assume visible
	}
	return ( tag & pn->selected_filetype->data.u ) ? visible_now : visible_not_now ;
}

static ZERO_OR_ERROR FS_r_page(struct one_wire_query *owq)
{
	size_t pagesize = 32;
	return GB_to_Z_OR_E(COMMON_OWQ_readwrite_paged(owq, OWQ_pn(owq).extension, pagesize, COMMON_read_memory_F0)) ;
}

static ZERO_OR_ERROR FS_w_memory(struct one_wire_query *owq)
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
	struct parsedname * pn = PN(owq) ;
	return GB_to_Z_OR_E( OW_w_mem( (BYTE *) OWQ_buffer(owq),OWQ_size(owq),OWQ_offset(owq) + _EDS_PAGESIZE * pn->extension,pn) );
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
