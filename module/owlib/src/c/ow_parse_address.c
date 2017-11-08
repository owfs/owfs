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
#include "ow_connection.h"

static void Init_Address( struct address_pair * ap ) ;
static void Parse_Single_Address( struct address_entry * ae ) ;

static regex_t rx_pa_none;
static regex_t rx_pa_all;
static regex_t rx_pa_scan;
static regex_t rx_pa_star;
static regex_t rx_pa_quad;
static regex_t rx_pa_num;
static regex_t rx_pa_one;
static regex_t rx_pa_two;
static regex_t rx_pa_three;

static pthread_once_t regex_init_once = PTHREAD_ONCE_INIT;

static void regex_init(void)
{
	ow_regcomp(&rx_pa_none, "^$", REG_NOSUB);
	ow_regcomp(&rx_pa_all, "^all$", REG_NOSUB | REG_ICASE);
	ow_regcomp(&rx_pa_scan, "^scan$", REG_NOSUB | REG_ICASE);
	ow_regcomp(&rx_pa_star, "^\\*$", REG_NOSUB);
	ow_regcomp(&rx_pa_quad, "^[[:digit:]]{1,3}\\.[[:digit:]]{1,3}\\.[[:digit:]]{1,3}\\.[[:digit:]]{1,3}$", REG_NOSUB);
	ow_regcomp(&rx_pa_num, "^-?[[:digit:]]+$", REG_NOSUB);
	ow_regcomp(&rx_pa_one, "^ *([^ ]+)[ \r]*$", 0);
	ow_regcomp(&rx_pa_two, "^ *([^ ]+) *: *([^ ]+)[ \r]*$", 0);
	ow_regcomp(&rx_pa_three, "^ *([^ ]+) *: *([^ ]+) *: *([^ ]+)[ \r]*$", 0);
}

static void Parse_Single_Address( struct address_entry * ae )
{
	pthread_once(&regex_init_once, regex_init);

	if ( ae->alpha == NULL  ) {
		// null entry
		ae->type = address_none ;
	} else if ( ow_regexec( &rx_pa_none, ae->alpha, NULL ) == 0 ) {
		ae->type = address_none ;
		LEVEL_DEBUG("None <%s>",ae->alpha);
	} else if ( ow_regexec( &rx_pa_all, ae->alpha, NULL ) == 0 ) {
		ae->type = address_all ;
		LEVEL_DEBUG("All <%s>",ae->alpha);
	} else if ( ow_regexec( &rx_pa_scan, ae->alpha, NULL ) == 0 ) {
		ae->type = address_scan ;
		LEVEL_DEBUG("Scan <%s>",ae->alpha);
	} else if ( ow_regexec( &rx_pa_star, ae->alpha, NULL ) == 0 ) {
		ae->type = address_asterix ;
		LEVEL_DEBUG("Star <%s>",ae->alpha);
	} else if ( ow_regexec( &rx_pa_quad, ae->alpha, NULL ) == 0 ) {
		ae->type = address_dottedquad ;
		LEVEL_DEBUG("IP <%s>",ae->alpha);
	} else if ( ow_regexec( &rx_pa_num, ae->alpha, NULL ) == 0 ) {
		ae->type = address_numeric ;
		ae->number = atoi(ae->alpha ) ;
		LEVEL_DEBUG("Num <%s> %d",ae->alpha,ae->number);
	} else {
		ae->type = address_alpha ;
		LEVEL_DEBUG("Text <%s>",ae->alpha);
	}
}

/* Search for a ":" in the name
   Change it to a null,and parse the remaining text as either
   null, a number, or nothing
*/
void Parse_Address( char * address, struct address_pair * ap )
{
	pthread_once(&regex_init_once, regex_init);

	struct ow_regmatch orm ;
	
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
	// Note that this is a maximum length, paring off spaces and colon will make room for extra \0
	ap->first.alpha = owstrdup(address) ;
	if ( ap->first.alpha == NULL ) {
		return ;
	}

	// test out the various matches
	orm.number = 3 ;
	if ( ow_regexec( &rx_pa_three, address, &orm ) == 0 ) {
		ap->entries = 3 ;
	} else {
		orm.number = 2 ;
		if ( ow_regexec( &rx_pa_two, address, &orm ) == 0 ) {
			ap->entries = 2 ;
		} else {
			orm.number = 1 ;
			if ( ow_regexec( &rx_pa_one, address, &orm ) == 0 ) {
				ap->entries = 1 ;
			} else {
				return ;
			}
		}
	}
			
	// Now copy and parse
	strcpy( ap->first.alpha, orm.match[1] ) ;
	Parse_Single_Address( &(ap->first) ) ;
	LEVEL_DEBUG("First <%s>",ap->first.alpha);

	if ( ap->entries > 1 ) {
		ap->second.alpha = ap->first.alpha + strlen(ap->first.alpha) + 1 ;
		strcpy( ap->second.alpha, orm.match[2] ) ;
		LEVEL_DEBUG("Second <%s>",ap->second.alpha);
		Parse_Single_Address( &(ap->second) ) ;

		if ( ap->entries > 2 ) {
			ap->third.alpha = ap->second.alpha + strlen(ap->second.alpha) + 1 ;
			strcpy( ap->third.alpha, orm.match[3] ) ;
			LEVEL_DEBUG("Third <%s>",ap->third.alpha);
			Parse_Single_Address( &(ap->third) ) ;
		}
	}
	
	ow_regexec_free( &orm ) ;	
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
	
