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
#include "ow_standard.h"

/* ------- Prototypes ----------- */
static int OW_locator(BYTE * loc, const struct parsedname *pn);
static void OW_fake_locator(BYTE * loc, const struct parsedname *pn);
static void OW_any_locator(BYTE * loc, const struct parsedname *pn);
/* ------- Functions ------------ */


int FS_locator(struct one_wire_query *owq)
{
	BYTE loc[8];
	ASCII ad[16];

	OW_any_locator(loc, PN(owq) ) ;
	bytes2string(ad, loc, 8);
	return OWQ_parse_output_offset_and_size(ad, 16, owq);
}

// reversed address
int FS_r_locator(struct one_wire_query *owq)
{
	BYTE loc[8];
	ASCII ad[16];
	size_t i;

	OW_any_locator(loc, PN(owq) ) ;
	for (i = 0; i < 8; ++i) {
		num2string(ad + (i << 1), loc[7 - i]);
	}
	return OWQ_parse_output_offset_and_size(ad, 16, owq);
}

static void OW_any_locator(BYTE * loc, const struct parsedname *pn)
{	
	memset( loc, 0xFF, 8 ) ; // default if no locator
	switch (get_busmode(pn->selected_connection)) {
		case bus_fake:
		case bus_tester:
		case bus_mock:
			OW_fake_locator(loc, pn);
			break ;
		default:
			OW_locator(loc, pn);
			break ;
	}
}

static int OW_locator(BYTE * loc, const struct parsedname *pn)
{
	BYTE addr[10] = { 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, };	// key and 8 byte default
	struct transaction_log t[] = {
		TRXN_NVERIFY,
		TRXN_MODIFY(addr, addr, 10),
		TRXN_END,
	};
	
	if (BUS_transaction(t, pn)) {
		return 1;
	}
	memcpy(loc, &addr[2], 8);
	return 0;
}

static void OW_fake_locator(BYTE * loc, const struct parsedname *pn)
{
	if (pn->sn[7] & 0x01) {	// 50% chance of locator
		// start with 0xFE and use rest of sn (with valid CRC8)
		loc[0] = 0xFE;
		loc[1] = pn->sn[1] ;
		loc[2] = pn->sn[2] ;
		loc[3] = pn->sn[3] ;
		loc[4] = pn->sn[4] ;
		loc[5] = pn->sn[5] ;
		loc[6] = pn->sn[6] ;
		loc[7] = CRC8compute(loc, 7, 0);
	}
}
