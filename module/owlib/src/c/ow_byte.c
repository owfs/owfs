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

struct one_wire_query * ALLtoBYTE(struct one_wire_query *owq_all)
{
	struct one_wire_query * owq_byte = OWQ_create_separate( EXTENSION_BYTE, owq_all );
	size_t elements = PN(owq_all)->selected_filetype->ag->elements ;
	size_t extension ;

	if ( owq_byte == NULL ) {
		return NULL ;
	}

	for ( extension = 0 ; extension < elements ; ++extension ) {
		UT_setbit( (BYTE *) &OWQ_U(owq_byte), extension, OWQ_array_Y(owq_all,extension) ) ;
	}
	return owq_byte ;
}	 

struct one_wire_query * BYTEtoALL(struct one_wire_query *owq_byte)
{
	struct one_wire_query * owq_all = OWQ_create_aggregate( owq_byte );
	size_t elements = PN(owq_all)->selected_filetype->ag->elements ;
	size_t extension ;

	if ( owq_all == NULL ) {
		return NULL ;
	}

	for ( extension = 0 ; extension < elements ; ++extension ) {
		OWQ_array_Y(owq_all,extension) = UT_getbit( (BYTE *) &OWQ_U(owq_byte), extension ) ;
	}
	return owq_all ;
}	 
