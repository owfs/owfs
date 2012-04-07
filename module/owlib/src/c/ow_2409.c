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
#include "ow_2409.h"

/* ------- Prototypes ----------- */

/* DS2409 switch */
WRITE_FUNCTION(FS_discharge);
WRITE_FUNCTION(FS_clearevent);
READ_FUNCTION(FS_r_control);
WRITE_FUNCTION(FS_w_control);
READ_FUNCTION(FS_r_sensed);
READ_FUNCTION(FS_r_branch);
READ_FUNCTION(FS_r_event);

/* ------- Structures ----------- */

static struct aggregate A2409 = { 2, ag_numbers, ag_aggregate, };
static struct filetype DS2409[] = {
	F_STANDARD,
	{"discharge", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_stable, NO_READ_FUNCTION, FS_discharge, VISIBLE, NO_FILETYPE_DATA, },
	{"clearevent", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_stable, NO_READ_FUNCTION, FS_clearevent, VISIBLE, NO_FILETYPE_DATA, },
	{"control", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_stable, FS_r_control, FS_w_control, VISIBLE, NO_FILETYPE_DATA, },
	{"sensed", PROPERTY_LENGTH_BITFIELD, &A2409, ft_bitfield, fc_volatile, FS_r_sensed, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA, },
	{"branch", PROPERTY_LENGTH_BITFIELD, &A2409, ft_bitfield, fc_volatile, FS_r_branch, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA, },
	{"event", PROPERTY_LENGTH_BITFIELD, &A2409, ft_bitfield, fc_volatile, FS_r_event, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA, },
	{"aux", 0, NON_AGGREGATE, ft_directory, fc_volatile, NO_READ_FUNCTION, NO_WRITE_FUNCTION, VISIBLE, {i:eBranch_aux}, },
	{"main", 0, NON_AGGREGATE, ft_directory, fc_volatile, NO_READ_FUNCTION, NO_WRITE_FUNCTION, VISIBLE, {i:eBranch_main}, },
};

DeviceEntry(1F, DS2409, NO_GENERIC_READ, NO_GENERIC_WRITE);

/* ------- Functions ------------ */

/* DS2409 */
static GOOD_OR_BAD OW_r_control(BYTE * data, const struct parsedname *pn);

static GOOD_OR_BAD OW_discharge(const struct parsedname *pn);
static GOOD_OR_BAD OW_clearevent(const struct parsedname *pn);
static GOOD_OR_BAD OW_w_control(const UINT data, const struct parsedname *pn);

#define _1W_STATUS_READ_WRITE  0x5A
#define _1W_ALL_LINES_OFF      0x66
#define _1W_DISCHARGE_LINES    0x99
#define _1W_DIRECT_ON_MAIN     0xA5
#define _1W_SMART_ON_MAIN      0xCC
#define _1W_SMART_ON_AUX       0x33

/* discharge 2409 lines */
static ZERO_OR_ERROR FS_discharge(struct one_wire_query *owq)
{
	if ((OWQ_Y(owq) != 0) && BAD( OW_discharge(PN(owq)) ) ) {
		return -EINVAL;
	}
	return 0;
}

/* Clear 2409 event flags */
/* Added by Jan Kandziora */
static ZERO_OR_ERROR FS_clearevent(struct one_wire_query *owq)
{
	if ((OWQ_Y(owq)!=0) && BAD( OW_clearevent(PN(owq)) ) ) {
		return -EINVAL;
	}
	return 0;
}

/* 2409 switch -- branch pin voltage */
static ZERO_OR_ERROR FS_r_sensed(struct one_wire_query *owq)
{
	BYTE data = 0 ;

//    y[0] = data&0x02 ? 1 : 0 ;
//    y[1] = data&0x08 ? 1 : 0 ;
	OWQ_U(owq) = ((data >> 1) & 0x01) | ((data >> 2) & 0x02);

	return GB_to_Z_OR_E( OW_r_control(&data, PN(owq))) ;
}

/* 2409 switch -- branch status  -- note that bit value is reversed */
static ZERO_OR_ERROR FS_r_branch(struct one_wire_query *owq)
{
	BYTE data = 0 ;

//    y[0] = data&0x01 ? 0 : 1 ;
//    y[1] = data&0x04 ? 0 : 1 ;
	OWQ_U(owq) = (((data) & 0x01) | ((data >> 1) & 0x02)) ^ 0x03;

	return GB_to_Z_OR_E(OW_r_control(&data, PN(owq))) ;
}

/* 2409 switch -- event status */
static ZERO_OR_ERROR FS_r_event(struct one_wire_query *owq)
{
	BYTE data = 0 ;

//    y[0] = data&0x10 ? 1 : 0 ;
//    y[1] = data&0x20 ? 1 : 0 ;
	OWQ_U(owq) = (data >> 4) & 0x03;

	return GB_to_Z_OR_E(OW_r_control(&data, PN(owq))) ;
}

/* 2409 switch -- control pin state */
static ZERO_OR_ERROR FS_r_control(struct one_wire_query *owq)
{
	BYTE data = 0 ;
	UINT control[] = { 2, 3, 0, 1, };

	OWQ_U(owq) = control[data >> 6];

	return GB_to_Z_OR_E(OW_r_control(&data, PN(owq))) ;
}

/* 2409 switch -- control pin state */
static ZERO_OR_ERROR FS_w_control(struct one_wire_query *owq)
{
	if (OWQ_U(owq) > 3) {
		return -EINVAL;
	}
	return GB_to_Z_OR_E(OW_w_control(OWQ_U(owq), PN(owq))) ;
}

/* Fix from Jan Kandziora for proper command code */
static GOOD_OR_BAD OW_discharge(const struct parsedname *pn)
{
	BYTE dis[] = { _1W_DISCHARGE_LINES, };
	BYTE alo[] = { _1W_ALL_LINES_OFF, };
	GOOD_OR_BAD gbRet ;
	struct transaction_log t[] = {
		TRXN_START,
		TRXN_WRITE1(dis),
		TRXN_DELAY(100),
		TRXN_START,
		TRXN_WRITE1(alo),
		TRXN_END,
	};

	BUSLOCK(pn);
	pn->selected_connection->branch.branch = eBranch_bad ;
	gbRet = BUS_transaction_nolock(t, pn) ;
	BUSUNLOCK(pn);

	return gbRet ;
}

/* Added by Jan Kandziora */
static GOOD_OR_BAD OW_clearevent(const struct parsedname *pn)
{
	BYTE clr[] = { _1W_ALL_LINES_OFF, };
	struct transaction_log t[] = {
		TRXN_START,
		TRXN_WRITE1(clr),
		TRXN_END,
	};

	// Could certainly couple this with next transaction
	 return BUS_transaction(t, pn) ;
}

static GOOD_OR_BAD OW_w_control(const UINT data, const struct parsedname *pn)
{
	const BYTE d[] = { 0x20, 0xA0, 0x00, 0x40, };
	BYTE p[] = { _1W_STATUS_READ_WRITE, d[data], };
	const BYTE r[] = { 0x80, 0xC0, 0x00, 0x40, };
	BYTE info;
	struct transaction_log t[] = {
		TRXN_START,
		TRXN_WRITE2(p),
		TRXN_READ1(&info),
		TRXN_END,
	};

	RETURN_BAD_IF_BAD(BUS_transaction(t, pn)) ;

	/* Check that Info corresponds */
	return (info & 0xC0) == r[data] ? gbGOOD : gbBAD;
}

static GOOD_OR_BAD OW_r_control(BYTE * data, const struct parsedname *pn)
{
	BYTE p[] = { _1W_STATUS_READ_WRITE, 0xFF, };
	struct transaction_log t[] = {
		TRXN_START,
		TRXN_WRITE2(p),
		TRXN_READ1(data),
		TRXN_END,
	};

	return BUS_transaction(t, pn) ;
}
