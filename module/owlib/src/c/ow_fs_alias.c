/*
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
#include "ow_standard.h"

/* ------- Prototypes ------------ */

/* ------- Functions ------------ */

ZERO_OR_ERROR FS_r_alias(struct one_wire_query *owq)
{
	BYTE * sn = OWQ_pn(owq).sn ;
	ASCII * alias_name = Cache_Get_Alias( sn ) ;

	if ( alias_name != NULL ) {
		ZERO_OR_ERROR zoe = OWQ_format_output_offset_and_size_z(alias_name, owq);
		LEVEL_DEBUG("Found alias %s for "SNformat,alias_name,SNvar(sn));
		owfree( alias_name ) ;
		return zoe;
	}

	LEVEL_DEBUG("Didn't find alias %s for "SNformat,alias_name,SNvar(sn));
	return OWQ_format_output_offset_and_size_z("", owq);
}

ZERO_OR_ERROR FS_w_alias(struct one_wire_query *owq)
{
	size_t size = OWQ_size(owq) ;
	ASCII * alias_name = owmalloc( size+1 ) ;
	GOOD_OR_BAD gob ;

	if ( alias_name == NULL ) {
		return -ENOMEM ;
	}

	// make a slightly larger buffer and add a null terminator
	memset( alias_name, 0, size+1 ) ;
	memcpy( alias_name, OWQ_buffer(owq), size ) ;
	
	gob = Test_and_Add_Alias( alias_name, OWQ_pn(owq).sn ) ;

	owfree( alias_name ) ;
	return GOOD(gob) ? 0 : -EINVAL ;
}

