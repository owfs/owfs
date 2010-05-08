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
	{"ControlRegister", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_stable, FS_r_control, FS_w_control, INVISIBLE, NO_FILETYPE_DATA, },
	{"user", PROPERTY_LENGTH_UNSIGNED, &A2415, ft_bitfield, fc_link, FS_r_user, FS_w_user, VISIBLE, NO_FILETYPE_DATA,},
	{"running", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_link, FS_r_run, FS_w_run, VISIBLE, NO_FILETYPE_DATA,},
	{"udate", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_second, FS_r_counter, FS_w_counter, VISIBLE, NO_FILETYPE_DATA,},
	{"date", PROPERTY_LENGTH_DATE, NON_AGGREGATE, ft_date, fc_link, COMMON_r_date, COMMON_w_date, VISIBLE, NO_FILETYPE_DATA,},
};

DeviceEntry(24, DS2415);

struct filetype DS2417[] = {
	F_STANDARD,
	{"ControlRegister", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_stable, FS_r_control, FS_w_control, INVISIBLE, NO_FILETYPE_DATA, },
	{"enable", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_link, FS_r_enable, FS_w_enable, VISIBLE, NO_FILETYPE_DATA,},
	{"interval", PROPERTY_LENGTH_INTEGER, NON_AGGREGATE, ft_integer, fc_link, FS_r_interval, FS_w_interval, VISIBLE, NO_FILETYPE_DATA,},
	{"itime", PROPERTY_LENGTH_INTEGER, NON_AGGREGATE, ft_integer, fc_link, FS_r_itime, FS_w_itime, VISIBLE, NO_FILETYPE_DATA,},
	{"running", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_link, FS_r_run, FS_w_run, VISIBLE, NO_FILETYPE_DATA,},
	{"udate", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_second, FS_r_counter, FS_w_counter, VISIBLE, NO_FILETYPE_DATA,},
	{"date", PROPERTY_LENGTH_DATE, NON_AGGREGATE, ft_date, fc_link, COMMON_r_date, COMMON_w_date, VISIBLE, NO_FILETYPE_DATA,},
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
static GOOD_OR_BAD OW_r_udate(UINT * U, const struct parsedname *pn);
static GOOD_OR_BAD OW_r_control(BYTE * cr, const struct parsedname *pn);
static GOOD_OR_BAD OW_w_udate(UINT control_reg, UINT U, const struct parsedname * pn);
static GOOD_OR_BAD OW_w_control(const BYTE cr, const struct parsedname *pn);

/* DS2417 interval */
static ZERO_OR_ERROR FS_r_interval(struct one_wire_query *owq)
{
	UINT U = 0 ;
	ZERO_OR_ERROR z_or_e = FS_r_sibling_U( &U, "ControlRegister", owq ) ;

	OWQ_U(owq) = ( U & _MASK_DS2417_IS ) >> 4 ;
	return z_or_e ;
}

static ZERO_OR_ERROR FS_w_interval(struct one_wire_query *owq)
{
	UINT U = ( OWQ_U(owq) << 4 ) & _MASK_DS2417_IS ; // Move to upper nibble

	return FS_w_sibling_bitwork( U, _MASK_DS2417_IS, "ControlRegister", owq ) ;
}

/* DS2415 User bits */
static ZERO_OR_ERROR FS_r_user(struct one_wire_query *owq)
{
	UINT U = 0 ;
	ZERO_OR_ERROR z_or_e = FS_r_sibling_U( &U, "ControlRegister", owq ) ;

	OWQ_U(owq) = ( U & _MASK_DS2415_USER ) >> 4 ;
	return z_or_e ;
}

static ZERO_OR_ERROR FS_w_user(struct one_wire_query *owq)
{
	UINT U = ( OWQ_U(owq) << 4 ) & _MASK_DS2415_USER ; // Move to upper nibble

	return FS_w_sibling_bitwork( U, _MASK_DS2415_USER, "ControlRegister", owq ) ;
}

/* DS2415-DS2417 Oscillator control */
static ZERO_OR_ERROR FS_r_run(struct one_wire_query *owq)
{
	UINT U = 0;
	ZERO_OR_ERROR z_or_e = FS_r_sibling_U( &U, "ControlRegister", owq ) ;

	OWQ_Y(owq) = ( ( U & _MASK_DS2415_OSC ) != 0 ) ;
	return z_or_e ;
}

static ZERO_OR_ERROR FS_w_run(struct one_wire_query *owq)
{
	UINT U = OWQ_Y(owq) ? _MASK_DS2415_OSC : 0 ;

	return FS_w_sibling_bitwork( U, _MASK_DS2415_OSC, "ControlRegister", owq ) ;
}


/* DS2417 interrupt enable */
static ZERO_OR_ERROR FS_r_enable(struct one_wire_query *owq)
{
	UINT U = 0 ;
	ZERO_OR_ERROR z_or_e = FS_r_sibling_U( &U, "ControlRegister", owq ) ;

	OWQ_Y(owq) = ( ( U & _MASK_DS2417_IE ) != 0 ) ;
	return z_or_e ;
}

static ZERO_OR_ERROR FS_w_enable(struct one_wire_query *owq)
{
	UINT U = OWQ_Y(owq) ? _MASK_DS2417_IE : 0 ;

	return FS_w_sibling_bitwork( U, _MASK_DS2417_IE, "ControlRegister", owq ) ;
}

/* DS1915 Control Register */
static ZERO_OR_ERROR FS_r_control(struct one_wire_query *owq)
{
	BYTE cr ;
	if ( OW_r_control( &cr, PN(owq) ) ) {
		return -EINVAL ;
	}

	OWQ_U(owq) = cr ;

	return 0 ;
}

static ZERO_OR_ERROR FS_w_control(struct one_wire_query *owq)
{
	return OW_w_control( OWQ_U(owq) & 0xFF, PN(owq) ) ? -EINVAL : 0 ;
}

/* DS2415 - DS2417 couter verions of date */
static ZERO_OR_ERROR FS_r_counter(struct one_wire_query *owq)
{
	return GB_to_Z_OR_E( OW_r_udate( &(OWQ_U(owq)), PN(owq) ) ) ; 
}

static ZERO_OR_ERROR FS_w_counter(struct one_wire_query *owq)
{
	UINT control_reg ; // included in write

	/* read in existing control byte to preserve bits 4-7 */
	if ( FS_r_sibling_U( &control_reg, "ControlRegister", owq) != 0 ) {
		return gbBAD;
	}

	return GB_to_Z_OR_E( OW_w_udate( control_reg, OWQ_U(owq), PN(owq) ) ) ;
}

/* DS2417 interval time (in seconds) */
static ZERO_OR_ERROR FS_w_itime(struct one_wire_query *owq)
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
	if ( FS_w_sibling_U( IS, "interval", owq ) != 0 ) {
		return -EINVAL ;
	}

	/* Enable as well */
	return FS_w_sibling_Y( 1, "enable", owq) ;
}

static ZERO_OR_ERROR FS_r_itime(struct one_wire_query *owq)
{
	UINT interval = 0;
	ZERO_OR_ERROR z_or_e = FS_r_sibling_U( &interval, "interval", owq) ;

	OWQ_I(owq) = itimes[interval];
	return z_or_e ;
}

/* 1904 clock-in-a-can */
static GOOD_OR_BAD OW_r_control(BYTE * cr, const struct parsedname *pn)
{
	BYTE r[1] = { _1W_READ_CLOCK, };
	struct transaction_log t[] = {
		TRXN_START,
		TRXN_WRITE1(r),
		TRXN_READ1(cr),
		TRXN_END,
	};

	 return BUS_transaction(t, pn) ;
}

/* 1904 clock-in-a-can */
static GOOD_OR_BAD OW_r_udate(UINT * U, const struct parsedname *pn)
{
	BYTE r[1] = { _1W_READ_CLOCK, };
	BYTE data[5];
	struct transaction_log t[] = {
		TRXN_START,
		TRXN_WRITE1(r),
		TRXN_READ(data, 5),
		TRXN_END,
	};

	RETURN_BAD_IF_BAD(BUS_transaction(t, pn)) ;

	U[0] = UT_int32(&data[1]);
	return gbGOOD;
}

static GOOD_OR_BAD OW_w_udate(UINT control_reg, UINT U, const struct parsedname *pn)
{
	BYTE w[6] = { _1W_WRITE_CLOCK, };
	struct transaction_log twrite[] = {
		TRXN_START,
		TRXN_WRITE(w, 6),
		TRXN_END,
	};

	w[1] = control_reg & 0xFF ;
	UT_uint32_to_bytes( U, &w[2] );
	return BUS_transaction(twrite, pn) ;
}

static GOOD_OR_BAD OW_w_control(const BYTE cr, const struct parsedname *pn)
{
	BYTE w[2] = { _1W_WRITE_CLOCK, cr, };
	struct transaction_log t[] = {
		TRXN_START,
		TRXN_WRITE2(w),
		TRXN_END,
	};

	/* read in existing control byte to preserve bits 4-7 */
	return BUS_transaction(t, pn) ;
}
