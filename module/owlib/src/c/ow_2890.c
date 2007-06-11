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
#include "ow_2890.h"

/* ------- Prototypes ----------- */

/* DS2890 Digital Potentiometer */
READ_FUNCTION(FS_r_cp);
WRITE_FUNCTION(FS_w_cp);
READ_FUNCTION(FS_r_wiper);
WRITE_FUNCTION(FS_w_wiper);

/* ------- Structures ----------- */

struct filetype DS2890[] = {
	F_STANDARD,
  {"chargepump",PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_stable,   FS_r_cp, FS_w_cp, {v:NULL},} ,
  {"wiper",PROPERTY_LENGTH_UNSIGNED, NULL, ft_unsigned, fc_stable,   FS_r_wiper, FS_w_wiper, {v:NULL},} ,
};

DeviceEntryExtended(2C, DS2890, DEV_alarm | DEV_resume | DEV_ovdr);

#define _1W_WRITE_POSITION 0x0F
#define _1W_READ_POSITION 0xF0
#define _1W_INCREMENT 0xC3
#define _1W_DECREMENT 0x99
#define _1W_WRITE_CONTROL_REGISTER 0x55
#define _1W_READ_CONTROL_REGISTER 0xAA
#define _1W_RELEASE_WIPER 0x96

/* ------- Functions ------------ */

/* DS2890 */
static int OW_r_wiper(UINT * val, const struct parsedname *pn);
static int OW_w_wiper(const UINT val, const struct parsedname *pn);
static int OW_r_cp(int *val, const struct parsedname *pn);
static int OW_w_cp(const int val, const struct parsedname *pn);

/* Wiper */
static int FS_w_wiper(struct one_wire_query * owq)
{
    UINT num = OWQ_U(owq);
	if (num > 255)
		num = 255;

    if (OW_w_wiper(num, PN(owq)))
		return -EINVAL;
	return 0;
}

/* write Charge Pump */
static int FS_w_cp(struct one_wire_query * owq)
{
    if (OW_w_cp(OWQ_Y(owq), PN(owq)))
		return -EINVAL;
	return 0;
}

/* read Wiper */
static int FS_r_wiper(struct one_wire_query * owq)
{
    if (OW_r_wiper(&OWQ_U(owq), PN(owq)))
		return -EINVAL;
	return 0;
}

/* Charge Pump */
static int FS_r_cp(struct one_wire_query * owq)
{
    if (OW_r_cp(&OWQ_Y(owq), PN(owq)))
		return -EINVAL;
	return 0;
}

/* write Wiper */
static int OW_w_wiper(const UINT val, const struct parsedname *pn)
{
	BYTE resp[1];
	BYTE cmd[] = { _1W_WRITE_POSITION, (BYTE) val, };
	BYTE ns[] = { _1W_RELEASE_WIPER, };
	struct transaction_log t[] = {
		TRXN_START,
        TRXN_WRITE2(cmd),
        TRXN_READ1(resp),
        TRXN_WRITE1(ns),
        TRXN_COMPARE(resp,&cmd[1],1),
		TRXN_END
	};

	return BUS_transaction(t, pn);
}

/* read Wiper */
static int OW_r_wiper(UINT * val, const struct parsedname *pn)
{
	BYTE fo[] = { _1W_READ_POSITION, };
	BYTE resp[2];
	struct transaction_log t[] = {
		TRXN_START,
        TRXN_WRITE1(fo),
        TRXN_READ2(resp),
		TRXN_END
	};

	if (BUS_transaction(t, pn))
		return 1;

	*val = resp[1];
	return 0;
}

/* write Charge Pump */
static int OW_w_cp(const int val, const struct parsedname *pn)
{
	BYTE resp[1];
	BYTE cmd[] = { _1W_WRITE_CONTROL_REGISTER, (val) ? 0x4C : 0x0C };
	BYTE ns[] = { _1W_RELEASE_WIPER, };
	struct transaction_log t[] = {
		TRXN_START,
        TRXN_WRITE2(cmd),
        TRXN_READ1(resp),
        TRXN_WRITE1(ns),
        TRXN_COMPARE(resp,&cmd[1],1),
		TRXN_END
	};

	return BUS_transaction(t, pn) ;
}

/* read Charge Pump */
static int OW_r_cp(int *val, const struct parsedname *pn)
{
	BYTE aa[] = { _1W_READ_CONTROL_REGISTER, };
	BYTE resp[2];
	struct transaction_log t[] = {
		TRXN_START,
        TRXN_WRITE1(aa),
        TRXN_READ2(resp) ,
		TRXN_END
	};

	if (BUS_transaction(t, pn))
		return 1;

	*val = resp[1] & 0x40;
	return 0;
}
