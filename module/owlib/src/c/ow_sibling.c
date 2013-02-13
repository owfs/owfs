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

ZERO_OR_ERROR FS_w_sibling_bitwork(UINT set, UINT mask, const char * sibling, struct one_wire_query *owq)
{
	ZERO_OR_ERROR write_error = -EINVAL ;
	struct one_wire_query * owq_sibling  = OWQ_create_sibling( sibling, owq ) ;
	if ( owq_sibling == NO_ONE_WIRE_QUERY ) {
		return -EINVAL ;
	}
	if ( FS_read_local(owq_sibling) == 0 ) {
//		UINT bitfield = OWQ_U(owq) ; //the original state does not come from owq, but from owq_sibling
		UINT bitfield = OWQ_U(owq_sibling) ;

		// clear mask:
		bitfield &= ~mask ;

		// set bits:
		bitfield |= (set & mask) ;

		OWQ_U(owq_sibling) = bitfield ;
		LEVEL_DEBUG("w sibling bit work  set=%04X  mask=%04X, sibling=%s, bitfield=%04X", set,mask, sibling,bitfield) ;

		write_error = FS_write_local(owq_sibling);
	}

	OWQ_destroy(owq_sibling) ;
	return write_error;
}

/* Delete entry in cache */
void FS_del_sibling(const char * sibling, struct one_wire_query *owq)
{
	struct one_wire_query * owq_sibling  = OWQ_create_sibling( sibling, owq ) ;

	if ( owq_sibling == NO_ONE_WIRE_QUERY ) {
		return ;
	}
	OWQ_Cache_Del(owq_sibling) ;
	OWQ_destroy(owq_sibling) ;
}
