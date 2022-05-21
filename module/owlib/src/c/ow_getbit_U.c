/*
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

// #define UT_getbit(buf, loc)  ( ( (buf)[(loc)>>3]>>((loc)&0x7) ) &0x01 )

/* Set and get a bit withing an unsigned integer */
/* Robin Gilks discovered that UT_getbit and UT_setbit are not 
 * endian-safe for setting bits in integers
 * so see UT_getbit_U and UT_getbit_U
 * */
 
int UT_getbit_U( UINT U, int loc)
{
	// get 'loc' bit from lsb
	return ( (U >> loc) & 0x01);
}

void UT_setbit_U( UINT * U, int loc, int bit)
{
	if (bit) {
		// set the bit (ignore what was there before)
		U[0] |= ( 1 << loc );
	} else if ( UT_getbit_U( U[0], loc ) ) {
		// need to clear the bit
		U[0] ^= ( 1 << loc );
	}
}
