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

struct s_regex {
	regex_t * reg ;
} ;

static int reg_compare( const void * a, const void *b)
{
	const struct s_regex * pa = (const struct s_regex *) a ;
	const struct s_regex * pb = (const struct s_regex *) b ;

	if ( pa->reg < pb->reg ) {
		return -1 ;
	}
	if ( pa->reg > pb->reg ) {
		return 1 ;
	}
	return 0 ;
}

// Add a reg comp and return 1 or 0 if exists already
static int regcomp_test( regex_t * reg )
{
	struct s_regex * pnode = owmalloc( sizeof( struct s_regex ) ) ;
	struct s_regex * found ;
	void * result ;

	if ( pnode == NULL ) {
		LEVEL_DEBUG("memory exhuasted") ;
		return 0 ;
	}
	
	pnode->reg = reg ;
	result = tsearch( (void *) pnode, &regex_tree, reg_compare ) ;
	found = *(struct s_regex **) result ;
	if ( found == pnode ) {
		// new entry
		return 1 ;
	}
	// existing entry
	owfree( pnode ) ;
	return 0 ;
}


GOOD_OR_BAD ow_regcomp( regex_t * reg, const char * regex, int cflags )
{
	if ( regcomp_test( reg ) ) {
		int reg_res = regcomp( reg, regex, cflags ) ;
		if ( reg_res == 0 ) {
			LEVEL_DEBUG("Reg Ex expression <%s> compiled to %p\n",regex,reg) ;
			return gbGOOD ;
		} else {
			char e[101];
			regerror( reg_res, reg, e, 100 ) ;
			LEVEL_DEBUG("Problem compiling reg expression <%s>: %s",regex, e ) ;
			return gbBAD ;
		}
	}
	return gbGOOD ;
}

// Add a reg comp and return 1 or 0 if exists already
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

static void regexaction(const void *t, const VISIT which, const int depth)
{
	(void) depth;
	switch (which) {
	case leaf:
	case postorder:
		printf("Regex compiled pointer %p\n",(*(const struct s_regex * const *) t)->reg ) ;
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
		printf(">>>>>>>>> Free %p\n",(*(const struct s_regex * const *) t)->reg ) ;
		regfree((*(const struct s_regex * const *) t)->reg ) ;
	default:
		break;
	}
}

void ow_regdestroy( void )
{
	twalk( regex_tree, regexaction ) ;
	twalk( regex_tree, regexkillaction ) ;
	SAFETDESTROY( regex_tree, owfree_func ) ;
	printf(">>>>>>>>> regdestroy done\n") ;
	regex_tree = NULL ;
}
 
	
