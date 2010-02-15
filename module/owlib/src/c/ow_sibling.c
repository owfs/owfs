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
	int sib_status ;

	if ( owq_sibling == NULL ) {
		return -EINVAL ;
	}
	sib_status = FS_read_local(owq_sibling) ;
	F[0] = OWQ_F(owq_sibling) ;
	FS_OWQ_destroy(owq_sibling) ;
	return sib_status?-EINVAL:0;
}

int FS_w_sibling_F(_FLOAT F, const char * sibling, struct one_wire_query *owq)
{
	int write_error;
	struct one_wire_query * owq_sibling  = FS_OWQ_create_sibling( sibling, owq ) ;

	if ( owq_sibling == NULL ) {
		return -EINVAL ;
	}
	OWQ_F(owq) = F ;
	write_error = FS_write_local(owq_sibling) ;
	FS_OWQ_destroy(owq_sibling) ;
	return write_error ;
}

/* DATE */
int FS_r_sibling_D(_DATE *D, const char * sibling, struct one_wire_query *owq)
{
	struct one_wire_query * owq_sibling  = FS_OWQ_create_sibling( sibling, owq ) ;
	int sib_status ;

	if ( owq_sibling == NULL ) {
		return -EINVAL ;
	}
	sib_status = FS_read_local(owq_sibling) ;
	memcpy ( D, &OWQ_D(owq_sibling), sizeof( _DATE ) ) ;
	FS_OWQ_destroy(owq_sibling) ;
	return sib_status?-EINVAL:0;
}

int FS_w_sibling_D(_DATE D, const char * sibling, struct one_wire_query *owq)
{
	int write_error;
	struct one_wire_query * owq_sibling  = FS_OWQ_create_sibling( sibling, owq ) ;

	if ( owq_sibling == NULL ) {
		return -EINVAL ;
	}
	OWQ_D(owq_sibling) = D ;
	write_error = FS_write_local(owq_sibling) ;
	FS_OWQ_destroy(owq_sibling) ;
	return write_error ;
}

int FS_r_sibling_U(UINT *U, const char * sibling, struct one_wire_query *owq)
{
	struct one_wire_query * owq_sibling  = FS_OWQ_create_sibling( sibling, owq ) ;
	int sib_status ;

	if ( owq_sibling == NULL ) {
		return -EINVAL ;
	}
	sib_status = FS_read_local(owq_sibling) ;
	U[0] = OWQ_U(owq_sibling) ;
	FS_OWQ_destroy(owq_sibling) ;
	return sib_status?-EINVAL:0;
}

int FS_r_sibling_binary(BYTE * data, size_t * size, const char * sibling, struct one_wire_query *owq)
{
	struct one_wire_query * owq_sibling  = FS_OWQ_create_sibling( sibling, owq ) ;
	int sib_status ;

	if ( owq_sibling == NULL ) {
		return -EINVAL ;
	}
	OWQ_offset(owq_sibling) = 0 ;
	sib_status = FS_read_local(owq_sibling) ;
	if ( (sib_status == 0) && (OWQ_length(owq_sibling) <= size[0]) ) {
		memset(data, 0, size[0] ) ;
		size[0] = OWQ_length(owq_sibling) ;
		memcpy( data, OWQ_buffer(owq_sibling), size[0] ) ;
	} else {
		sib_status = -ENOMEM ;
	}
	FS_OWQ_destroy(owq_sibling) ;
	return sib_status?-EINVAL:0;
}

int FS_w_sibling_U(UINT U, const char * sibling, struct one_wire_query *owq)
{
	int write_error;
	struct one_wire_query * owq_sibling  = FS_OWQ_create_sibling( sibling, owq ) ;

	if ( owq_sibling == NULL ) {
		return -EINVAL ;
	}
	OWQ_U(owq) = U ;
	write_error = FS_write_local(owq_sibling) ;
	FS_OWQ_destroy(owq_sibling) ;
	return write_error ;
}

int FS_r_sibling_Y(INT *Y, const char * sibling, struct one_wire_query *owq)
{
	struct one_wire_query * owq_sibling  = FS_OWQ_create_sibling( sibling, owq ) ;
	int sib_status ;

	if ( owq_sibling == NULL ) {
		return -EINVAL ;
	}
	sib_status = FS_read_local(owq_sibling) ;
	Y[0] = OWQ_Y(owq_sibling) ;
	FS_OWQ_destroy(owq_sibling) ;
	return sib_status?-EINVAL:0;
}

int FS_w_sibling_Y(INT Y, const char * sibling, struct one_wire_query *owq)
{
	int write_error;
	struct one_wire_query * owq_sibling  = FS_OWQ_create_sibling( sibling, owq ) ;

	if ( owq_sibling == NULL ) {
		return -EINVAL ;
	}
	OWQ_Y(owq) = Y ;
	write_error = FS_write_local(owq_sibling) ;
	FS_OWQ_destroy(owq_sibling) ;
	return write_error ;
}

int FS_w_sibling_bitwork(UINT set, UINT mask, const char * sibling, struct one_wire_query *owq)
{
	int write_error = -EINVAL ;
	struct one_wire_query * owq_sibling  = FS_OWQ_create_sibling( sibling, owq ) ;

	if ( owq_sibling == NULL ) {
		return -EINVAL ;
	}
	if ( FS_read_local(owq_sibling) == 0 ) {
		UINT bitfield = OWQ_U(owq_sibling) ;

		// clear mask:
		bitfield &= ~mask ;

		// set bits:
		bitfield |= (set & mask) ;

		OWQ_U(owq_sibling) = bitfield ;

		write_error = FS_write_local(owq_sibling);
	}

	FS_OWQ_destroy(owq_sibling) ;
	return write_error;
}

/* Delete entry in cache */
void FS_del_sibling(const char * sibling, struct one_wire_query *owq)
{
	struct one_wire_query * owq_sibling  = FS_OWQ_create_sibling( sibling, owq ) ;

	if ( owq_sibling == NULL ) {
		return ;
	}
	Cache_Del(PN(owq_sibling)) ;
	FS_OWQ_destroy(owq_sibling) ;
}
