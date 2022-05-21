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

static regex_t rx_sn_parse;

static void regex_fini(void)
{
	regfree(&rx_sn_parse);
}

static pthread_once_t regex_init_once = PTHREAD_ONCE_INIT;

static void regex_init(void)
{
	ow_regcomp(&rx_sn_parse, "^([[:xdigit:]]{2})\\.?([[:xdigit:]]{12})\\.?([[:xdigit:]]{2}){0,1}$", 0);

	atexit(regex_fini);
}

/* Fill get serikal number from a character string */ 
enum parse_serialnumber Parse_SerialNumber(char *sn_char, BYTE * sn)
{
	pthread_once(&regex_init_once,regex_init);

	struct ow_regmatch orm ;
	orm.number = 3 ;
	
	if ( sn_char == NULL ) {
		return sn_null ;
	}

	if ( ow_regexec( &rx_sn_parse, sn_char, &orm ) != 0 ) {
		return sn_not_sn ;
	}
		
	sn[0] = string2num(orm.match[1]);
	sn[1] = string2num(&orm.match[2][0]);
	sn[2] = string2num(&orm.match[2][2]);
	sn[3] = string2num(&orm.match[2][4]);
	sn[4] = string2num(&orm.match[2][6]);
	sn[5] = string2num(&orm.match[2][8]);
	sn[6] = string2num(&orm.match[2][10]);
	sn[7] = CRC8compute(sn, SERIAL_NUMBER_SIZE-1, 0);
	if (orm.match[3] != NULL) {
		// CRC given
		if ( string2num(orm.match[3]) != sn[7] ) {
			ow_regexec_free( &orm ) ;
			return sn_invalid;
		}
	}
	ow_regexec_free( &orm ) ;
	return sn_valid ;
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
	
