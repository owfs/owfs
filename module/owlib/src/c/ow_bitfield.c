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

/* Bitfield
 * Change individual bits in a variable
 * the variable is the alias_link
 * the bitfield size is number_of_bits
 * and the position from the lowest is shift_left
 * */

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

/* Bit Array
 * single bits interspersed
 * alias_link is the base value
 * number_of_bits is the number of arrays
 * shift_left is the starting bit
 * */

ZERO_OR_ERROR FS_r_bit_array(struct one_wire_query *owq)
{
	struct filetype * ft = PN(owq)->selected_filetype ;
	struct  bitfield * bf = ft->data.v ;
	int elements = ft->ag->elements ;
	UINT val ;
	UINT array = 0 ;
	int i ;
	BYTE data[4] ;

	RETURN_ERROR_IF_BAD( FS_r_sibling_U( &val, bf->link, owq ) ) ; // get value
	UT_uint32_to_bytes( val, data ) ; // convert to bytes

	for ( i = 0 ; i < elements ; ++i ) {
		// extract bits
		UT_setbit( &array, i, UT_getbit( data, i*bf->size + bf->shift ) ) ;
	}

	OWQ_U(owq) = array ;
	return 0 ;
}

ZERO_OR_ERROR FS_w_bit_array(struct one_wire_query *owq)
{
	struct filetype * ft = PN(owq)->selected_filetype ;
	struct  bitfield * bf = ft->data.v ;
	int elements = ft->ag->elements ;
	UINT val ;
	UINT array = OWQ_U(owq) ;
	int i ;
	BYTE data[4] ;

	// read it in
	RETURN_ERROR_IF_BAD( FS_r_sibling_U( &val, bf->link, owq ) ) ;
	UT_uint32_to_bytes( val, data ) ;
	
	for ( i = 0 ; i < elements ; ++i ) {
		UT_setbit( data, i*bf->size + bf->shift, UT_getbit( &array, i) ) ;
	}

	// write it out
	return FS_w_sibling_U( UT_uint32(data), bf->link, owq ) ;
}

