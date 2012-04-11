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
	struct {
		struct sensor_node * key ;
		char other[0] ;
	} * opaque = tfind( (void *) (&sense_key), &sensor_tree, sensor_compare ) ;
	return opaque==NULL ? NULL : opaque->key ;
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
	struct {
		struct property_node * key ;
		char other[0] ;
	} * opaque = tfind( (void *) (&property_key), &property_tree, property_compare ) ;
	return opaque==NULL ? NULL : opaque->key ;
}

int family_compare( const void * a , const void * b )
{
	const struct family_node * na = a ;
	const struct family_node * nb = b ;
	return strcmp( na->family, nb->family ) ;
}

struct family_node * Find_External_Family( char * family )
{
	struct family_node family_key = {
		.family = family,
		} ;
	// find family node
	struct {
		struct family_node * key ;
		char other[0] ;
	} * opaque = tfind( (void *) (&family_key), &family_tree, family_compare ) ;
	return opaque==NULL ? NULL : opaque->key ;
}
