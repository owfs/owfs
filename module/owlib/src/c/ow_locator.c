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

#include <config.h>
#include "owfs_config.h"
#include "ow.h"
#include "ow_connection.h"
#include "ow_xxxx.h"

/* ------- Prototypes ----------- */
static int OW_locator(BYTE * addr, const struct parsedname *pn);
/* ------- Functions ------------ */


int FS_locator(struct one_wire_query *owq)
{
	struct parsedname *pn = PN(owq);
	BYTE loc[8];
	ASCII ad[16];

	switch (get_busmode(pn->selected_connection)) {
		case bus_fake:
		case bus_tester:
		case bus_mock:
			if (pn->sn[7] & 0x01) {	// 50% chance of locator
				loc[0] = 0xFE;
				loc[7] = CRC8compute(loc, 7, 0);
			}
			break ;
		default:
			OW_locator(loc, pn);
	}
	bytes2string(ad, loc, 8);
	return Fowq_output_offset_and_size(ad, 16, owq);
}

int FS_r_locator(struct one_wire_query *owq)
{
	struct parsedname *pn = PN(owq);
	BYTE loc[8];
	ASCII ad[16];
	size_t i;

	if (get_busmode(pn->selected_connection) == bus_fake) {
		if (pn->sn[7] & 0x01) {	// 50% chance of locator
			loc[0] = 0xFE;
			loc[7] = CRC8compute(loc, 7, 0);
		}
	} else {
		OW_locator(loc, pn);
	}
	for (i = 0; i < 8; ++i) {
		num2string(ad + (i << 1), loc[7 - i]);
	}
	return Fowq_output_offset_and_size(ad, 16, owq);
}

static int OW_locator(BYTE * addr, const struct parsedname *pn)
{
	BYTE loc[10] = { 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, };	// key and 8 byte default
	struct transaction_log t[] = {
		TRXN_NVERIFY,
		TRXN_MODIFY(loc, loc, 10),
		TRXN_END,
	};

	memset(addr, 0xFF, 8);
	if (BUS_transaction(t, pn))
		return 1;
	memcpy(addr, &loc[2], 8);
	return 0;
}
