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

/* Changes
    7/2004 Extensive improvements based on input from Serg Oskin
*/

#include <config.h>
#include "owfs_config.h"
#include "ow_standard.h"

/* ------- Prototypes ----------- */
static GOOD_OR_BAD COMMON_write_eprom_mem(const BYTE * data, size_t size, off_t offset, const struct parsedname *pn);
static GOOD_OR_BAD OW_write_eprom_byte(BYTE code, BYTE data, off_t offset, const struct parsedname *pn);

#define _1W_WRITE_MEMORY         0x0F
#define _1W_WRITE_STATUS         0x55

// use owq instead of components
ZERO_OR_ERROR COMMON_write_eprom_mem_owq(struct one_wire_query * owq)
{
	return GB_to_Z_OR_E(COMMON_write_eprom_mem( OWQ_explode(owq)));
}

static GOOD_OR_BAD COMMON_write_eprom_mem(const BYTE * data, size_t size, off_t offset, const struct parsedname *pn)
{
	int byte_number;
	for (byte_number = 0; byte_number < (int) size; ++byte_number) {
		RETURN_BAD_IF_BAD( OW_write_eprom_byte(_1W_WRITE_MEMORY, data[byte_number], offset + byte_number, pn) ) ;
	}
	return gbGOOD;
}

static GOOD_OR_BAD OW_write_eprom_byte(BYTE code, BYTE data, off_t offset, const struct parsedname *pn)
{
	BYTE p[6] = { code, LOW_HIGH_ADDRESS(offset), data, };
	struct transaction_log t[] = {
		TRXN_START,
		TRXN_WR_CRC16(p, 4, 0),
		TRXN_PROGRAM,
		TRXN_END,
	};

	return BUS_transaction(t, pn) ;
}
