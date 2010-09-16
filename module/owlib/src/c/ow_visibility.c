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

/* ------- Prototypes ------------ */

enum e_visibility AlwaysVisible( const struct parsedname * pn )
{
	(void) pn ;
	return visible_always ;
}

enum e_visibility NeverVisible( const struct parsedname * pn )
{
	(void) pn ;
	return visible_never ;
}

/* Internal files */
MakeInternalProp(VIS, fc_persistent);	//cumulative

GOOD_OR_BAD GetVisibilityCache( int * visibility_parameter, const struct parsedname * pn ) 
{
	return Cache_Get_Internal_Strict( visibility_parameter, sizeof(int), InternalProp(VIS), pn) ;
}
	
void SetVisibilityCache( int visibility_parameter, const struct parsedname * pn ) 
{
	Cache_Add_Internal( &visibility_parameter, sizeof(int), InternalProp(VIS), pn) ;
}

enum e_visibility FS_visible( const struct parsedname * pn )
{
	struct filetype * ft = pn->selected_filetype ;
	if ( ft != NULL ) {
		return ft->visible(pn ) ;
	}
	return visible_always ;
}

