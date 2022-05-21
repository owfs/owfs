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

GOOD_OR_BAD COMMON_readwrite_paged(struct one_wire_query *owq, size_t page, size_t pagelen, GOOD_OR_BAD (*readwritefunc) (BYTE *, size_t, off_t, struct parsedname *))
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
		RETURN_BAD_IF_BAD( readwritefunc(buffer_position, thispage, offset, pn) ) ;
		buffer_position += thispage;
		size -= thispage;
		offset += thispage;
	}

	return gbGOOD;
}


// The same as COMMON_readwrite_paged above except the results are nicely placed in the OWQ buffers with sizes and offsets.
GOOD_OR_BAD COMMON_OWQ_readwrite_paged(struct one_wire_query *owq, size_t page, size_t pagelen, GOOD_OR_BAD (*readwritefunc) (struct one_wire_query *, size_t, size_t))
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
			LEVEL_DEBUG("error at offset %ld", (long) offset);
			return gbBAD;
		}
		OWQ_buffer(owq_page) += thispage;
		size -= thispage;
		offset += thispage;
		// This is permitted (changing OWQ_offset and not restoring) since owq_page is a scratch var
		OWQ_offset(owq_page) = offset;
	}

	return gbGOOD;
}
