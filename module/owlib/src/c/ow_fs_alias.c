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
#include "ow_standard.h"

/* ------- Prototypes ------------ */

/* ------- Functions ------------ */

ZERO_OR_ERROR FS_alias(struct one_wire_query *owq)
{
	struct parsedname *pn = PN(owq);
	ASCII * alias_name = Cache_Get_Alias( pn->sn ) ;

	if ( alias_name != NULL ) {
		ZERO_OR_ERROR zoe = OWQ_format_output_offset_and_size_z(alias_name, owq);
		LEVEL_DEBUG("Found alias %s for "SNformat"\n",alias_name,SNvar(pn->sn));
		owfree( alias_name ) ;
		return zoe;
	}

	LEVEL_DEBUG("Didn't find alias %s for "SNformat"\n",alias_name,SNvar(pn->sn));
	return OWQ_format_output_offset_and_size_z("", owq);
}

