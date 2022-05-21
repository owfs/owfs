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
#include "ow.h"
#include "ow_external.h"


/* strategy for external write:
   A common write function for all communication schemes e.g. script
   The actual device record and actual family record is obtained from the trees.
   The script is called (via popen) or other method for other types
   The device should be locked for the communication
   arguments include fields from the tree records and owq
   the returned value is obvious
*/

static ZERO_OR_ERROR OW_trees_for_write( char * device, char * property, struct one_wire_query * owq ) ;

static ZERO_OR_ERROR OW_write_external_script( struct sensor_node * sensor_n, struct property_node * property_n, struct one_wire_query * owq ) ;

static ZERO_OR_ERROR OW_script_write( FILE * script_f, struct one_wire_query * owq ) ;

// ------------------------


ZERO_OR_ERROR FS_w_external( struct one_wire_query * owq )
{
	ZERO_OR_ERROR zoe = -ENOENT ; // default
	char * device_property = owstrdup( PN(owq)->device_name ) ; // copy of device/property part of path
	
	if ( device_property != NULL ) {
		char * property = device_property ;
		char * device = strsep( &property, "/" ) ; // separate out property
		char * extension = property ;
		if ( property != NULL ) { // pare off extension
			property = strsep( &extension, "." ) ;
		}

		zoe = OW_trees_for_write( device, property, owq ) ;

		owfree( device_property ) ; // clean up
	}
	return zoe ;
}

static ZERO_OR_ERROR OW_trees_for_write( char * device, char * property, struct one_wire_query * owq )
{
	// find sensor node
	struct sensor_node * sense_n = Find_External_Sensor( device ) ;
	
	if ( sense_n != NULL ) {
		// found
		// find property node
		struct property_node * property_n = Find_External_Property( sense_n->family, property ) ;
		
		if ( property_n != NULL ) {
			switch ( property_n->et ) {
				case et_none:
					return 0 ;
				case et_internal:
					return -ENOTSUP ;
				case et_script:
					return OW_write_external_script( sense_n, property_n, owq ) ;
				default:
					return -ENOTSUP ;
			}
		}
	}
	return -ENOENT ;
}

static ZERO_OR_ERROR OW_write_external_script( struct sensor_node * sensor_n, struct property_node * property_n, struct one_wire_query * owq )
{
	char cmd[PATH_MAX+1] ;
	struct parsedname * pn = PN(owq) ;
	FILE * script_f ;
	int snp_return ;
	ZERO_OR_ERROR zoe ;
	
	
	// load the command script and arguments
	if ( pn->sparse_name == NULL ) {
		// not a text sparse name
		snp_return =
		snprintf( cmd, PATH_MAX+1, "%s %s %s %d %s %d %d %s %s",
			property_n->write, // command
			sensor_n->name, // sensor name
			property_n->property, // property,
			pn->extension, // extension
			"write", // mode
			(int) OWQ_size(owq), // size
			(int) OWQ_offset(owq), // offset
			sensor_n->data, // sensor-specific data
			property_n->data // property-specific data
		) ;
	} else {
		snp_return =
		snprintf( cmd, PATH_MAX+1, "%s %s %s %s %s %d %d %s %s",
			property_n->write, // command
			sensor_n->name, // sensor name
			property_n->property, // property,
			pn->sparse_name, // extension
			"write", // mode
			(int) OWQ_size(owq), // size
			(int) OWQ_offset(owq), // offset
			sensor_n->data, // sensor-specific data
			property_n->data // property-specific data
		) ;
	}

	if ( snp_return < 0 ) {
		LEVEL_DEBUG("Problem creating script string for %s/%s",sensor_n->name,property_n->property) ;
		return -EINVAL ;
	}
	
	script_f = popen( cmd, "w" ) ;
	if ( script_f == NULL ) {
		ERROR_DEBUG("Cannot create external program link for writing %s/%s",sensor_n->name,property_n->property);
		return -EIO ;
	}
	
	zoe = OW_script_write( script_f, owq ) ;
	
	pclose( script_f ) ;
	return zoe ;
}

static ZERO_OR_ERROR OW_script_write( FILE * script_f, struct one_wire_query * owq )
{
	size_t fr_return ;

	int po_return = OWQ_parse_output(owq) ; // load data in buffer
	
	if ( po_return < 0 ) {
		return -EINVAL ;
	}
	
	fr_return = fwrite( OWQ_buffer(owq), po_return, 1, script_f ) ;
	
	if ( fr_return == 0 && ferror(script_f) != 0 ) {
		LEVEL_DEBUG( "Could not write script data back for %s",PN(owq)->path ) ;
		return -EIO ;
	}
	
	return 0 ;
}
