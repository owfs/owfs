/*
$Id$
    OWFS -- One-Wire filesystem
    OWHTTPD -- One-Wire Web Server
    Written 2003 Paul H Alfille
	email: paul.alfille@gmail.com
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

/* Offset manipulation
 * Performs a shift of the offset to do an operation,
 * then returns the offset to the initial value.
 * The reason is that some reads or writes are done in another place in memory,
 * but OWQ_parse_output checks the file size against the original property constraints.
 */

ZERO_OR_ERROR COMMON_offset_process( ZERO_OR_ERROR (*func) (struct one_wire_query *), struct one_wire_query * owq, off_t shift_offset)
{
	ZERO_OR_ERROR func_return_value ;

	OWQ_offset(owq) += shift_offset ;
	func_return_value = func(owq) ;
	OWQ_offset(owq) -= shift_offset ;

	return func_return_value ;
}
