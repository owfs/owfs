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
READ_FUNCTION(FS_r_enable);
WRITE_FUNCTION(FS_w_enable);
READ_FUNCTION(FS_r_interval);
WRITE_FUNCTION(FS_w_interval);
READ_FUNCTION(FS_r_itime);
WRITE_FUNCTION(FS_w_itime);
READ_FUNCTION(FS_r_control);
WRITE_FUNCTION(FS_w_control);
READ_FUNCTION(FS_r_user);
WRITE_FUNCTION(FS_w_user);

/* ------- Structures ----------- */

struct aggregate A2415 = { 4, ag_numbers, ag_aggregate, };
struct filetype DS2415[] = {
	F_STANDARD,
  {"ControlRegister", PROPERTY_LENGTH_HIDDEN, NULL, ft_unsigned, fc_stable, FS_r_control, FS_w_control, {v:NULL}, },
  {"user", PROPERTY_LENGTH_UNSIGNED, &A2415, ft_bitfield, fc_link, FS_r_user, FS_w_user, {v:NULL},},
  {"running", PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_link, FS_r_run, FS_w_run, {v:NULL},},
  {"udate", PROPERTY_LENGTH_UNSIGNED, NULL, ft_unsigned, fc_link, FS_r_counter, FS_w_counter, {v:NULL},},
  {"date", PROPERTY_LENGTH_DATE, NULL, ft_date, fc_second, FS_r_date, FS_w_date, {v:NULL},},
};

DeviceEntry(24, DS2415);

struct filetype DS2417[] = {
	F_STANDARD,
  {"ControlRegister", PROPERTY_LENGTH_HIDDEN, NULL, ft_unsigned, fc_stable, FS_r_control, FS_w_control, {v:NULL}, },
  {"enable", PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_link, FS_r_enable, FS_w_enable, {v:NULL},},
  {"interval", PROPERTY_LENGTH_INTEGER, NULL, ft_integer, fc_link, FS_r_interval, FS_w_interval, {v:NULL},},
  {"itime", PROPERTY_LENGTH_INTEGER, NULL, ft_integer, fc_link, FS_r_itime, FS_w_itime, {v:NULL},},
  {"running", PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_link, FS_r_run, FS_w_run, {v:NULL},},
  {"udate", PROPERTY_LENGTH_UNSIGNED, NULL, ft_unsigned, fc_link, FS_r_counter, FS_w_counter, {v:NULL},},
  {"date", PROPERTY_LENGTH_DATE, NULL, ft_date, fc_second, FS_r_date, FS_w_date, {v:NULL},},
};

DeviceEntry(27, DS2417);

static int itimes[] = { 1, 4, 32, 64, 2048, 4096, 65536, 131072, };

#define _1W_READ_CLOCK 0x66
#define _1W_WRITE_CLOCK 0x99

#define _MASK_DS2415_USER 0xF0
#define _MASK_DS2415_OSC 0x0C
#define _MASK_DS2417_IE 0x80
#define _MASK_DS2417_IS 0x70

/* ------- Functions ------------ */
/* DS2415/DS1904 Digital clock in a can */

/* DS2415 & DS2417 */
static int OW_r_clock(_DATE * d, const struct parsedname *pn);
static int OW_r_control(BYTE * cr, const struct parsedname *pn);
static int OW_w_clock(const _DATE d, struct one_wire_query *owq);
static int OW_w_control(const BYTE cr, const struct parsedname *pn);

/* DS2417 interval */
static int FS_r_interval(struct one_wire_query *owq)
{
	UINT U ;

	if ( FS_r_sibling_U( &U, "ControlRegister", owq ) ) {
		return -EINVAL ;
	}

	OWQ_U(owq) = ( U & _MASK_DS2417_IS ) >> 4 ;

	return 0 ;
}

static int FS_w_interval(struct one_wire_query *owq)
{
	UINT U = ( OWQ_U(owq) << 4 ) & _MASK_DS2417_IS ; // Move to upper nibble

	return FS_w_sibling_bitwork( U, _MASK_DS2417_IS, "ControlRegister", owq ) ;
}

/* DS2415 User bits */
static int FS_r_user(struct one_wire_query *owq)
{
	UINT U ;

	if ( FS_r_sibling_U( &U, "ControlRegister", owq ) ) {
		return -EINVAL ;
	}

	OWQ_U(owq) = ( U & _MASK_DS2415_USER ) >> 4 ;

	return 0 ;
}

static int FS_w_user(struct one_wire_query *owq)
{
	UINT U = ( OWQ_U(owq) << 4 ) & _MASK_DS2415_USER ; // Move to upper nibble

	return FS_w_sibling_bitwork( U, _MASK_DS2415_USER, "ControlRegister", owq ) ;
}

/* DS2415-DS2417 Oscillator control */
static int FS_r_run(struct one_wire_query *owq)
{
	UINT U ;

	if ( FS_r_sibling_U( &U, "ControlRegister", owq ) ) {
		return -EINVAL ;
	}

	OWQ_Y(owq) = ( ( U & _MASK_DS2415_OSC ) != 0 ) ;

	return 0 ;
}

static int FS_w_run(struct one_wire_query *owq)
{
	UINT U = OWQ_Y(owq) ? _MASK_DS2415_OSC : 0 ;

	return FS_w_sibling_bitwork( U, _MASK_DS2415_OSC, "ControlRegister", owq ) ;
}


/* DS2417 interrupt enable */
static int FS_r_enable(struct one_wire_query *owq)
{
	UINT U ;

	if ( FS_r_sibling_U( &U, "ControlRegister", owq ) ) {
		return -EINVAL ;
	}

	OWQ_Y(owq) = ( ( U & _MASK_DS2417_IE ) != 0 ) ;

	return 0 ;
}

static int FS_w_enable(struct one_wire_query *owq)
{
	UINT U = OWQ_Y(owq) ? _MASK_DS2417_IE : 0 ;

	return FS_w_sibling_bitwork( U, _MASK_DS2417_IE, "ControlRegister", owq ) ;
}

/* DS1915 Control Register */
static int FS_r_control(struct one_wire_query *owq)
{
	BYTE cr ;
	if ( OW_r_control( &cr, PN(owq) ) ) {
		return -EINVAL ;
	}

	OWQ_U(owq) = cr ;

	return 0 ;
}

static int FS_w_control(struct one_wire_query *owq)
{
	return OW_w_control( OWQ_U(owq) & 0xFF, PN(owq) ) ? -EINVAL : 0 ;
}

/* DS2415 - DS2417 couter verions of date */
int FS_r_counter(struct one_wire_query *owq)
{
	_DATE D ;

	if ( FS_r_sibling_D( &D, "date", owq ) ) {
		return -EINVAL ;
	}

	OWQ_U(owq) = (UINT) D ;
	return 0;
}

static int FS_w_counter(struct one_wire_query *owq)
{
	_DATE D = (_DATE) OWQ_D(owq);

	return FS_w_sibling_D( D, "date", owq ) ;
}

/* DS2415 - DS2417 clock */
int FS_r_date(struct one_wire_query *owq)
{
	if (OW_r_clock(&OWQ_D(owq), PN(owq))) {
		return -EINVAL;
	}
	return 0;
}

static int FS_w_date(struct one_wire_query *owq)
{
	if (OW_w_clock(OWQ_D(owq), owq)) {
		return -EINVAL;
	}
	return 0;
}

/* DS2417 interval time (in seconds) */
static int FS_w_itime(struct one_wire_query *owq)
{
	UINT IS ;
	int I = OWQ_I(owq);

	if (I == 0) {
		/* Special case -- 0 time equivalent to disable */
		return FS_w_sibling_Y( 0, "enable", owq ) ;
	} else if (I == 1) {
		IS = 0 ; // 1 sec
	} else if (I <= 4) {
		IS = 1 ; // 4 sec
	} else if (I <= 32) {
		IS = 2 ; // 32 sec
	} else if (I <= 64) {
		IS = 3 ; // 64 sec ~1 min
	} else if (I <= 2048) {
		IS = 4 ; // 2048 sec ~30 min
	} else if (I <= 4096) {
		IS = 5 ; // 4096 sec ~1 hr
	} else if (I <= 65536) {
		IS = 6 ; // 65536 sec ~18 hr
	} else {
		IS = 7 ; // 131072 sec ~36 hr
	}

	/* Set interval */
	if ( FS_w_sibling_U( IS, "interval", owq ) ) {
		return -EINVAL ;
	}

	/* Enable as well */
	return FS_w_sibling_Y( 1, "enable", owq) ;
}

int FS_r_itime(struct one_wire_query *owq)
{
	UINT interval;
	if ( FS_r_sibling_U( &interval, "interval", owq) ) {
		return -EINVAL;
	}
	OWQ_I(owq) = itimes[interval];
	return 0;
}

/* 1904 clock-in-a-can */
static int OW_r_control(BYTE * cr, const struct parsedname *pn)
{
	BYTE r[1] = { _1W_READ_CLOCK, };
	struct transaction_log t[] = {
		TRXN_START,
		TRXN_WRITE1(r),
		TRXN_READ1(cr),
		TRXN_END,
	};

	if (BUS_transaction(t, pn)) {
		return 1;
	}

	return 0;
}

/* 1904 clock-in-a-can */
static int OW_r_clock(_DATE * d, const struct parsedname *pn)
{
	BYTE r[1] = { _1W_READ_CLOCK, };
	BYTE data[5];
	struct transaction_log t[] = {
		TRXN_START,
		TRXN_WRITE1(r),
		TRXN_READ(data, 5),
		TRXN_END,
	};

	if (BUS_transaction(t, pn)) {
		return 1;
	}

//    d[0] = (((((((UINT) data[4])<<8)|data[3])<<8)|data[2])<<8)|data[1] ;
	d[0] = UT_toDate(&data[1]);
	return 0;
}

static int OW_w_clock(const _DATE d, struct one_wire_query *owq)
{
	UINT cr ;
	BYTE w[6] = { _1W_WRITE_CLOCK, };
	struct transaction_log twrite[] = {
		TRXN_START,
		TRXN_WRITE(w, 6),
		TRXN_END,
	};

	/* read in existing control byte to preserve bits 4-7 */
	if ( FS_r_sibling_U( &cr, "ControlRegister", owq) ) {
		return 1;
	}

	w[1] = cr & 0xFF ;
	UT_fromDate(d, &w[2]);

	if (BUS_transaction(twrite, PN(owq))) {
		return 1;
	}
	return 0;
}

static int OW_w_control(const BYTE cr, const struct parsedname *pn)
{
	BYTE w[2] = { _1W_WRITE_CLOCK, cr, };
	struct transaction_log t[] = {
		TRXN_START,
		TRXN_WRITE2(w),
		TRXN_END,
	};

	/* read in existing control byte to preserve bits 4-7 */
	if (BUS_transaction(t, pn)) {
		return 1;
	}

	return 0;
}
