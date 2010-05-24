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
#include "ow_2436.h"

/* ------- Prototypes ----------- */

/* DS2436 Battery */
READ_FUNCTION(FS_r_page);
WRITE_FUNCTION(FS_w_page);
READ_FUNCTION(FS_temp);
READ_FUNCTION(FS_volts);

/* ------- Structures ----------- */

struct aggregate A2436 = { 3, ag_numbers, ag_separate, };
struct filetype DS2436[] = {
	F_STANDARD,
	{"pages", PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir, fc_volatile, NO_READ_FUNCTION, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA,},
	{"pages/page", 32, &A2436, ft_binary, fc_stable, FS_r_page, FS_w_page, VISIBLE, NO_FILETYPE_DATA,},
	{"volts", PROPERTY_LENGTH_FLOAT, NON_AGGREGATE, ft_float, fc_volatile, FS_volts, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA,},
	{"temperature", PROPERTY_LENGTH_TEMP, NON_AGGREGATE, ft_temperature, fc_volatile, FS_temp, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA,},
};

DeviceEntry(1B, DS2436);

#define _1W_READ_SCRATCHPAD 0x11
#define _1W_WRITE_SCRATCHPAD 0x17
#define _1W_COPY_SP1_TO_NV1	0x22
#define _1W_COPY_SP2_TO_NV2	0x25
#define _1W_COPY_SP3_TO_NV3	0x28
#define _1W_COPY_NV1_TO_SP1 0x71
#define _1W_COPY_NV2_TO_SP2 0x77
#define _1W_COPY_NV3_TO_SP3 0x7A
#define _1W_LOCK_NV1 0x43
#define _1W_UNLOCK_NV1 0x44
#define _1W_CONVERT_T 0xD2
#define _1W_CONVERT_V 0xB4
#define _1W_READ_REGISTERS 0xB2
#define _1W_INCREMENT_CYCLE 0xB5
#define _1W_RESET_CYCLE_COUNTER 0xB8

#define _ADDRESS_TEMPERATURE 0x60
#define _ADDRESS_VOLTAGE 0x77

/* ------- Functions ------------ */

/* DS2436 */
static GOOD_OR_BAD OW_r_page(BYTE * p, size_t size, off_t offset, const struct parsedname *pn);
static GOOD_OR_BAD OW_w_page(const BYTE * p, size_t size, off_t offset, const struct parsedname *pn);
static GOOD_OR_BAD OW_temp(_FLOAT * T, const struct parsedname *pn);
static GOOD_OR_BAD OW_volts(_FLOAT * V, const struct parsedname *pn);

/* 2436 A/D */
static ZERO_OR_ERROR FS_r_page(struct one_wire_query *owq)
{
	struct parsedname *pn = PN(owq);
	return GB_to_Z_OR_E(OW_r_page((BYTE *) OWQ_buffer(owq), OWQ_size(owq), OWQ_offset(owq) + ((pn->extension) << 5), pn)) ;
}

static ZERO_OR_ERROR FS_w_page(struct one_wire_query *owq)
{
	struct parsedname *pn = PN(owq);
	return GB_to_Z_OR_E(OW_w_page((BYTE *) OWQ_buffer(owq), OWQ_size(owq), OWQ_offset(owq) + ((pn->extension) << 5), pn)) ;
}

static ZERO_OR_ERROR FS_temp(struct one_wire_query *owq)
{
	return GB_to_Z_OR_E(OW_temp(&OWQ_F(owq), PN(owq))) ;
}

static ZERO_OR_ERROR FS_volts(struct one_wire_query *owq)
{
	return GB_to_Z_OR_E(OW_volts(&OWQ_F(owq), PN(owq))) ;
}

/* DS2436 simple battery */
/* only called for a single page, and that page is 0,1,2 only*/
static GOOD_OR_BAD OW_r_page(BYTE * data, size_t size, off_t offset, const struct parsedname *pn)
{
	int pagesize = 32;
	int page = offset / pagesize;
	BYTE scratchin[] = { _1W_READ_SCRATCHPAD, offset, };
	static BYTE copyin[] = { _1W_COPY_NV1_TO_SP1, _1W_COPY_NV2_TO_SP2, _1W_COPY_NV3_TO_SP3, };
	BYTE *copy = &copyin[page];
	struct transaction_log tcopy[] = {
		TRXN_START,
		TRXN_WRITE1(copy),
		TRXN_DELAY(10),
		TRXN_END,
	};
	struct transaction_log tscratch[] = {
		TRXN_START,
		TRXN_WRITE2(scratchin),
		TRXN_READ(data, size),
		TRXN_END,
	};

	RETURN_BAD_IF_BAD(BUS_transaction(tcopy, pn)) ;

	return BUS_transaction(tscratch, pn) ;
}

/* only called for a single page, and that page is 0,1,2 only*/
static GOOD_OR_BAD OW_w_page(const BYTE * data, size_t size, off_t offset, const struct parsedname *pn)
{
	int pagesize = 32;
	int page = offset / pagesize;
	BYTE scratchin[] = { _1W_READ_SCRATCHPAD, offset, };
	BYTE scratchout[] = { _1W_WRITE_SCRATCHPAD, offset, };
	BYTE p[pagesize];
	static BYTE copyout[] = { _1W_COPY_SP1_TO_NV1, _1W_COPY_SP2_TO_NV2, _1W_COPY_SP3_TO_NV3, };
	BYTE *copy = &copyout[page];
	struct transaction_log twrite[] = {
		TRXN_START,
		TRXN_WRITE2(scratchout),
		TRXN_WRITE(data, size),
		TRXN_END,
	};
	struct transaction_log tread[] = {
		TRXN_START,
		TRXN_WRITE2(scratchin),
		TRXN_READ(p, size),
		TRXN_COMPARE(data, p, size),
		TRXN_END,
	};
	struct transaction_log tcopy[] = {
		TRXN_START,
		TRXN_WRITE1(copy),
		TRXN_DELAY(10),
		TRXN_END,
	};

	RETURN_BAD_IF_BAD(BUS_transaction(twrite, pn)) ;
	RETURN_BAD_IF_BAD(BUS_transaction(tread, pn)) ;
	return BUS_transaction(tcopy, pn) ;
}

static GOOD_OR_BAD OW_temp(_FLOAT * T, const struct parsedname *pn)
{
	BYTE d2[] = { _1W_CONVERT_T, };
	BYTE b2[] = { _1W_READ_REGISTERS, _ADDRESS_TEMPERATURE, };
	BYTE t[2];
	struct transaction_log tconvert[] = {
		TRXN_START,
		TRXN_WRITE1(d2),
		TRXN_DELAY(10),
		TRXN_END,
	};
	struct transaction_log tdata[] = {
		TRXN_START,
		TRXN_WRITE2(b2),
		TRXN_READ3(t),
		TRXN_END,
	};

	// initiate conversion
	RETURN_BAD_IF_BAD(BUS_transaction(tconvert, pn)) ;


	/* Get data */
	RETURN_BAD_IF_BAD(BUS_transaction(tdata, pn)) ;

	// success
	//printf("Temp bytes %0.2X %0.2X\n",t[0],t[1]);
	//printf("temp int=%d\n",((int)((int8_t)t[1])));

	//T[0] = ((int)((int8_t)t[1])) + .00390625*t[0] ;
	T[0] = UT_int16(t) / 256.;
	return gbGOOD;
}

static GOOD_OR_BAD OW_volts(_FLOAT * V, const struct parsedname *pn)
{
	BYTE b4[] = { _1W_CONVERT_V, };
	BYTE b2[] = { _1W_READ_REGISTERS, _ADDRESS_VOLTAGE, };
	BYTE v[2];
	struct transaction_log tconvert[] = {
		TRXN_START,
		TRXN_WRITE1(b4),
		TRXN_DELAY(10),
		TRXN_END,
	};
	struct transaction_log tdata[] = {
		TRXN_START,
		TRXN_WRITE2(b2),
		TRXN_READ2(v),
		TRXN_END,
	};

	// initiate conversion
	RETURN_BAD_IF_BAD(BUS_transaction(tconvert, pn)) ;

	/* Get data */
	RETURN_BAD_IF_BAD( BUS_transaction(tdata, pn)) ;

	// success
	//V[0] = .01 * (_FLOAT)( ( ((uint32_t)v[1]) <<8 )|v[0] ) ;
	V[0] = .01 * (_FLOAT) (UT_uint16(v));
	return gbGOOD;
}
