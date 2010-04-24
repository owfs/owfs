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

/* The parallel adapter has never worked for us, let's not pretend */

ZERO_OR_ERROR DS1410_detect(struct connection_in *in)
{
	(void) in;
	return 0;
}
