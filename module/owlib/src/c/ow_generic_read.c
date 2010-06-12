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
#include "ow_generic_read.h"

/* ------- Prototypes ----------- */
static ZERO_OR_ERROR UnpagedRead( struct generic_read * gread, struct one_wire_query *owq )
{
	size_t size = OWQ_size(owq) ;
	BYTE * p = owmalloc( 3+size );
	struct transaction_log t[] = {
		TRXN_START,
		TRXN_READ( p, 3+size ) ,
		TRXN_END,
	} ;
	
	if ( p==NULL ) {
		return -ENOMEM ;
	}
	
	p[0] = gread->command ;
	p[1] = BYTE_MASK(OWQ_offset(owq)) ;
	p[2] = BYTE_MASK(OWQ_offset(owq)>>8) ;

	if ( BAD( BUS_transaction(t,PN(owq)) ) ) {
		owfree(p) ;
		return -EINVAL ;
	}
	memcpy( OWQ_buffer(owq), &p[3], size ) ;
	owfree(p) ;
	return 0 ;
}

ZERO_OR_ERROR Generic_Read( struct one_wire_query *owq)
{
	struct parsedname * pn = PN(owq) ;
	struct generic_read * gread ;
	
	if ( pn==NULL && pn->selected_device==NULL) {
		return -ENOTSUP ;
	}
	
	gread = pn->selected_device->g_read ;
	if ( gread==NO_GENERIC_READ ) {
		return -ENOTSUP ;
	}
	
	switch ( gread->addressing_type) {
	case rat_not_supported:
		return -ENOTSUP ;
	case rat_2byte:
	case rat_1byte:
	case rat_page:
	case rat_complement:
	default:
		return 0 ;
	}
}
