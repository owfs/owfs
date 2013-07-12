/*
$Id$
    OWFS -- One-Wire filesystem
    OWHTTPD -- One-Wire Web Server
    Written 2003 Paul H Alfille
	email: paul.alfille@gmail.com
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

/* ------- Functions ------------ */

// BUS_verify tests if device is present in requested mode
//   serialnumber is 1-wire device address (64 bits)
//   return 0 good, 1 bad
GOOD_OR_BAD BUS_verify(BYTE search, const struct parsedname *pn)
{
	BYTE buffer[25];
	int i, goodbits = 0;

	// set all bits at first
	memset(buffer, 0xFF, 25);
	buffer[0] = search;

	// now set or clear apropriate bits for search
	for (i = 0; i < 64; i++) {
		UT_setbit(buffer, 3 * i + 10, UT_getbit(pn->sn, i));
	}

	// send/recieve the transfer buffer
	RETURN_BAD_IF_BAD(BUS_sendback_data(buffer, buffer, 25, pn) ) ;

	if (buffer[0] != search) {
		return gbBAD;
	}
	for (i = 0; (i < 64) && (goodbits < 64); i++) {
		switch (UT_getbit(buffer, 3 * i + 8) << 1 | UT_getbit(buffer, 3 * i + 9)) {
		case 0:
			break;
		case 1:
			if (!UT_getbit(pn->sn, i)) {
				goodbits++;
			}
			break;
		case 2:
			if (UT_getbit(pn->sn, i)){
				goodbits++;
			}
			break;
		case 3:				// No device on line
			return gbBAD;
		}
	}
	// check to see if there were enough good bits to be successful
	return ( goodbits < 8 ) ? gbBAD : gbGOOD;
}
