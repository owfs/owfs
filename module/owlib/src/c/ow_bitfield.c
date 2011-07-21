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

// struct bitfield { "alias_link", number_of_bits, shift_left, }

ZERO_OR_ERROR FS_r_bitfield(struct one_wire_query *owq)
{
	struct  bitfield * bf = PN(owq)->selected_filetype->data.v ;
	UINT mask = ( 0x01 << bf->size ) - 1 ;
	UINT val ;

	RETURN_ERROR_IF_BAD( FS_r_sibling_U( &val, bf->link, owq ) ) ;

	OWQ_U(owq) = (val >> bf->shift ) & mask ;
	return 0 ;
}

ZERO_OR_ERROR FS_w_bitfield(struct one_wire_query *owq)
{
	struct  bitfield * bf = PN(owq)->selected_filetype->data.v ;
	UINT mask = ( 0x01 << bf->size ) - 1 ;
	UINT val ;

	// read it in
	RETURN_ERROR_IF_BAD( FS_r_sibling_U( &val, bf->link, owq ) ) ;

	// clear the bits
	val &= ~(mask << bf->shift) ;

	// Add in the new value
	val |= (OWQ_U(owq) & mask) << bf->shift ;

	// write it out
	return FS_w_sibling_U( val, bf->link, owq ) ;
}
