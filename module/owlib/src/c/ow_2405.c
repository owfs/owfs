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
#include "ow_2405.h"

/* ------- Prototypes ----------- */

/* DS2405 */
READ_FUNCTION(FS_r_sense);
READ_FUNCTION(FS_r_PIO);
WRITE_FUNCTION(FS_w_PIO);

/* ------- Structures ----------- */

struct filetype DS2405[] = {
	F_STANDARD,
	{"PIO", PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_stable, FS_r_PIO, FS_w_PIO, VISIBLE, NO_FILETYPE_DATA,},
	{"sensed", PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_volatile, FS_r_sense, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA,},
};

DeviceEntryExtended(05, DS2405, DEV_alarm);

/* ------- Functions ------------ */

static GOOD_OR_BAD OW_r_sense(int *val, const struct parsedname *pn);
static GOOD_OR_BAD OW_r_PIO(int *val, const struct parsedname *pn);
static GOOD_OR_BAD OW_w_PIO(int val, const struct parsedname *pn);

/* 2405 switch */
static ZERO_OR_ERROR FS_r_PIO(struct one_wire_query *owq)
{
	int num;
	if ( BAD( OW_r_PIO(&num, PN(owq)) ) ) {
		return -EINVAL;
	}
	OWQ_Y(owq) = (num != 0);
	return 0;
}

/* 2405 switch */
static ZERO_OR_ERROR FS_r_sense(struct one_wire_query *owq)
{
	int num;
	if ( BAD( OW_r_sense(&num, PN(owq)) ) ) {
		return -EINVAL;
	}
	OWQ_Y(owq) = (num != 0);
	return 0;
}

/* write 2405 switch */
static ZERO_OR_ERROR FS_w_PIO(struct one_wire_query *owq)
{
	if ( BAD( OW_w_PIO(OWQ_Y(owq), PN(owq)) ) ) {
		return -EINVAL;
	}
	return 0;
}

/* read the sense of the DS2405 switch */
static GOOD_OR_BAD OW_r_sense(int *val, const struct parsedname *pn)
{
	BYTE inp;
	struct transaction_log r[] = {
		TRXN_NVERIFY,
		TRXN_READ1(&inp),
		TRXN_END,
	};

	if (BUS_transaction(r, pn)) {
		return gbBAD;
	}

	val[0] = (inp != 0);
	return gbGOOD;
}

/* read the state of the DS2405 switch */
static GOOD_OR_BAD OW_r_PIO(int *val, const struct parsedname *pn)
{
	struct transaction_log a[] = {
		TRXN_AVERIFY,
		TRXN_END,
	};

	if (BUS_transaction(a, pn)) {
		struct transaction_log n[] = {
			TRXN_NVERIFY,
			TRXN_END,
		};
		if (BUS_transaction(n, pn)) {
			return gbBAD;
		}
		val[0] = 0;
	} else {
		val[0] = 1;
	}
	return gbGOOD;
}

/* write (set) the state of the DS2405 switch */
static GOOD_OR_BAD OW_w_PIO(const int val, const struct parsedname *pn)
{
	int current;

	if (OW_r_PIO(&current, pn)) {
		return 1;
	}

	if (current != val) {
		struct transaction_log n[] = {
			TRXN_START,
			TRXN_END,
		};
		if (BUS_transaction(n, pn)) {
			return gbBAD;
		}
	}
	//printf("2405write current=%d new=%d\n",current,val) ;
	return gbGOOD;
}
