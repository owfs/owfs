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
#include "ow_connection.h"

GOOD_OR_BAD BUS_Set_Config(void * v, const struct parsedname *pn)
{
	GOOD_OR_BAD (*set_config) (void * param, const struct parsedname *pn) = pn->selected_connection->iroutines.set_config ;
	if ( set_config != NO_SET_CONFIG_ROUTINE ) {
		return (set_config)(v,pn) ;
	}
	return gbBAD ;
}

GOOD_OR_BAD BUS_Get_Config(void * v, const struct parsedname *pn)
{
	GOOD_OR_BAD (*get_config) (void * param, const struct parsedname *pn) = pn->selected_connection->iroutines.get_config ;
	if ( get_config != NO_GET_CONFIG_ROUTINE ) {
		return (get_config)(v,pn) ;
	}
	return gbBAD ;
}
