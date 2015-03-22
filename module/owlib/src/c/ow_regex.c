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

// tree to hold pointers to compiled regex expressions to cache compilation and free
void * regex_tree = NULL ;

enum e_regcomp_test { e_regcomp_exists, e_regcomp_new, e_regcomp_error, } ;

struct s_regex {
	regex_t * reg ;
} ;

static int reg_compare( const void * a, const void *b)
{
	const struct s_regex * pa = (const struct s_regex *) a ;
	const struct s_regex * pb = (const struct s_regex *) b ;

	return (int) (pa->reg - pb->reg) ;
}

// Add a reg comp to tree if doesn't already exist
static enum e_regcomp_test regcomp_test( regex_t * reg )
{
	struct s_regex * pnode = owmalloc( sizeof( struct s_regex ) ) ;
	struct s_regex * found ;
	void * result ;

	if ( pnode == NULL ) {
		LEVEL_DEBUG("memory exhuasted") ;
		return e_regcomp_error ;
	}
	
	pnode->reg = reg ;
	result = tsearch( (void *) pnode, &regex_tree, reg_compare ) ;
	found = *(struct s_regex **) result ;
	if ( found == pnode ) {
		// new entry
		return e_regcomp_new ;
	}
	// existing entry
	owfree( pnode ) ;
	return e_regcomp_exists ;
}

// Compile a regex
// Actually only compile if needed, check first if it's already
// cached in the regex tree
void ow_regcomp( regex_t * reg, const char * regex, int cflags )
{
	switch ( regcomp_test( reg ) ) {
		case e_regcomp_error:
			return ;
		case e_regcomp_exists:
			return ;
		case e_regcomp_new:
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
	}
}
// free a regex node (cached compiled action)
void ow_regfree( regex_t * reg )
{
	struct s_regex node = { reg, } ;
	void * result = tdelete( (void *) (&node), &regex_tree, reg_compare ) ;

	if ( result != NULL ) {
		regfree( reg ) ;
	} else {
		LEVEL_DEBUG( "attempt to free an uncompiled regex" ) ;
	}
}

// Debugging to show tree contents
static void regexaction(const void *t, const VISIT which, const int depth)
{
	(void) depth;
	switch (which) {
	case leaf:
	case postorder:
		LEVEL_DEBUG("Regex compiled pointer %p",(*(const struct s_regex * const *) t)->reg ) ;
	default:
		break;
	}
}

static void regexkillaction(const void *t, const VISIT which, const int depth)
{
	(void) depth;
	switch (which) {
	case leaf:
	case postorder:
		LEVEL_DEBUG("Regex Free %p",(*(const struct s_regex * const *) t)->reg ) ;
		regfree((*(const struct s_regex * const *) t)->reg ) ;
	default:
		break;
	}
}

void ow_regdestroy( void )
{
	// twalk( regex_tree, regexaction ) ;
	twalk( regex_tree, regexkillaction ) ;
	SAFETDESTROY( regex_tree, owfree_func ) ;
	LEVEL_DEBUG("Regex destroy done") ;
	regex_tree = NULL ;
}

// wrapper for regcomp
// pmatch rm_so abd rm_eo handled internally
// allocates (with owcalloc) and fills match_strings
// Can be nmatch==0 and matched_strings==NULL for just a test with no return  
int ow_regexec( const regex_t * rex, const char * string, struct ow_regmatch * orm )
{
	if ( orm == NULL ) {
		if ( regexec( rex, string, 0, NULL, 0 ) != 0 ) {
			return -1 ;
		}
		return 0 ;
	} else {
		int i ;
		int number = orm->number ;
		
		regmatch_t pmatch[ number + 2 ] ;
		if ( regexec( rex, string, number+1, pmatch, 0 ) != 0 ) {
			LEVEL_DEBUG("Not found");
			return -1 ;
		}
		orm->matches = owcalloc( sizeof( char * ) , number+1 ) ;
		if ( orm->matches == NULL ) {
			LEVEL_DEBUG("Memory allocation error");
			return -1 ;
		}
		
		for ( i=0 ; i <= number ; ++i ) {
			orm->matches[i] = NULL ;
		}

		for ( i=0 ; i <= number ; ++i ) {
			int s = pmatch[i].rm_so ;
			int e = pmatch[i].rm_eo ;
			if ( s != -1 && e != -1 ) {
				int l = e - s   ;
				orm->matches[i] = owmalloc( l+1 ) ;
				if ( orm->matches[i] == NULL ) {
					LEVEL_DEBUG("Memory problem") ;
					ow_regexec_free( orm )  ;
					return -1 ;
				}
				memcpy( orm->matches[i], &string[s], l ) ;
				orm->matches[i][l] = '\0' ;
				LEVEL_DEBUG("%d: %d->%d found <%s>",i,s,e,orm->matches[i]) ;
			}
		}
		return 0 ;
	}
}		

void ow_regexec_free( struct ow_regmatch * orm )
{
	if ( orm != NULL ) {
		int i ;
		for ( i = 0 ; i <= orm->number ; ++ i ) {
			if ( orm->matches[i] != NULL ) {
				owfree( orm->matches[i] ) ;
			}
		}
		owfree( orm->matches ) ;
	}
}
