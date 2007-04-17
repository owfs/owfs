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

struct aggregate A2409 = { 2, ag_numbers, ag_aggregate, };
struct filetype DS2409[] = {
	F_STANDARD,
  {"discharge",PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_stable, {v: NULL}, {o: FS_discharge}, {v:NULL},},
  {"clearevent",PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_stable, {v: NULL}, {o: FS_clearevent}, {v:NULL},},
  {"control",PROPERTY_LENGTH_UNSIGNED, NULL, ft_unsigned, fc_stable, {o: FS_r_control}, {o: FS_w_control}, {v:NULL},},
  {"sensed",PROPERTY_LENGTH_BITFIELD, &A2409, ft_bitfield, fc_volatile, {o: FS_r_sensed}, {v: NULL}, {v:NULL},},
  {"branch",PROPERTY_LENGTH_BITFIELD, &A2409, ft_bitfield, fc_volatile, {o: FS_r_branch}, {v: NULL}, {v:NULL},},
  {"event",PROPERTY_LENGTH_BITFIELD, &A2409, ft_bitfield, fc_volatile, {o: FS_r_event}, {v: NULL}, {v:NULL},},
  {"aux", 0, NULL, ft_directory, fc_volatile, {v: NULL}, {v: NULL}, {i:1},},
  {"main", 0, NULL, ft_directory, fc_volatile, {v: NULL}, {v: NULL}, {i:0},},
};

DeviceEntry(1F, DS2409);

/* ------- Functions ------------ */

/* DS2409 */
static int OW_r_control(BYTE * data, const struct parsedname *pn);

static int OW_discharge(const struct parsedname *pn);
static int OW_clearevent(const struct parsedname *pn);
static int OW_w_control(const UINT data, const struct parsedname *pn);

#define _1W_STATUS_READ_WRITE  0x5A
#define _1W_ALL_LINES_OFF      0x66
#define _1W_DISCHARGE_LINES    0x99
#define _1W_DIRECT_ON_MAIN     0xA5
#define _1W_SMART_ON_MAIN      0xCC
#define _1W_SMART_ON_AUX       0x33

/* discharge 2409 lines */
static int FS_discharge(struct one_wire_query * owq)
{
    if ((OWQ_Y(owq)) && OW_discharge(PN(owq)))
		return -EINVAL;
	return 0;
}

/* Clear 2409 event flags */
/* Added by Jan Kandziora */
static int FS_clearevent(struct one_wire_query * owq)
{
    if ((OWQ_Y(owq)) && OW_clearevent(PN(owq)))
		return -EINVAL;
	return 0;
}

/* 2409 switch -- branch pin voltage */
static int FS_r_sensed(struct one_wire_query * owq)
{
	BYTE data;
    if (OW_r_control(&data, PN(owq)))
		return -EINVAL;
//    y[0] = data&0x02 ? 1 : 0 ;
//    y[1] = data&0x08 ? 1 : 0 ;
    OWQ_U(owq) = ((data >> 1) & 0x01) | ((data >> 2) & 0x02);
	return 0;
}

/* 2409 switch -- branch status  -- note that bit value is reversed */
static int FS_r_branch(struct one_wire_query * owq)
{
	BYTE data;
    if (OW_r_control(&data, PN(owq)))
		return -EINVAL;
//    y[0] = data&0x01 ? 0 : 1 ;
//    y[1] = data&0x04 ? 0 : 1 ;
    OWQ_U(owq) = (((data) & 0x01) | ((data >> 1) & 0x02)) ^ 0x03;
	return 0;
}

/* 2409 switch -- event status */
static int FS_r_event(struct one_wire_query * owq)
{
	BYTE data;
    if (OW_r_control(&data, PN(owq)))
		return -EINVAL;
//    y[0] = data&0x10 ? 1 : 0 ;
//    y[1] = data&0x20 ? 1 : 0 ;
    OWQ_U(owq) = (data >> 4) & 0x03;
	return 0;
}

/* 2409 switch -- control pin state */
static int FS_r_control(struct one_wire_query * owq)
{
	BYTE data;
	UINT control[] = { 2, 3, 0, 1, };
    if (OW_r_control(&data, PN(owq)))
		return -EINVAL;
    OWQ_U(owq) = control[data >> 6];
	return 0;
}

/* 2409 switch -- control pin state */
static int FS_w_control(struct one_wire_query * owq)
{
    if (OWQ_U(owq) > 3)
		return -EINVAL;
    if (OW_w_control(OWQ_U(owq), PN(owq)))
		return -EINVAL;
	return 0;
}

/* Fix from Jan Kandziora for proper command code */
static int OW_discharge(const struct parsedname *pn)
{
    BYTE dis[] = { _1W_DISCHARGE_LINES, };
	struct transaction_log t[] = {
		TRXN_START,
		{dis, NULL, 1, trxn_match},
		TRXN_END,
	};

	// Could certainly couple this with next transaction
	BUSLOCK(pn);
	pn->in->buspath_bad = 1;
	BUSUNLOCK(pn);

	if (BUS_transaction(t, pn))
		return 1;

	UT_delay(100);

    dis[0] = _1W_ALL_LINES_OFF;
	if (BUS_transaction(t, pn))
		return 1;

	return 0;
}

/* Added by Jan Kandziora */
static int OW_clearevent(const struct parsedname *pn)
{
    BYTE clr[] = { _1W_ALL_LINES_OFF, };
	struct transaction_log t[] = {
		TRXN_START,
		{clr, NULL, 1, trxn_match},
		TRXN_END,
	};

	// Could certainly couple this with next transaction
	BUSLOCK(pn);
//  pn->in->buspath_bad = 1;
	BUSUNLOCK(pn);

	if (BUS_transaction(t, pn))
		return 1;

	return 0;
}

static int OW_w_control(const UINT data, const struct parsedname *pn)
{
	const BYTE d[] = { 0x20, 0xA0, 0x00, 0x40, };
    BYTE p[] = { _1W_STATUS_READ_WRITE, d[data], };
	const BYTE r[] = { 0x80, 0xC0, 0x00, 0x40, };
	BYTE info;
	struct transaction_log t[] = {
		TRXN_START,
		{p, NULL, 2, trxn_match},
		{NULL, &info, 1, trxn_read},
		TRXN_END,
	};

	if (BUS_transaction(t, pn))
		return 1;

	/* Check that Info corresponds */
	return (info & 0xC0) == r[data] ? 0 : 1;
}

static int OW_r_control(BYTE * data, const struct parsedname *pn)
{
    BYTE p[] = { _1W_STATUS_READ_WRITE, 0xFF, };
	struct transaction_log t[] = {
		TRXN_START,
		{p, NULL, 2, trxn_match},
		{NULL, data, 1, trxn_read},
		TRXN_END,
	};

	if (BUS_transaction(t, pn))
		return 1;
	return 0;
}
