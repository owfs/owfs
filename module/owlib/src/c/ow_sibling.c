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

/* FLOAT */
int FS_r_sibling_F(_FLOAT *F, const char * sibling, struct one_wire_query *owq)
{
	struct one_wire_query * owq_sibling  = FS_OWQ_create_sibling( sibling, owq ) ;

	if ( owq_sibling == NULL ) {
		return -EINVAL ;
	}
	if ( FS_read_local(owq_sibling) ) {
		FS_OWQ_destroy_sibling(owq_sibling) ;
		return -EINVAL ;
	}

	*F = OWQ_F(owq_sibling) ;

	FS_OWQ_destroy_sibling(owq_sibling) ;
	return 0;
}

int FS_w_sibling_F(_FLOAT F, const char * sibling, struct one_wire_query *owq)
{
	int write_error = -EINVAL ;
	struct one_wire_query * owq_sibling  = FS_OWQ_create_sibling( sibling, owq ) ;

	if ( owq_sibling != NULL ) {
		OWQ_F(owq) = F ;
		write_error = FS_write_local(owq_sibling) ;
		FS_OWQ_destroy_sibling(owq_sibling) ;
	}

	return write_error ;
}

/* DATE */
int FS_r_sibling_D(_DATE *D, const char * sibling, struct one_wire_query *owq)
{
	struct one_wire_query * owq_sibling  = FS_OWQ_create_sibling( sibling, owq ) ;

	if ( owq_sibling == NULL ) {
		return -EINVAL ;
	}
	if ( FS_read_local(owq_sibling) ) {
		FS_OWQ_destroy_sibling(owq_sibling) ;
		return -EINVAL ;
	}

	memcpy ( D, &OWQ_D(owq_sibling), sizeof( _DATE ) ) ;

	FS_OWQ_destroy_sibling(owq_sibling) ;
	return 0;
}

int FS_w_sibling_D(_DATE D, const char * sibling, struct one_wire_query *owq)
{
	int write_error = -EINVAL ;
	struct one_wire_query * owq_sibling  = FS_OWQ_create_sibling( sibling, owq ) ;

	if ( owq_sibling != NULL ) {
		OWQ_D(owq_sibling) = D ;
		write_error = FS_write_local(owq_sibling) ;
		FS_OWQ_destroy_sibling(owq_sibling) ;
	}

	return write_error ;
}

int FS_r_sibling_U(UINT *U, const char * sibling, struct one_wire_query *owq)
{
	struct one_wire_query * owq_sibling  = FS_OWQ_create_sibling( sibling, owq ) ;

	if ( owq_sibling == NULL ) {
		return -EINVAL ;
	}
	if ( FS_read_local(owq_sibling) ) {
		FS_OWQ_destroy_sibling(owq_sibling) ;
		return -EINVAL ;
	}

	*U = OWQ_U(owq_sibling) ;

	FS_OWQ_destroy_sibling(owq_sibling) ;
	return 0;
}

int FS_w_sibling_U(UINT U, const char * sibling, struct one_wire_query *owq)
{
	int write_error = -EINVAL ;
	struct one_wire_query * owq_sibling  = FS_OWQ_create_sibling( sibling, owq ) ;

	if ( owq_sibling != NULL ) {
		OWQ_U(owq) = U ;
		write_error = FS_write_local(owq_sibling) ;
		FS_OWQ_destroy_sibling(owq_sibling) ;
	}

	return write_error ;
}

int FS_r_sibling_Y(INT *Y, const char * sibling, struct one_wire_query *owq)
{
	struct one_wire_query * owq_sibling  = FS_OWQ_create_sibling( sibling, owq ) ;

	if ( owq_sibling == NULL ) {
		return -EINVAL ;
	}
	if ( FS_read_local(owq_sibling) ) {
		FS_OWQ_destroy_sibling(owq_sibling) ;
		return -EINVAL ;
	}

	*Y = OWQ_Y(owq_sibling) ;

	FS_OWQ_destroy_sibling(owq_sibling) ;
	return 0;
}

int FS_w_sibling_Y(INT Y, const char * sibling, struct one_wire_query *owq)
{
	int write_error = -EINVAL ;
	struct one_wire_query * owq_sibling  = FS_OWQ_create_sibling( sibling, owq ) ;

	if ( owq_sibling != NULL ) {
		OWQ_Y(owq) = Y ;
		write_error = FS_write_local(owq_sibling) ;
		FS_OWQ_destroy_sibling(owq_sibling) ;
	}

	return write_error ;
}

int FS_w_sibling_bitwork(UINT set, UINT mask, const char * sibling, struct one_wire_query *owq)
{
	struct one_wire_query * owq_sibling  = FS_OWQ_create_sibling( sibling, owq ) ;
	UINT bitfield ;

	if ( owq_sibling == NULL ) {
		return -EINVAL ;
	}
	if ( FS_read_local(owq_sibling) ) {
		FS_OWQ_destroy_sibling(owq_sibling) ;
		return -EINVAL ;
	}

	bitfield = OWQ_U(owq_sibling) ;

	// clear mask:
	bitfield &= ~mask ;

	// set bits:
	bitfield |= (set & mask) ;

	OWQ_U(owq_sibling) = bitfield ;

	if ( FS_write_local(owq_sibling) ) {
		FS_OWQ_destroy_sibling(owq_sibling) ;
		return -EINVAL ;
	}

	FS_OWQ_destroy_sibling(owq_sibling) ;
	return 0;
}
