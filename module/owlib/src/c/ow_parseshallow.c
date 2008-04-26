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

/* make a "shallow" copy -- just an element size */
void OWQ_create_shallow_single(struct one_wire_query *owq_shallow, struct one_wire_query *owq_original)
{
    memcpy(owq_shallow, owq_original, sizeof(struct one_wire_query));
}

/* make a "shallow" copy -- bitfield */
void OWQ_create_shallow_bitfield(struct one_wire_query *owq_shallow, struct one_wire_query *owq_original)
{
    memcpy(owq_shallow, owq_original, sizeof(struct one_wire_query));
    PN(owq_shallow)->extension = EXTENSION_BYTE ;
}

/* make a "shallow" copy -- but possibly full array size or just an element size */
/* if full array size, allocate extra space for value_object and possible buffer */
int OWQ_create_shallow_aggregate(struct one_wire_query *owq_shallow, struct one_wire_query *owq_original)
{
	struct parsedname *pn = PN(owq_original);

	memcpy(owq_shallow, owq_original, sizeof(struct one_wire_query));

	/* allocate new value_object array space */
	OWQ_array(owq_shallow) = calloc((size_t) pn->selected_filetype->ag->elements, sizeof(union value_object));
    if (OWQ_array(owq_shallow) == NULL) {
		return -ENOMEM;
    }
	if (pn->extension == EXTENSION_ALL) {
		memcpy(OWQ_array(owq_shallow), OWQ_array(owq_original), pn->selected_filetype->ag->elements * sizeof(union value_object));
	}

	OWQ_offset(owq_shallow) = 0;
	OWQ_pn(owq_shallow).extension = EXTENSION_ALL;

	switch (pn->selected_filetype->format) {
	case ft_binary:
	case ft_ascii:
	case ft_vascii:
		/* allocate larger buffer space */
		OWQ_size(owq_shallow) = FullFileLength(PN(owq_shallow));
		OWQ_buffer(owq_shallow) = malloc(OWQ_size(owq_shallow));
		if (OWQ_buffer(owq_shallow) == NULL) {
			free(OWQ_array(owq_shallow));
			return -ENOMEM;
		}
		break;
	default:
		break;
	}

	return 0;
}

void OWQ_destroy_shallow_aggregate(struct one_wire_query *owq_shallow)
{
	if (OWQ_array(owq_shallow)) {
		free(OWQ_array(owq_shallow));
		OWQ_array(owq_shallow) = NULL;
	}
	switch (OWQ_pn(owq_shallow).selected_filetype->format) {
	case ft_binary:
	case ft_ascii:
	case ft_vascii:
		free(OWQ_buffer(owq_shallow));
		break;
	default:
		break;
	}
}

void OWQ_create_temporary(struct one_wire_query *owq_temporary, char *buffer, size_t size, off_t offset, struct parsedname *pn)
{
	OWQ_buffer(owq_temporary) = buffer;
	OWQ_size(owq_temporary) = size;
	OWQ_offset(owq_temporary) = offset;
	memcpy(PN(owq_temporary), pn, sizeof(struct parsedname));
}
