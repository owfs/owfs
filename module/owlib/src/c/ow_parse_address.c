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

static void Init_Address( struct address_pair * ap ) ;
static void Parse_Single_Address( struct address_entry * ae ) ;


static void Parse_Single_Address( struct address_entry * ae )
{
	int q1,q2,q3,q4 ;
	if ( ae->alpha == NULL  ) {
		// null entry
		ae->type = address_none ;
	} else if ( ae->alpha[0] == '\0' ) {
		// blank entry (actually no trim or whitespace)
		ae->type = address_none ;
	} else if ( strncasecmp( ae->alpha, "all", 3 ) == 0 ) {
		ae->type = address_all ;
	} else if ( strncasecmp( ae->alpha, "scan", 4 ) == 0 ) {
		ae->type = address_scan ;
	} else if ( strncasecmp( ae->alpha, "*", 1 ) == 0 ) {
		ae->type = address_asterix ;
	} else {
		switch( sscanf( ae->alpha, "%i.%i.%i.%i", &q1, &q2, &q3, &q4 ) ) {
		case 4:
			ae->type = address_dottedquad ;
			ae->number = q1 ;
			break ;
		case 3:
		case 2:
			ae->number = q1 ;
			ae->type = address_alpha ;
			break ;
		case 1:
			ae->type = address_numeric ;
			ae->number = q1 ;
			break ;
		case 0:
		default:
			ae->type = address_alpha ;
			break ;
		}
	}
}

/* Search for a ":" in the name
   Change it to a null,and parse the remaining text as either
   null, a number, or nothing
*/
void Parse_Address( char * address, struct address_pair * ap )
{
	char * colon ;
	char * colon2 ;

	// Set up address structure into previously allocated structure
	if ( ap == NULL ) {
		return ;
	}
	Init_Address(ap);

	// no entries
	if ( address == NULL) {
		ap->entries = 0 ;
		return ;
	}

	// copy the text string
	// All entries will point into this text copy
	ap->first.alpha = owstrdup(address) ;
	if ( ap->first.alpha == NULL ) {
		return ;
	}

	colon = strchr( ap->first.alpha, ':' ) ;

	if ( colon != NULL ) { // 1st colon exists
		// Colon exists, second entry, so add a null at colon to separate the string.
		*colon = '\0' ;
		// second part starts after colon position
		ap->second.alpha = colon + 1 ; // part of first.alpha
	}
	Parse_Single_Address( &(ap->first) ) ;

	if ( colon == NULL ) {
		// no colon, so only one entry
		ap->entries = 1 ;
		return ;
	}
	
	colon2 = strchr( ap->second.alpha, ':' ) ;

	if ( colon2 != NULL ) { // 2nd colon exists
		// Colon exists, third entry, so add a null at colon to separate the string.
		*colon2 = '\0' ;
		// third part starts after colon position
		ap->third.alpha = colon2 + 1 ; // still part of first.alpha
	}
	Parse_Single_Address( &(ap->second) ) ;

	if ( colon2 == NULL ) {
		// no colon2, so only two entries
		ap->entries = 2 ;
		return ;
	}
	
	// third part starts after colon2 position
	ap->entries = 3 ;
	Parse_Single_Address( &(ap->third) ) ;
}

void Free_Address( struct address_pair * ap )
{
	if ( ap == NULL ) {
		return ;
	}
	// Eliminates ap->second.alpha, too
	SAFEFREE( ap->first.alpha ) ;

	Init_Address( ap ) ;
}	
	
static void Init_Address( struct address_pair * ap )
{
	if ( ap == NULL ) {
		return ;
	}

	ap->first.alpha = NULL ;
	ap->first.type = address_none ;

	ap->second.alpha = NULL ;
	ap->second.type = address_none ;

	ap->third.alpha = NULL ;
	ap->third.type = address_none ;
}	
	
