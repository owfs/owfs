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

// DS1954 Cryptographics iButton
// DS1957 ibutton
// From datasheet in the web -- hard to find info

#include <config.h>
#include "owfs_config.h"
#include "ow_1954.h"

/* ------- Prototypes ----------- */

/* DS1902 ibutton memory */
READ_FUNCTION(FS_r_ipr);
WRITE_FUNCTION(FS_w_ipr);
READ_FUNCTION(FS_r_io);
WRITE_FUNCTION(FS_w_io);
READ_FUNCTION(FS_r_status);
WRITE_FUNCTION(FS_w_status);
WRITE_FUNCTION(FS_reset);

/* ------- Structures ----------- */

static struct filetype DS1954[] = {
	F_STANDARD,
	{"ipr", 128, NON_AGGREGATE, ft_binary, fc_volatile, FS_r_ipr, FS_w_ipr, VISIBLE, NO_FILETYPE_DATA, },
	{"io_buffer", 8, NON_AGGREGATE, ft_binary, fc_volatile, FS_r_io, FS_w_io, VISIBLE, NO_FILETYPE_DATA, },
	{"status", 4, NON_AGGREGATE, ft_binary, fc_volatile, FS_r_status, FS_w_status, VISIBLE, NO_FILETYPE_DATA, },
	{"reset", 1, NON_AGGREGATE, ft_yesno, fc_volatile, NO_READ_FUNCTION, FS_reset, VISIBLE, NO_FILETYPE_DATA, },
};

DeviceEntryExtended(16, DS1954, DEV_ovdr, NO_GENERIC_READ, NO_GENERIC_WRITE);

#define _1W_WRITE_IPR 0x0F
#define _1W_READ_IPR 0xAA
#define _1W_WRITE_IO_BUFFER 0x2D
#define _1W_READ_IO_BUFFER 0x22
#define _1W_START_PROGRAM 0x77
#define _1W_CONTINUE_PROGRAM 0x87
#define _1W_RESET_MICRO 0xDD
#define _1W_WRITE_STATUS 0xD2
#define _1W_READ_STATUS 0xE1

#define _1W_WRITE_RELEASE_SEQUENCE 0x9D,0xB3
#define _1W_READ_RELEASE_SEQUENCE 0x62,0x4C
#define _1W_START_RELEASE_SEQUENCE 0x6D,0x43
#define _1W_CONTINUE_RELEASE_SEQUENCE 0x5D,0x73
#define _1W_RESET_RELEASE_SEQUENCE 0x92,0xBC
#define _1W_STATUS_RELEASE_SEQUENCE 0x51,0x7F

/* ------- Functions ------------ */

/* DS1957 */
static GOOD_OR_BAD OW_w_ipr(size_t size, BYTE * data, struct parsedname * pn);
static GOOD_OR_BAD OW_r_ipr(struct one_wire_query *owq);
static GOOD_OR_BAD OW_w_io(struct one_wire_query *owq);
static GOOD_OR_BAD OW_r_io(struct one_wire_query *owq);
static GOOD_OR_BAD OW_w_status(struct one_wire_query *owq);
static GOOD_OR_BAD OW_r_status(struct one_wire_query *owq);
static GOOD_OR_BAD OW_reset(struct one_wire_query *owq);

/* 1954 */
static ZERO_OR_ERROR FS_w_ipr(struct one_wire_query *owq)
{
	return GB_to_Z_OR_E( OW_w_ipr(OWQ_size(owq), (BYTE*) OWQ_buffer(owq),PN(owq)) ) ;
}

static ZERO_OR_ERROR FS_r_ipr(struct one_wire_query *owq)
{
	return GB_to_Z_OR_E(OW_r_ipr(owq)) ;
}

static ZERO_OR_ERROR FS_w_io(struct one_wire_query *owq)
{
	return GB_to_Z_OR_E(OW_w_io(owq)) ;
}

static ZERO_OR_ERROR FS_r_io(struct one_wire_query *owq)
{
	return GB_to_Z_OR_E(OW_r_io(owq)) ;
}

static ZERO_OR_ERROR FS_w_status(struct one_wire_query *owq)
{
	return GB_to_Z_OR_E(OW_w_status(owq)) ;
}

static ZERO_OR_ERROR FS_r_status(struct one_wire_query *owq)
{
	return GB_to_Z_OR_E(OW_r_status(owq)) ;
}

static ZERO_OR_ERROR FS_reset(struct one_wire_query *owq)
{
	return GB_to_Z_OR_E(OW_reset(owq)) ;
}

static GOOD_OR_BAD OW_w_ipr(size_t size, BYTE * data, struct parsedname * pn)
{
	BYTE p[2 + 128 + 2] = { _1W_WRITE_IPR, BYTE_MASK(size), };
	struct transaction_log t[] = {
		TRXN_START,
		TRXN_WR_CRC16(p, 2 + size, 0),
		TRXN_END,
	};
	memcpy(&p[2],data,size) ;
	return BUS_transaction(t, pn);
};

/* Read all 128 bytes, then transfer over what's needed */
static GOOD_OR_BAD OW_r_ipr(struct one_wire_query *owq)
{
	BYTE p[2 + 128 + 2] = { _1W_READ_IPR, 128, };
	int size = OWQ_size(owq);
	struct transaction_log t[] = {
		TRXN_START,
		TRXN_WR_CRC16(p, 2, 128),
		TRXN_END,
	};
	RETURN_BAD_IF_BAD(BUS_transaction(t, PN(owq))) ;
	if (size > 128) {
		size = 128;
	}
	memcpy(OWQ_buffer(owq), &p[2], size);
	OWQ_length(owq) = size;
	return gbGOOD;
};

static GOOD_OR_BAD OW_w_status(struct one_wire_query *owq)
{
	int size = OWQ_size(owq);
	BYTE p[1 + 1 + 2] = { _1W_WRITE_STATUS, BYTE_MASK(size), };
	BYTE release[2] = { _1W_STATUS_RELEASE_SEQUENCE, };
	BYTE zero[2] = { 0x00, 0x00, };
	struct transaction_log t[] = {
		TRXN_START,
		TRXN_WR_CRC16(p, 1, 1),
		TRXN_MODIFY(release, release, 2),
		TRXN_COMPARE(release, zero, 2),
		TRXN_END,
	};
	return BUS_transaction(t, PN(owq));
};

static GOOD_OR_BAD OW_r_status(struct one_wire_query *owq)
{
	BYTE p[1 + 4 + 2] = { _1W_READ_STATUS, };
	int size = OWQ_size(owq);
	struct transaction_log t[] = {
		TRXN_START,
		TRXN_WR_CRC16(p, 1, 4),
		TRXN_END,
	};
	RETURN_BAD_IF_BAD(BUS_transaction(t, PN(owq)));
	if (size > 4) {
		size = 4;
	}
	memcpy(OWQ_buffer(owq), &p[1], size);
	OWQ_length(owq) = size;
	return gbGOOD;
};

static GOOD_OR_BAD OW_w_io(struct one_wire_query *owq)
{
	int size = OWQ_size(owq);
	BYTE p[2 + 8 + 2] = { _1W_WRITE_IO_BUFFER, BYTE_MASK(size), };
	BYTE release[2] = { _1W_WRITE_RELEASE_SEQUENCE, };
	BYTE zero[2] = { 0x00, 0x00, };
	struct transaction_log t[] = {
		TRXN_START,
		TRXN_WR_CRC16(p, 2 + size, 0),
		TRXN_MODIFY(release, release, 2),
		TRXN_COMPARE(release, zero, 2),
		TRXN_END,
	};
	return BUS_transaction(t, PN(owq));
};

static GOOD_OR_BAD OW_r_io(struct one_wire_query *owq)
{
	int size = OWQ_size(owq);
	BYTE p[2 + 8 + 2] = { _1W_READ_IO_BUFFER, BYTE_MASK(size), };
	BYTE release[2] = { _1W_READ_RELEASE_SEQUENCE, };
	BYTE zero[2] = { 0x00, 0x00, };
	struct transaction_log t[] = {
		TRXN_START,
		TRXN_WR_CRC16(p, 2, size),
		TRXN_MODIFY(release, release, 2),
		TRXN_COMPARE(release, zero, 2),
		TRXN_END,
	};
	RETURN_BAD_IF_BAD(BUS_transaction(t, PN(owq))) ;
	memcpy(OWQ_buffer(owq), &p[2], size);
	OWQ_length(owq) = size;
	return gbGOOD;
};

static GOOD_OR_BAD OW_reset(struct one_wire_query *owq)
{
	BYTE p[] = { _1W_RESET_MICRO, };
	BYTE release[2] = { _1W_RESET_RELEASE_SEQUENCE, };
	BYTE zero[2] = { 0x00, 0x00, };
	BYTE testbit[1];
	struct transaction_log t[] = {
		TRXN_START,
		TRXN_WRITE1(p),
		TRXN_MODIFY(release, release, 2),
		TRXN_COMPARE(release, zero, 2),
		TRXN_READ1(testbit),
		TRXN_END,
	};
	RETURN_BAD_IF_BAD(BUS_transaction(t, PN(owq)) ) ;
	if ( testbit[0] & 0x80 ) {
		return gbBAD;
	}
	return gbGOOD;
};
