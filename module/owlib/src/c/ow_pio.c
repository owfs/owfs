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

int FS_generic_r_pio(struct one_wire_query *owq)
{
	UINT piostate ;

	if ( FS_r_sibling_U( &piostate, "piostate", owq ) ) {
		return -EINVAL ;
	}

	OWQ_U(owq) = piostate ^ 0x0F ;

	return 0;
}

int FS_generic_r_sense(struct one_wire_query *owq)
{
	UINT piostate ;

	if ( FS_r_sibling_U( &piostate, "piostate", owq ) ) {
		return -EINVAL ;
	}

	// bits 0->0 and 2->1
	OWQ_U(owq) = piostate ;

	return 0;
}

static int FS_generic_w_pio(struct one_wire_query *owq)
{
	FS_del_sibling( "piostate", owq ) ;

	return 0 ;
}
