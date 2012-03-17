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

/* ow_parse_external -- read the SENSOR and PROPERTY lines */

/* in general the lines are comma separated tokens
 * with backslash escaping and
 * quote and double quote matching
 * */

#include <config.h>
#include "owfs_config.h"
#include "ow.h"
 
// Look through text_string, ignore backslash and match quoting varibles
// allocates a new string with the token
static char * string_parse( char * text_string, char delim, char ** last_char )
{
	char * copy_string ;
	char * old_string_pointer = text_string ;
	char * new_string_pointer ;
		
	if ( text_string == NULL ) {
		*last_char = text_string ;
		return owstrdup("") ;
	}

	new_string_pointer = copy_string = owstrdup( text_string ) ; // more than enough room
	if ( copy_string == NULL ) {
		*last_char = text_string ;
		return NULL ;
	}
			
	while ( 1 ) {
		char current_char = old_string_pointer[0] ;
		new_string_pointer[0] = current_char ; // default copy
		if ( current_char == '\0' ) {
			// end of string
			*last_char = old_string_pointer ;
			return copy_string ;
		} else if ( current_char == '\\' ) {
			// backslash, use next char literally
			if ( old_string_pointer[1] ) {
				new_string_pointer[0] = old_string_pointer[1] ;
				++ new_string_pointer ;
				old_string_pointer += 2 ;
			} else {
				// unless there is no next char
				*last_char = old_string_pointer + 1 ;
				return copy_string ;
			}
		} else if ( current_char == delim ) {
			// found delimitter match
			// point to it, and null terminate
			*last_char = old_string_pointer ;
			new_string_pointer[1] = '\0' ;
			return copy_string ;
		} else if ( current_char == '"' || current_char == '\'' ) {
			// quotation -- find matching end
			char * quote_last_char ;
			char * quoted_string = string_parse( old_string_pointer+1, current_char, &quote_last_char ) ; 
			if ( quoted_string ) {
				strcpy( new_string_pointer+1, quoted_string ) ;
				new_string_pointer += strlen( quoted_string ) + 1 ;
				if ( quote_last_char[0] == current_char ) {
					old_string_pointer = quote_last_char + 1 ;
				} else {
					old_string_pointer = quote_last_char ;
				}
				owfree( quoted_string ) ;
			} else {
				new_string_pointer[1] = '\0' ;
				*last_char = old_string_pointer ;
				return copy_string ;
			}
		} else {
			++ old_string_pointer ;
			++ new_string_pointer ;
		}
	}
}
	
static char * trim_parse( char * raw_string )
{
	char * start_position ;
	char * end_position ;
	char * return_string ;
	for ( start_position = raw_string ; start_position[0] ; ++ start_position ) {
		if ( start_position[0] != ' ' ) {
			break ;
		}
	}
	return_string = owstrdup ( start_position ) ;
	owfree( raw_string ) ;
	
	for ( end_position = return_string + strlen( return_string ) ; end_position >= return_string ; -- end_position ) {
		if ( end_position[0] == '\0' ) {
			continue ;
		} else if ( end_position[0] != ' ' ) {
			break ;
		} else {
			end_position[0] = '\0' ;
		}
	}
	
	return return_string ;
}

static char * unquote_parse( raw_string )
{
	if ( raw_string == NULL ) {
		return NULL ;
	}
	switch ( raw_string[0] ) {		
	case '"':
	case '\'':
		if ( raw_string[1] == '\0' ) {
			owfree( raw_string ) ;
			return owstrdup("") ;
		} else {
			char * unquoted = owstrdup( raw_string+1 ) ;
			char * unquoted_end = unquoted + strlen(unquoted) -1 ;
			if ( unquoted_end[0] == raw_string[0] ) {
				unquoted_end[0] = '\0' ;
			}
			owfree( raw_string ) ;
			return unquoted ;
		}
		break
	default:
		return raw_string ;
	}
}	
	
