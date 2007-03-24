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
#include "ow_2415.h"

/* ------- Prototypes ----------- */

/* DS2415/DS1904 Digital clock in a can */
READ_FUNCTION(FS_r_counter);
WRITE_FUNCTION(FS_w_counter);
READ_FUNCTION(FS_r_date);
WRITE_FUNCTION(FS_w_date);
READ_FUNCTION(FS_r_run);
WRITE_FUNCTION(FS_w_run);
READ_FUNCTION(FS_r_flags);
WRITE_FUNCTION(FS_w_flags);
READ_FUNCTION(FS_r_enable);
WRITE_FUNCTION(FS_w_enable);
READ_FUNCTION(FS_r_interval);
WRITE_FUNCTION(FS_w_interval);
READ_FUNCTION(FS_r_itime);
WRITE_FUNCTION(FS_w_itime);

/* ------- Structures ----------- */

struct filetype DS2415[] = {
	F_STANDARD,
  {"flags", 1, NULL, ft_unsigned, fc_stable, {o: FS_r_flags}, {o: FS_w_flags}, {v:NULL},},
  {"running", 1, NULL, ft_yesno, fc_stable, {o: FS_r_run}, {o: FS_w_run}, {v:NULL},},
  {"udate", 12, NULL, ft_unsigned, fc_second, {o: FS_r_counter}, {o: FS_w_counter}, {v:NULL},},
  {"date", 24, NULL, ft_date, fc_second, {o: FS_r_date}, {o: FS_w_date}, {v:NULL},},
};

DeviceEntry(24, DS2415);

struct filetype DS2417[] = {
	F_STANDARD,
  {"enable", 1, NULL, ft_yesno, fc_stable, {o: FS_r_enable}, {o: FS_w_enable}, {v:NULL},},
  {"interval", 1, NULL, ft_integer, fc_stable, {o: FS_r_interval}, {o: FS_w_interval}, {v:NULL},},
  {"itime", 1, NULL, ft_integer, fc_stable, {o: FS_r_itime}, {o: FS_w_itime}, {v:NULL},},
  {"running", 1, NULL, ft_yesno, fc_stable, {o: FS_r_run}, {o: FS_w_run}, {v:NULL},},
  {"udate", 12, NULL, ft_unsigned, fc_second, {o: FS_r_counter}, {o: FS_w_counter}, {v:NULL},},
  {"date", 24, NULL, ft_date, fc_second, {o: FS_r_date}, {o: FS_w_date}, {v:NULL},},
};

DeviceEntry(27, DS2417);

static int itimes[] = { 1, 4, 32, 64, 2048, 4096, 65536, 131072, };

/* ------- Functions ------------ */
/* DS2415/DS1904 Digital clock in a can */

/* DS1904 */
static int OW_r_clock(_DATE * d, const struct parsedname *pn);
static int OW_r_control(BYTE * cr, const struct parsedname *pn);
static int OW_w_clock(const _DATE d, const struct parsedname *pn);
static int OW_w_control(const BYTE cr, const struct parsedname *pn);

/* set clock */
static int FS_w_counter(struct one_wire_query * owq)
{
    _DATE d = (_DATE) OWQ_D(owq) ;
    if (OW_w_clock(d, PN(owq)))
		return -EINVAL;
	return 0;
}

/* set clock */
static int FS_w_date(struct one_wire_query * owq)
{
    if (OW_w_clock(OWQ_D(owq), PN(owq)))
		return -EINVAL;
	return 0;
}

/* write running */
static int FS_w_run(struct one_wire_query * owq)
{
	BYTE cr;
    if (OW_r_control(&cr, PN(owq))
        || OW_w_control((BYTE) (OWQ_Y(owq) ? cr | 0x0C : cr & 0xF3), PN(owq)))
		return -EINVAL;
	return 0;
}

/* write running */
static int FS_w_enable(struct one_wire_query * owq)
{
	BYTE cr;

    if (OW_r_control(&cr, PN(owq))
        || OW_w_control((BYTE) (OWQ_Y(owq) ? cr | 0x80 : cr & 0x7F), PN(owq)))
		return -EINVAL;
	return 0;
}

/* write flags */
static int FS_w_flags(struct one_wire_query * owq)
{
	BYTE cr;

    if (OW_r_control(&cr, PN(owq))
		||
        OW_w_control((BYTE) ((cr & 0x0F) | ((((UINT) OWQ_I(owq)) & 0x0F) << 4)),
                      PN(owq)))
		return -EINVAL;
	return 0;
}

/* write flags */
static int FS_w_interval(struct one_wire_query * owq)
{
	BYTE cr;

    if (OW_r_control(&cr, PN(owq))
		||
        OW_w_control((BYTE) ((cr & 0x8F) | ((((UINT) OWQ_I(owq)) & 0x07) << 4)),
                      PN(owq)))
		return -EINVAL;
	return 0;
}

/* write flags */
static int FS_w_itime(struct one_wire_query * owq)
{
	BYTE cr;
    int I = OWQ_I(owq) ;

    if (OW_r_control(&cr, PN(owq)))
		return -EINVAL;

	if (I == 0) {
		cr &= 0x7F;				/* disable */
	} else if (I == 1) {
		cr = (cr & 0x8F) | 0x00;	/* set interval */
	} else if (I <= 4) {
		cr = (cr & 0x8F) | 0x10;	/* set interval */
	} else if (I <= 32) {
		cr = (cr & 0x8F) | 0x20;	/* set interval */
	} else if (I <= 64) {
		cr = (cr & 0x8F) | 0x30;	/* set interval */
	} else if (I <= 2048) {
		cr = (cr & 0x8F) | 0x40;	/* set interval */
	} else if (I <= 4096) {
		cr = (cr & 0x8F) | 0x50;	/* set interval */
	} else if (I <= 65536) {
		cr = (cr & 0x8F) | 0x60;	/* set interval */
	} else {
		cr = (cr & 0x8F) | 0x70;	/* set interval */
	}

    if (OW_w_control(cr, PN(owq)))
		return -EINVAL;
	return 0;
}

/* read flags */
int FS_r_flags(struct one_wire_query * owq)
{
	BYTE cr;
    if (OW_r_control(&cr, PN(owq)))
		return -EINVAL;
    OWQ_U(owq) = cr >> 4;
	return 0;
}

/* read flags */
int FS_r_interval(struct one_wire_query * owq)
{
	BYTE cr;
    if (OW_r_control(&cr, PN(owq)))
		return -EINVAL;
    OWQ_I(owq) = (cr >> 4) & 0x07;
	return 0;
}

/* read flags */
int FS_r_itime(struct one_wire_query * owq)
{
	BYTE cr;
    if (OW_r_control(&cr, PN(owq)))
		return -EINVAL;
    OWQ_I(owq) = itimes[(cr >> 4) & 0x07];
	return 0;
}

/* read running */
int FS_r_run(struct one_wire_query * owq)
{
	BYTE cr;
    if (OW_r_control(&cr, PN(owq)))
		return -EINVAL;
    OWQ_Y(owq) = ((cr & 0x08) != 0);
	return 0;
}

/* read running */
int FS_r_enable(struct one_wire_query * owq)
{
	BYTE cr;
    if (OW_r_control(&cr, PN(owq)))
		return -EINVAL;
    OWQ_Y(owq) = ((cr & 0x80) != 0);
	return 0;
}

/* read clock */
int FS_r_counter(struct one_wire_query * owq)
{
    int ret = FS_r_date(owq) ;
    OWQ_U(owq) = (UINT) OWQ_D(owq) ;
    return ret ;
}

/* read clock */
int FS_r_date(struct one_wire_query * owq)
{
    if (OW_r_clock(&OWQ_D(owq), PN(owq)))
		return -EINVAL;
	return 0;
}

/* 1904 clock-in-a-can */
static int OW_r_control(BYTE * cr, const struct parsedname *pn)
{
	BYTE r[] = { 0x66, };
	struct transaction_log t[] = {
		TRXN_START,
		{r, NULL, 1, trxn_match,},
		{NULL, cr, 1, trxn_read,},
		TRXN_END,
	};

	if (BUS_transaction(t, pn))
		return 1;

	return 0;
}

/* 1904 clock-in-a-can */
static int OW_r_clock(_DATE * d, const struct parsedname *pn)
{
	BYTE r[] = { 0x66, };
	BYTE data[5];
	struct transaction_log t[] = {
		TRXN_START,
		{r, NULL, 1, trxn_match,},
		{NULL, data, 5, trxn_read,},
		TRXN_END,
	};

	if (BUS_transaction(t, pn))
		return 1;

//    d[0] = (((((((UINT) data[4])<<8)|data[3])<<8)|data[2])<<8)|data[1] ;
	d[0] = UT_toDate(&data[1]);
	return 0;
}

static int OW_w_clock(const _DATE d, const struct parsedname *pn)
{
	BYTE r[] = { 0x66, };
	BYTE w[6] = { 0x99, };
	struct transaction_log tread[] = {
		TRXN_START,
		{r, NULL, 1, trxn_match,},
		{NULL, &w[1], 1, trxn_read,},
		TRXN_END,
	};
	struct transaction_log twrite[] = {
		TRXN_START,
		{w, NULL, 6, trxn_match,},
		TRXN_END,
	};

	/* read in existing control byte to preserve bits 4-7 */
	if (BUS_transaction(tread, pn))
		return 1;

	UT_fromDate(d, &w[2]);

	if (BUS_transaction(twrite, pn))
		return 1;
	return 0;
}

static int OW_w_control(const BYTE cr, const struct parsedname *pn)
{
	BYTE w[2] = { 0x99, cr, };
	struct transaction_log t[] = {
		TRXN_START,
		{w, NULL, 2, trxn_match,},
		TRXN_END,
	};

	/* read in existing control byte to preserve bits 4-7 */
	if (BUS_transaction(t, pn))
		return 1;

	return 0;
}
