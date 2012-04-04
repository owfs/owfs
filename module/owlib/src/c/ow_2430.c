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
#include "ow_2430.h"

/* ------- Prototypes ----------- */

/* DS2423 counter */
READ_FUNCTION(FS_r_memory);
WRITE_FUNCTION(FS_w_memory);
READ_FUNCTION(FS_r_status);
READ_FUNCTION(FS_r_application);
WRITE_FUNCTION(FS_w_application);

#define _DS2430A_MEM_SIZE 32

/* ------- Structures ----------- */

static struct filetype DS2430A[] = {
	F_STANDARD,
	{"memory", _DS2430A_MEM_SIZE, NON_AGGREGATE, ft_binary, fc_link, FS_r_memory, FS_w_memory, VISIBLE, NO_FILETYPE_DATA, ftt_internal,},

	{"application", 8, NON_AGGREGATE, ft_binary, fc_stable, FS_r_application, FS_w_application, VISIBLE, NO_FILETYPE_DATA, ftt_internal,},
	{"status", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_stable, FS_r_status, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA, ftt_internal,},
};

DeviceEntry(14, DS2430A, NO_GENERIC_READ, NO_GENERIC_WRITE);

#define _1W_WRITE_SCRATCHPAD 0x0F
#define _1W_READ_SCRATCHPAD 0xAA
#define _1W_COPY_SCRATCHPAD 0x55
#define _1W_COPY_SCRATCHPAD_VALIDATION_KEY 0xA5
#define _1W_READ_MEMORY 0xF0
#define _1W_READ_STATUS_REGISTER 0x66
#define _1W_STATUS_VALIDATION_KEY 0x00
#define _1W_WRITE_APPLICATION_REGISTER 0x99
#define _1W_READ_APPLICATION_REGISTER 0xC3
#define _1W_COPY_AND_LOCK_APPLICATION_REGISTER 0x5A

/* ------- Functions ------------ */

/* DS2502 */
static GOOD_OR_BAD OW_w_mem(const BYTE * data, size_t size, off_t offset, const struct parsedname *pn);
static GOOD_OR_BAD OW_r_mem(BYTE * data, size_t size, off_t offset, const struct parsedname *pn);
static GOOD_OR_BAD OW_w_app(const BYTE * data, size_t size, off_t offset, const struct parsedname *pn);
static GOOD_OR_BAD OW_r_app(BYTE * data, size_t size, off_t offset, const struct parsedname *pn);
static GOOD_OR_BAD OW_r_status(BYTE * data, const struct parsedname *pn);

/* DS2430A memory */
static ZERO_OR_ERROR FS_r_memory(struct one_wire_query *owq)
{
	OWQ_length(owq) = OWQ_size(owq) ;
	return GB_to_Z_OR_E(OW_r_mem((BYTE *) OWQ_buffer(owq), OWQ_size(owq), (size_t) OWQ_offset(owq), PN(owq))) ;
}

/* DS2430A memory */
static ZERO_OR_ERROR FS_r_application(struct one_wire_query *owq)
{
	return GB_to_Z_OR_E(OW_r_app((BYTE *) OWQ_buffer(owq), OWQ_size(owq), (size_t) OWQ_offset(owq), PN(owq))) ;
}

static ZERO_OR_ERROR FS_w_memory(struct one_wire_query *owq)
{
	return GB_to_Z_OR_E(OW_w_mem((BYTE *) OWQ_buffer(owq), OWQ_size(owq), (size_t) OWQ_offset(owq), PN(owq))) ;
}

static ZERO_OR_ERROR FS_w_application(struct one_wire_query *owq)
{
	return GB_to_Z_OR_E(OW_w_app((BYTE *) OWQ_buffer(owq), OWQ_size(owq), (size_t) OWQ_offset(owq), PN(owq))) ;
}

static ZERO_OR_ERROR FS_r_status(struct one_wire_query *owq)
{
	BYTE data ;
	ZERO_OR_ERROR z_or_e = OW_r_status(&data, PN(owq)) ;

	OWQ_U(owq) = data ;
	return z_or_e ;
}

/* Byte-oriented write */
static GOOD_OR_BAD OW_w_mem(const BYTE * data, size_t size, off_t offset, const struct parsedname *pn)
{
	BYTE scratch[_DS2430A_MEM_SIZE];
	BYTE vr[] = { _1W_READ_SCRATCHPAD, BYTE_MASK(offset), };
	BYTE of[] = { _1W_WRITE_SCRATCHPAD, BYTE_MASK(offset), };
	BYTE cp[] = { _1W_COPY_SCRATCHPAD, _1W_COPY_SCRATCHPAD_VALIDATION_KEY, };
	struct transaction_log t[] = {
		TRXN_START,
		TRXN_WRITE2(of),
		TRXN_WRITE(data, size),
		TRXN_START,
		TRXN_WRITE2(vr),
		TRXN_READ(scratch, size),
		TRXN_COMPARE(data,scratch,size),
		TRXN_START,
		TRXN_WRITE2(cp),
		TRXN_DELAY(10),
		TRXN_END,
	};

	/* load scratch pad if incomplete write */
	if ( size != _DS2430A_MEM_SIZE ) {
		RETURN_BAD_IF_BAD( OW_r_mem( scratch, 0, 0x00, pn)) ;
	}

	/* write data to scratchpad */
	/* read back the scratchpad */
	/* copy scratchpad to memory */
	return BUS_transaction(t, pn);
}

/* Byte-oriented write */
static GOOD_OR_BAD OW_r_mem( BYTE * data, size_t size, off_t offset, const struct parsedname *pn)
{
	BYTE fo[] = { _1W_READ_MEMORY, BYTE_MASK(offset), };
	struct transaction_log t[] = {
		TRXN_START,
		TRXN_WRITE2(fo),
		TRXN_READ( data, size ) ,
		TRXN_END,
	};

	return BUS_transaction(t, pn);
}

static GOOD_OR_BAD OW_w_app(const BYTE * data, size_t size, off_t offset, const struct parsedname *pn)
{
	BYTE nn[] = { _1W_WRITE_APPLICATION_REGISTER, BYTE_MASK(offset), };
	struct transaction_log t[] = {
		TRXN_START,
		TRXN_WRITE2(nn),
		TRXN_WRITE(data, size),
		TRXN_END,
	};

	return BUS_transaction(t, pn);
}

static GOOD_OR_BAD OW_r_app(BYTE * data, size_t size, off_t offset, const struct parsedname *pn)
{
	BYTE c3[] = { _1W_READ_APPLICATION_REGISTER, BYTE_MASK(offset), };
	struct transaction_log t[] = {
		TRXN_START,
		TRXN_WRITE2(c3),
		TRXN_READ(data, size),
		TRXN_END,
	};
	return BUS_transaction(t, pn) ;
}

static GOOD_OR_BAD OW_r_status(BYTE * data, const struct parsedname *pn)
{
	BYTE ss[] = { _1W_READ_STATUS_REGISTER, _1W_STATUS_VALIDATION_KEY };
	struct transaction_log t[] = {
		TRXN_START,
		TRXN_WRITE2(ss),
		TRXN_READ1(data),
		TRXN_END,
	};
	return BUS_transaction(t, pn);
}
