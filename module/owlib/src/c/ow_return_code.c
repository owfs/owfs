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

char * return_code_strings[N_RETURN_CODES] {
	"SUCCESS", // e_rc_success (first entry)
	"Not initialized",
	"Unknown error", // e_rc_last (last entry)
} ;

