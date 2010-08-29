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
#include "ow_counters.h"
#include "ow_connection.h"

void OWQ_create_temporary(struct one_wire_query *owq_temporary, char *buffer, size_t size, off_t offset, struct parsedname *pn)
{
	OWQ_buffer(owq_temporary) = buffer;
	OWQ_size(owq_temporary) = size;
	OWQ_offset(owq_temporary) = offset;
	memcpy(PN(owq_temporary), pn, sizeof(struct parsedname));
}
