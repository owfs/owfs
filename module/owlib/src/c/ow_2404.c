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
#include "ow_2404.h"

/* ------- Prototypes ----------- */

/* DS1902 ibutton memory */
READ_FUNCTION(FS_r_page);
WRITE_FUNCTION(FS_w_page);
READ_FUNCTION(FS_r_memory);
WRITE_FUNCTION(FS_w_memory);
READ_FUNCTION(FS_r_alarm);
READ_FUNCTION(FS_r_set_alarm);
WRITE_FUNCTION(FS_w_set_alarm);
READ_FUNCTION(FS_r_counter4);
WRITE_FUNCTION(FS_w_counter4);
READ_FUNCTION(FS_r_counter5);
WRITE_FUNCTION(FS_w_counter5);
READ_FUNCTION(FS_r_flag);
WRITE_FUNCTION(FS_w_flag);

/* ------- Structures ----------- */

struct aggregate A2404 = { 16, ag_numbers, ag_separate, };
struct filetype DS2404[] = {
	F_STANDARD,
	{"alarm", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_volatile, FS_r_alarm, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA,},
	{"set_alarm", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_stable, FS_r_set_alarm, FS_w_set_alarm, VISIBLE, NO_FILETYPE_DATA,},
	{"pages", PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir, fc_volatile, NO_READ_FUNCTION, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA,},
	{"pages/page", 32, &A2404, ft_binary, fc_stable, FS_r_page, FS_w_page, VISIBLE, NO_FILETYPE_DATA,},
	{"memory", 512, NON_AGGREGATE, ft_binary, fc_stable, FS_r_memory, FS_w_memory, VISIBLE, NO_FILETYPE_DATA,},
	{"running", PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_stable, FS_r_flag, FS_w_flag, VISIBLE, {c:0x10},},
	{"auto", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_stable, FS_r_flag, FS_w_flag, VISIBLE, {c:0x20},},
	{"start", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_stable, FS_r_flag, FS_w_flag, VISIBLE, {c:0x40},},
	{"delay", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_stable, FS_r_flag, FS_w_flag, VISIBLE, {c:0x80},},
	{"date", PROPERTY_LENGTH_DATE, NON_AGGREGATE, ft_date, fc_link, COMMON_r_date, COMMON_w_date, VISIBLE, {s:0x202},},
	{"udate", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_second, FS_r_counter5, FS_w_counter5, VISIBLE, {s:0x202},},
	{"interval", PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir, fc_volatile, NO_READ_FUNCTION, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA,},
	{"interval/date", PROPERTY_LENGTH_DATE, NON_AGGREGATE, ft_date, fc_link, COMMON_r_date, COMMON_w_date, VISIBLE, {s:0x207},},
	{"interval/udate", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_second, FS_r_counter5, FS_w_counter5, VISIBLE, {s:0x207},},
	{"cycle", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_second, FS_r_counter4, FS_w_counter4, VISIBLE, {s:0x20C},},
	{"trigger", PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir, fc_volatile, NO_READ_FUNCTION, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA,},
	{"trigger/date", PROPERTY_LENGTH_DATE, NON_AGGREGATE, ft_date, fc_link, COMMON_r_date, COMMON_w_date, VISIBLE, {s:0x210},},
	{"trigger/udate", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_second, FS_r_counter5, FS_w_counter5, VISIBLE, {s:0x210},},
	{"trigger/date", PROPERTY_LENGTH_DATE, NON_AGGREGATE, ft_date, fc_link, COMMON_r_date, COMMON_w_date, VISIBLE, {s:0x215},},
	{"trigger/udate", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_second, FS_r_counter5, FS_w_counter5, VISIBLE, {s:0x215},},
	{"trigger/cycle", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_second, FS_r_counter4, FS_w_counter4, VISIBLE, {s:0x21A},},
	{"readonly", PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir, fc_volatile, NO_READ_FUNCTION, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA,},
	{"readonly/memory", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_stable, FS_r_flag, FS_w_flag, VISIBLE, {c:0x08},},
	{"readonly/cycle", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_stable, FS_r_flag, FS_w_flag, VISIBLE, {c:0x04},},
	{"readonly/interval", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_stable, FS_r_flag, FS_w_flag, VISIBLE, {c:0x02},},
	{"readonly/clock", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_stable, FS_r_flag, FS_w_flag, VISIBLE, {c:0x01},},
};

DeviceEntryExtended(04, DS2404, DEV_alarm);

#define _1W_WRITE_SCRATCHPAD 0x0F
#define _1W_READ_SCRATCHPAD 0xAA
#define _1W_COPY_SCRATCHPAD 0x55
#define _1W_READ_MEMORY 0xF0

/* ------- Functions ------------ */

/* DS2404 */
static GOOD_OR_BAD OW_w_mem(BYTE * data, size_t size, off_t offset, struct parsedname *pn);
static GOOD_OR_BAD OW_r_ulong(uint64_t * L, size_t size, off_t offset, struct parsedname *pn);
static GOOD_OR_BAD OW_w_ulong(uint64_t * L, size_t size, off_t offset, struct parsedname *pn);
static void OW_reset(struct parsedname *pn) ;

static UINT Avals[] = { 0, 1, 10, 11, 100, 101, 110, 111, };

/* 2404 */
static ZERO_OR_ERROR FS_r_page(struct one_wire_query *owq)
{
	size_t pagesize = 32;
	ZERO_OR_ERROR read_status = COMMON_read_memory_F0(owq, OWQ_pn(owq).extension, pagesize) ;

	OW_reset(PN(owq)) ; // DS2404 needs this to release for 3-wire communication
	return read_status ;
}

static ZERO_OR_ERROR FS_r_memory(struct one_wire_query *owq)
{
	/* read is consecutive, unchecked. No paging */
	ZERO_OR_ERROR read_status = COMMON_read_memory_F0(owq, 0, 0) ;

	OW_reset(PN(owq)) ; // DS2404 needs this to release for 3-wire communication
	return read_status ;
}

static ZERO_OR_ERROR FS_w_page(struct one_wire_query *owq)
{
	size_t pagesize = 32;
	ZERO_OR_ERROR error_code = COMMON_readwrite_paged(owq, OWQ_pn(owq).extension, pagesize, OW_w_mem) ;

	OW_reset(PN(owq)) ; // DS2404 needs this to release for 3-wire communication
	return error_code ;
}

static ZERO_OR_ERROR FS_w_memory(struct one_wire_query *owq)
{
	size_t pagesize = 32;
	ZERO_OR_ERROR error_code = COMMON_readwrite_paged(owq, 0, pagesize, OW_w_mem) ;

	/* paged write */
	if (error_code != 0) {
		error_code = -EFAULT ;
	}
	OW_reset(PN(owq)) ; // DS2404 needs this to release for 3-wire communication
	return error_code ;
}

/* set clock */
static ZERO_OR_ERROR FS_w_counter5(struct one_wire_query *owq)
{
	struct parsedname *pn = PN(owq);
	uint64_t L = ((uint64_t) OWQ_U(owq)) << 8;
	return OW_w_ulong(&L, 5, pn->selected_filetype->data.s, pn)?-EINVAL:0;
}

/* set clock */
static ZERO_OR_ERROR FS_w_counter4(struct one_wire_query *owq)
{
	struct parsedname *pn = PN(owq);
	uint64_t L = ((uint64_t) OWQ_U(owq));
	return OW_w_ulong(&L, 4, pn->selected_filetype->data.s, pn)?-EINVAL:0;
}

/* read clock */
static ZERO_OR_ERROR FS_r_counter5(struct one_wire_query *owq)
{
	struct parsedname *pn = PN(owq);
	uint64_t L;
	if (OW_r_ulong(&L, 5, pn->selected_filetype->data.s, pn)) {
		return -EINVAL;
	}
	OWQ_U(owq) = L >> 8;
	return 0;
}

/* read clock */
static ZERO_OR_ERROR FS_r_counter4(struct one_wire_query *owq)
{
	struct parsedname *pn = PN(owq);
	uint64_t L;
	if (OW_r_ulong(&L, 4, pn->selected_filetype->data.s, pn)) {
		return -EINVAL;
	}
	OWQ_U(owq) = L;
	return 0;
}

/* alarm */
static ZERO_OR_ERROR FS_w_set_alarm(struct one_wire_query *owq)
{
	BYTE c;
	switch (OWQ_U(owq)) {
	case 0:
		c = 0 << 3;
		break;
	case 1:
		c = 1 << 3;
		break;
	case 10:
		c = 2 << 3;
		break;
	case 11:
		c = 3 << 3;
		break;
	case 100:
		c = 4 << 3;
		break;
	case 101:
		c = 5 << 3;
		break;
	case 110:
		c = 6 << 3;
		break;
	case 111:
		c = 7 << 3;
		break;
	default:
		return -ERANGE;
	}
	if (OW_w_mem(&c, 1, 0x200, PN(owq))) {
		return -EINVAL;
	}
	return 0;
}

static ZERO_OR_ERROR FS_r_alarm(struct one_wire_query *owq)
{
	BYTE c;
	OWQ_allocate_struct_and_pointer(owq_alarm);

	OWQ_create_temporary(owq_alarm, (char *) &c, 1, 0x0200, PN(owq));
	if (COMMON_read_memory_F0(owq_alarm, 0, 0)) {
		return -EINVAL;
	}
	OWQ_U(owq) = Avals[c & 0x07];
	return 0;
}

static ZERO_OR_ERROR FS_r_set_alarm(struct one_wire_query *owq)
{
	BYTE c;
	OWQ_allocate_struct_and_pointer(owq_alarm);

	OWQ_create_temporary(owq_alarm, (char *) &c, 1, 0x0200, PN(owq));
	if (COMMON_read_memory_F0(owq_alarm, 0, 0)) {
		return -EINVAL;
	}
	OWQ_U(owq) = Avals[(c >> 3) & 0x07];
	return 0;
}

/* write flag */
static ZERO_OR_ERROR FS_w_flag(struct one_wire_query *owq)
{
	struct parsedname *pn = PN(owq);
	BYTE cr;
	BYTE fl = pn->selected_filetype->data.c;
	OWQ_allocate_struct_and_pointer(owq_flag);
	
	OWQ_create_temporary(owq_flag, (char *) &cr, 1, 0x0201, pn);
	if (COMMON_read_memory_F0(owq_flag, 0, 0)) {
		return -EINVAL;
	}
	if (OWQ_Y(owq)) {
		if (cr & fl) {
			return 0;
		}
	} else {
		if ((cr & fl) == 0) {
			return 0;
		}
	}
	cr ^= fl;					/* flip the bit */
	if (OW_w_mem(&cr, 1, 0x0201, pn)) {
		return -EINVAL;
	}
	return 0;
}

/* read flag */
static ZERO_OR_ERROR FS_r_flag(struct one_wire_query *owq)
{
	struct parsedname *pn = PN(owq);
	BYTE cr;
	BYTE fl = pn->selected_filetype->data.c;
	OWQ_allocate_struct_and_pointer(owq_flag);

	OWQ_create_temporary(owq_flag, (char *) &cr, 1, 0x0201, pn);
	if (COMMON_read_memory_F0(owq_flag, 0, 0)) {
		return -EINVAL;
	}
	OWQ_Y(owq) = (cr & fl) ? 1 : 0;
	return 0;
}

/* PAged access -- pre-screened */
static GOOD_OR_BAD OW_w_mem(BYTE * data, size_t size, off_t offset, struct parsedname *pn)
{
	BYTE p[4 + 32] = { _1W_WRITE_SCRATCHPAD, LOW_HIGH_ADDRESS(offset), };
	struct transaction_log tcopy[] = {
		TRXN_START,
		TRXN_WRITE3(p),
		TRXN_WRITE(data, size),
		TRXN_END,
	};
	struct transaction_log tread[] = {
		TRXN_START,
		TRXN_WRITE1(p),
		TRXN_READ(&p[1], 3 + size),
		TRXN_COMPARE(data, &p[4], size),
		TRXN_END,
	};
	struct transaction_log tsram[] = {
		TRXN_START,
		TRXN_WRITE(p, 4),
		TRXN_END,
	};

	/* Copy to scratchpad */
	if (BUS_transaction(tcopy, pn)) {
		return gbBAD;
	}

	/* Re-read scratchpad and compare */
	p[0] = _1W_READ_SCRATCHPAD;
	if (BUS_transaction(tread, pn)) {
		return gbBAD;
	}

	/* Copy Scratchpad to SRAM */
	p[0] = _1W_COPY_SCRATCHPAD;
	if (BUS_transaction(tsram, pn)) {
		return gbBAD;
	}

	UT_delay(32);
	return gbGOOD;
}

/* read 4 or 5 byte number */
static GOOD_OR_BAD OW_r_ulong(uint64_t * L, size_t size, off_t offset, struct parsedname *pn)
{
	BYTE data[5] = { 0x00, 0x00, 0x00, 0x00, 0x00, };
	OWQ_allocate_struct_and_pointer(owq_ulong);

	OWQ_create_temporary(owq_ulong, (char *) data, size, offset, pn);
	if (size > 5) {
		return gbBAD;
	}
	if (COMMON_read_memory_F0(owq_ulong, 0, 0)) {
		return gbBAD;
	}
	L[0] = ((uint64_t) data[0])
		+ (((uint64_t) data[1]) << 8)
		+ (((uint64_t) data[2]) << 16)
		+ (((uint64_t) data[3]) << 24)
		+ (((uint64_t) data[4]) << 32);
	return gbGOOD;
}

/* write 4 or 5 byte number */
static GOOD_OR_BAD OW_w_ulong(uint64_t * L, size_t size, off_t offset, struct parsedname *pn)
{
	BYTE data[5] = { 0x00, 0x00, 0x00, 0x00, 0x00, };
	if (size > 5) {
		return -ERANGE;
	}
	data[0] = BYTE_MASK(L[0]);
	data[1] = BYTE_MASK(L[0] >> 8);
	data[2] = BYTE_MASK(L[0] >> 16);
	data[3] = BYTE_MASK(L[0] >> 24);
	data[4] = BYTE_MASK(L[0] >> 32);
	if (OW_w_mem(data, size, offset, pn)) {
		return gbBAD;
	}
	return gbGOOD;
}

static void OW_reset(struct parsedname *pn)
{
	struct transaction_log t[] = {
		TRXN_RESET,
	};
	BUS_transaction(t, pn) ;
}
