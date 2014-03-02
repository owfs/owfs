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

/* Set and get a bit from a binary buffer */
/* Robin Gilks discovered that this is not endian-safe for setting bits in integers
 * so see UT_getbit_U and UT_getbit_U
 * */
 
int UT_getbit(const BYTE * buf, int loc)
{
	// devide location by 8 to get byte
	return (((buf[loc >> 3]) >> (loc & 0x7)) & 0x01);
}

void UT_setbit(BYTE * buf, int loc, int bit)
{
	if (bit) {
		buf[loc >> 3] |= 0x01 << (loc & 0x7);
	} else {
		buf[loc >> 3] &= ~(0x01 << (loc & 0x7));
	}
}

int UT_get2bit(const BYTE * buf, int loc)
{
	return (((buf[loc >> 2]) >> ((loc & 0x3) << 1)) & 0x03);
}

void UT_set2bit(BYTE * buf, int loc, int bits)
{
	BYTE *p = &buf[loc >> 2];
	switch (loc & 3) {
	case 0:
		*p = (*p & 0xFC) | bits;
		return;
	case 1:
		*p = (*p & 0xF3) | (bits << 2);
		return;
	case 2:
		*p = (*p & 0xCF) | (bits << 4);
		return;
	case 3:
		*p = (*p & 0x3F) | (bits << 6);
		return;
	}
}
