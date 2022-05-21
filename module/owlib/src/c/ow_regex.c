/*
    OWFS -- One-Wire filesystem
    OWHTTPD -- One-Wire Web Server
    Written 2003 Paul H Alfille
	email: paul.alfille@gmail.com
	Released under the GPL
	See the header file: ow.h for full attribution
	1wire/iButton system from Dallas Semiconductor
*/


/* ow_interate.c */
/* routines to split reads and writes if longer than page */

#include <config.h>
#include "owfs_config.h"
#include "ow.h"

// Compile a regex
void ow_regcomp( regex_t * reg, const char * regex, int cflags )
{
	int reg_res = regcomp( reg, regex, cflags | REG_EXTENDED ) ;
	if ( reg_res == 0 ) {
		LEVEL_DEBUG("Reg Ex expression <%s> compiled to %p",regex,reg) ;
	} else {
		char e[101];
		regerror( reg_res, reg, e, 100 ) ;
		LEVEL_DEBUG("Problem compiling reg expression <%s>: %s",regex, e ) ;
	}
}

// wrapper for regcomp
// pmatch rm_so abd rm_eo handled internally
// allocates (with owcalloc) and fills match_strings
// Can be nmatch==0 and matched_strings==NULL for just a test with no return  
int ow_regexec( const regex_t * rex, const char * string, struct ow_regmatch * orm )
{
	if ( orm == NULL ) {
		// special case -- no matches saved
		if ( regexec( rex, string, 0, NULL, 0 ) != 0 ) {
			return -1 ;
		}
		return 0 ;
	} else {
		// case with saved matches
		int i ;
		int number = orm->number ;
		int len = strlen( string ) ;
		
		regmatch_t pmatch[ number + 2 ] ;
		// try the regexec on the string 
		if ( regexec( rex, string, number+1, pmatch, 0 ) != 0 ) {
			LEVEL_DEBUG("Not found");
			return -1 ;
		}
		
		// allocate space for the array of matches -- pointer array first
		orm->pre = owcalloc( sizeof( char * ) , 3*(number+1) ) ;
		if ( orm->pre == NULL ) {
			LEVEL_DEBUG("Memory allocation error");
			return -1 ;
		}
		orm->match = orm->pre + number+1 ;
		orm->post = orm->match + number+1 ;
		
		for ( i=0 ; i < number+1 ; ++i ) {
			// Note last index is kept null
			orm->pre[i] = NULL ;
			orm->match[i] = NULL ;
			orm->post[i] = NULL ;
		}

		// not actual string array -- allocated as a buffer with space for pre,match and post
		// only need to allocat once per matched number
		for ( i=0 ; i < number+1 ; ++i ) {
			int s = pmatch[i].rm_so ;
			int e = pmatch[i].rm_eo ;
			if ( s != -1 && e != -1 ) {
				int l = e - s   ;
				// each buffer is only slightly longer than original string (since contains string plus some EOS nulls
				orm->pre[i] = owmalloc( len + 3 ) ;
				if ( orm->pre[i] == NULL ) {
					LEVEL_DEBUG("Memory problem") ;
					ow_regexec_free( orm )  ;
					return -1 ;
				}
				// pre at start
				memset( orm->pre[i], 0 , len+3 ) ; 

				memcpy( orm->pre[i], string, s ) ;

				// match next
				orm->match[i] = orm->pre[i] + s + 1 ;
				memcpy( orm->match[i], &string[s], l ) ;

				// then post
				orm->post[i] = orm->match[i] + l + 1 ;
				memcpy( orm->post[i], &string[e], len-e+1 ) ;

				LEVEL_DEBUG("%d: %d->%d found <%s><%s><%s>",i,s,e,orm->pre[i],orm->match[i],orm->post[i]) ;
			}
		}
		return 0 ;
	}
}		

void ow_regexec_free( struct ow_regmatch * orm )
{
	if ( orm != NULL ) {
		int i ;
		for ( i = 0 ; i < orm->number + 1 ; ++ i ) {
			if ( orm->pre[i] != NULL ) {
				owfree( orm->pre[i] ) ;
			}
		}
		owfree( orm->pre ) ;
	}
}
