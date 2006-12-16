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


int FS_locator(char *buf, size_t size, off_t offset,
	       const struct parsedname *pn)
{
    BYTE loc[8];
    ASCII ad[16];

    if (get_busmode(pn->in) == bus_fake) {
	if (pn->sn[7] & 0x01) {	// 50% chance of locator
	    loc[0] = 0xFE;
	    loc[7] = CRC8compute(loc, 7, 0);
	}
    } else {
	OW_locator(loc, pn);
    }
    bytes2string(ad, loc, 8);
    return FS_output_ascii(buf, size, offset, ad, 16);
}

int FS_r_locator(char *buf, size_t size, off_t offset,
		 const struct parsedname *pn)
{
    BYTE loc[8];
    ASCII ad[16];
    size_t i;

    if (get_busmode(pn->in) == bus_fake) {
	if (pn->sn[7] & 0x01) {	// 50% chance of locator
	    loc[0] = 0xFE;
	    loc[7] = CRC8compute(loc, 7, 0);
	}
    } else {
	OW_locator(loc, pn);
    }
    for (i = 0; i < 8; ++i)
	num2string(ad + (i << 1), loc[7 - i]);
    return FS_output_ascii(buf, size, offset, ad, 16);
}

static int OW_locator(BYTE * addr, const struct parsedname *pn)
{
    BYTE loc[10] = { 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, };	// key and 8 byte default
    struct transaction_log t[] = {
	TRXN_NVERIFY,
	{loc, loc, 10, trxn_read,},
	TRXN_END,
    };

    memset(addr, 0xFF, 8);
    if (BUS_transaction(t, pn))
	return 1;
    memcpy(addr, &loc[2], 8);
    return 0;
}
