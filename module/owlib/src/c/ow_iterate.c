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

/* ow_interate.c */
/* routines to split reads and writes if longer than page */

#include <config.h>
#include "owfs_config.h"
#include "ow.h"

int OW_readwrite_paged(struct one_wire_query *owq, size_t page, size_t pagelen, int (*readwritefunc) (BYTE *, size_t, off_t, struct parsedname *))
{
	size_t size = OWQ_size(owq);
	off_t offset = OWQ_offset(owq) + pagelen * page;
	BYTE *buffer_position = (BYTE *) OWQ_buffer(owq);
	struct parsedname *pn = PN(owq);

	/* successive pages, will start at page start */
	OWQ_length(owq) = size;
    while (size > 0) {
		size_t thispage = pagelen - (offset % pagelen);
		if (thispage > size) {
			thispage = size;
		}
        if (readwritefunc(buffer_position, thispage, offset, pn)) {
			return 1;
		}
		buffer_position += thispage;
		size -= thispage;
		offset += thispage;
	}

	return 0;
}

int OWQ_readwrite_paged(struct one_wire_query *owq, size_t page, size_t pagelen, int (*readwritefunc) (struct one_wire_query *, size_t, size_t))
{
	size_t size = OWQ_size(owq);
	off_t offset = OWQ_offset(owq) + pagelen * page;
	struct parsedname *pn = PN(owq);
	OWQ_allocate_struct_and_pointer(owq_page);

	/* holds a pointer to a position in owq's buffer */
	OWQ_create_temporary(owq_page, OWQ_buffer(owq), size, offset, pn);

	/* successive pages, will start at page start */
	OWQ_length(owq) = size;
	while (size > 0) {
		size_t thispage = pagelen - (offset % pagelen);
		if (thispage > size) {
			thispage = size;
		}
		OWQ_size(owq_page) = thispage;
		if (readwritefunc(owq_page, 0, pagelen)) {
			LEVEL_DEBUG("error at offset %ld\n", (long) offset);
			return 1;
		}
		OWQ_buffer(owq_page) += thispage;
		size -= thispage;
		offset += thispage;
		OWQ_offset(owq_page) = offset;
	}

	return 0;
}
