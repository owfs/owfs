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

/* General Device File format:
    This device file corresponds to a specific 1wire/iButton chip type
	( or a closely related family of chips )

	The connection to the larger program is through the "device" data structure,
	  which must be declared in the acompanying header file.

	The device structure holds the
	  family code,
	  name,
	  device type (chip, interface or pseudo)
	  number of properties,
	  list of property structures, called "filetype".

	Each filetype structure holds the
	  name,
	  estimated length (in bytes),
	  aggregate structure pointer,
	  data format,
	  read function,
	  write funtion,
	  generic data pointer

	The aggregate structure, is present for properties that several members
	(e.g. pages of memory or entries in a temperature log. It holds:
	  number of elements
	  whether the members are lettered or numbered
	  whether the elements are stored together and split, or separately and joined
*/

#include <config.h>
#include "owfs_config.h"
#include "ow.h"

ZERO_OR_ERROR FS_r_sibling_binary(char * data, size_t * size, const char * sibling, struct one_wire_query *owq)
{
	struct one_wire_query * owq_sibling  = OWQ_create_sibling( sibling, owq ) ;
	SIZE_OR_ERROR sib_status ;

	if ( owq_sibling == NULL ) {
		return -EINVAL ;
	}
	
	if ( GOOD( OWQ_allocate_read_buffer(owq_sibling)) ) {
		OWQ_offset(owq_sibling) = 0 ;
		sib_status = FS_read_local(owq_sibling) ;
		if ( (sib_status >= 0) && (OWQ_length(owq_sibling) <= size[0]) ) {
			memset(data, 0, size[0] ) ;
			size[0] = OWQ_length(owq_sibling) ;
			memcpy( data, OWQ_buffer(owq_sibling), size[0] ) ;
			sib_status = 0 ;
		} else {
			sib_status = -ENOMEM ;
		}
	} else {
		sib_status = -ENOMEM ;
	}
	OWQ_destroy(owq_sibling) ;
	return sib_status ;
}

ZERO_OR_ERROR FS_w_sibling_binary(char * data, size_t size, off_t offset, const char * sibling, struct one_wire_query *owq)
{
	struct one_wire_query * owq_sibling  = OWQ_create_sibling( sibling, owq ) ;
	SIZE_OR_ERROR write_error ;

	if ( owq_sibling == NULL ) {
		return -EINVAL ;
	}
	
	OWQ_assign_write_buffer(data, size, offset, owq_sibling) ;
	write_error = FS_write_local(owq_sibling) ;
	OWQ_destroy(owq_sibling) ;
	return write_error ;
}
