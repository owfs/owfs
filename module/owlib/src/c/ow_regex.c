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

GOOD_OR_BAD ow_regcomp( regex_t * preg, const char * regex, int cflags ) ;
