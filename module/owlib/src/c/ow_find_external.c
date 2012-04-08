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
#include "ow_external.h"

int sensor_compare( const void * a , const void * b )
{
	const struct sensor_node * na = a ;
	const struct sensor_node * nb = b ;
	return strcmp( na->name, nb->name ) ;
}

struct sensor_node * Find_External_Sensor( char * sensor )
{
	struct sensor_node sense_key = {
		.name = sensor,
		} ;
	// find sensor node
	return (struct sensor_node *) tfind( (void *) (&sense_key), &sensor_tree, sensor_compare ) ;
}
	
int property_compare( const void * a , const void * b )
{
	const struct property_node * na = a ;
	const struct property_node * nb = b ;
	int c = strcmp( na->family, nb->family ) ;
	if ( c==0 ) {
		return strcmp( na->property, nb->property ) ;
	}
	return c ;
}

struct property_node * Find_External_Property( char * family, char * property )
{
	struct property_node property_key = {
		.family = family ,
		.property = property ,
	} ;
	// find property node
	return (struct property_node *) tfind( (void *) (&property_key), &property_tree, property_compare ) ;
}


