/*
    OWFS -- One-Wire filesystem
    OWHTTPD -- One-Wire Web Server
    Written 2003 Paul H Alfille
    email: paul.alfille@gmail.com
    Released under the GPL
    See the header file: ow.h for full attribution
    1wire/iButton system from Dallas Semiconductor
*/

// regex

#include <config.h>
#include "owfs_config.h"
#include "ow.h"

/* Fill get serikal number from a character string */ 
enum parse_serialnumber Parse_SerialNumber(char *sn_char, BYTE * sn)
{
	ASCII ID[14];
	int i;

	memset( sn, 0, SERIAL_NUMBER_SIZE ) ;
	if ( sn_char == NULL ) {
		return sn_null ;
	}

	for (i = 0; i < 14; ++i, ++sn_char) {	/* get ID number */
		if (*sn_char == '.') { // ignore dots
			++sn_char;
		}
		if (isxdigit(*sn_char)) {
			ID[i] = *sn_char;
		} else {
			return sn_not_sn; // non-hex
		}
	}
	sn[0] = string2num(&ID[0]);
	sn[1] = string2num(&ID[2]);
	sn[2] = string2num(&ID[4]);
	sn[3] = string2num(&ID[6]);
	sn[4] = string2num(&ID[8]);
	sn[5] = string2num(&ID[10]);
	sn[6] = string2num(&ID[12]);
	sn[7] = CRC8compute(sn, SERIAL_NUMBER_SIZE-1, 0);
	if (*sn_char == '.') {
		++sn_char;
	}
	// Check CRC8 if given
	if (isxdigit(sn_char[0]) && isxdigit(sn_char[1])) {
		char crc[2];
		num2string(crc, sn[SERIAL_NUMBER_SIZE-1]);
		if (strncasecmp(crc, sn_char, 2)) {
			return sn_invalid;
		}
		sn_char += 2 ;
	}
	return ( *sn_char == '\0' ) ? sn_valid : sn_not_sn ;
}

// returns number of valid bytes in serial number
int SerialNumber_length(char *sn_char, BYTE * sn)
{
	int bytes;

	for ( bytes = 0 ; bytes < SERIAL_NUMBER_SIZE ; ++bytes ) {
		char ID[2] ;
		if (*sn_char == '.') { // ignore dots
			++sn_char;
		}
		if (isxdigit(*sn_char)) {
			ID[0] = *sn_char;
		} else {
			return bytes; // non-hex
		}
		++sn_char ;
		if (isxdigit(*sn_char)) {
			ID[1] = *sn_char;
		} else {
			return bytes; // non-hex
		}
		sn[bytes] = string2num(ID);
		++sn_char ;
	}
	return SERIAL_NUMBER_SIZE ;
}
	
