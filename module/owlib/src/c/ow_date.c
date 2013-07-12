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


#include <config.h>
#include "owfs_config.h"
#include "ow.h"

/* Uses udate to read and write date */

ZERO_OR_ERROR COMMON_r_date( struct one_wire_query * owq )
{
	UINT U ;
	ZERO_OR_ERROR read_sibling = FS_r_sibling_U( &U, PN(owq)->selected_filetype->data.a, owq ) ;
	OWQ_D(owq) = (_DATE) U ;
	return read_sibling ;
}

ZERO_OR_ERROR COMMON_w_date( struct one_wire_query * owq )
{
	UINT U = (UINT) OWQ_D(owq) ;
	return FS_w_sibling_U( U, PN(owq)->selected_filetype->data.a, owq) ;
}
