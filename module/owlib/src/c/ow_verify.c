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

/* ------- Functions ------------ */

// BUS_verify tests if device is present in requested mode
//   serialnumber is 1-wire device address (64 bits)
//   return 0 good, 1 bad
int BUS_verify(BYTE search, const struct parsedname *pn)
{
	BYTE buffer[25];
	int i, goodbits = 0;

	// set all bits at first
	memset(buffer, 0xFF, 25);
	buffer[0] = search;

	// now set or clear apropriate bits for search
	for (i = 0; i < 64; i++)
		UT_setbit(buffer, 3 * i + 10, UT_getbit(pn->sn, i));

	// send/recieve the transfer buffer
	if (BUS_sendback_data(buffer, buffer, 25, pn))
		return 1;
	if (buffer[0] != search)
		return 1;
	for (i = 0; (i < 64) && (goodbits < 64); i++) {
		switch (UT_getbit(buffer, 3 * i + 8) << 1 | UT_getbit(buffer, 3 * i + 9)) {
		case 0:
			break;
		case 1:
			if (!UT_getbit(pn->sn, i))
				goodbits++;
			break;
		case 2:
			if (UT_getbit(pn->sn, i))
				goodbits++;
			break;
		case 3:				// No device on line
			return 1;
		}
	}
	// check to see if there were enough good bits to be successful
	// remember 1 is bad!
	return goodbits < 8;
}
