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

static void Init_Address( struct address_pair * ap ) ;
static void Parse_Single_Address( struct address_entry * ae ) ;


static void Parse_Single_Address( struct address_entry * ae )
{
	int q1,q2,q3,q4 ;
	if ( ae->alpha == NULL  ) {
		ae->type = address_none ;
	} else if ( strncasecmp( ae->alpha, "all", 3 ) == 0 ) {
		ae->type = address_all ;
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
	ap->first.alpha = owstrdup(address) ;
	if ( ap->first.alpha == NULL ) {
		return ;
	}
	colon = strchr( ap->first.alpha, ':' ) ;

	if ( colon == NULL ) { // not found
		// no colon, so only one entry
		ap->entries = 1 ;
		Parse_Single_Address( &(ap->first) ) ;
		return ;
	}
	
	// Colon exists, second entry, so add a null at colon to separate the string.
	*colon = '\0' ;
	Parse_Single_Address( &(ap->first) ) ;

	// second part starts after colon position
	ap->entries = 2 ;
	ap->second.alpha = colon + 1 ; // part of first.alpha
	Parse_Single_Address( &(ap->second) ) ;
}

void Free_Address( struct address_pair * ap )
{
	if ( ap == NULL ) {
		return ;
	}
	if ( ap->first.alpha ) {
		owfree(ap->first.alpha) ;
		// Eliminates ap->second.alpha, too
		ap->first.alpha = NULL ;
	}
	ap->first.type = address_none ;
	ap->second.type = address_none ;
}	
	
static void Init_Address( struct address_pair * ap )
{
	if ( ap == NULL ) {
		return ;
	}
	ap->first.alpha = NULL ;
	ap->second.alpha = NULL ;
	ap->first.type = address_none ;
	ap->second.type = address_none ;
}	
	
