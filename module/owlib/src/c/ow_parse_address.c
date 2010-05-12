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

static ADDRESS_OR_FLAG Parse_Single_Address( char * address )
{
	ADDRESS_OR_FLAG num ;
	if ( strncasecmp( address, "all", 3 ) == 0 ) {
		return ADDRESS_ALL ;
	}
	if ( isalpha(address[0]) || ispunct(address[0]) ) {
		return ADDRESS_NOTNUM ;
	}
	if ( sscanf( address, "%i", &num ) == 1 ) {
		return num ;
	}
	return ADDRESS_NONE ;
}

/* Search for a ":" in the name
   Change it to a null,and parse the remaining text as either
   null, a number, or nothing
*/
void Parse_Address( char * address, struct address_pair * ap )
{
	char * colon ;

	// no entries
	if ( address == NULL) {
		ap->entries = 0 ;
		ap->first = ADDRESS_NONE ;
		ap->second = ADDRESS_NONE ;
		return ;
	}

	colon = strchr( address, ':' ) ;
	if ( colon == NULL ) { // not found
		ap->entries = 1 ;
		ap->first = Parse_Single_Address(address) ;
		ap->second = ADDRESS_NONE ;
		return ;
	}

	ap->entries = 2 ;
	ap->first = Parse_Single_Address(address) ;
	ap->second = Parse_Single_Address(colon+1) ;
}

