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

/* ow_parse_external -- read the SENSOR and PROPERTY lines */

/* in general the lines are comma separated tokens
 * with backslash escaping and
 * quote and double quote matching
 * */

#include <config.h>
#include "owfs_config.h"
#include "ow.h"
#include "ow_external.h"

static char * string_parse( char * text_string, char delim, char ** last_char );
static char * trim_parse( char * raw_string );
static char * unquote_parse( char * raw_string );
static int LastParam( char * input_string ) ;
static void create_just_print( char * s_family, char * s_prop, char * s_data );
static void create_subdirs( char * s_family, char * s_prop );

void * property_tree = NULL ;
void * family_tree = NULL ;
void * sensor_tree = NULL ;

// Look through text_string, ignore backslash and match quoting varibles
// allocates a new string with the token
static char * string_parse( char * text_string, char delim, char ** last_char )
{
	char * copy_string ;
	char * old_string_pointer = text_string ;
	char * new_string_pointer ;
		
	if ( text_string == NULL ) {
		*last_char = text_string ;
		return owstrdup("") ;
	}

	new_string_pointer = copy_string = owstrdup( text_string ) ; // more than enough room
	if ( copy_string == NULL ) {
		*last_char = text_string ;
		return NULL ;
	}
			
	while ( 1 ) {
		char current_char = old_string_pointer[0] ;
		new_string_pointer[0] = current_char ; // default copy
		if ( current_char == '\0' ) {
			// end of string
			*last_char = old_string_pointer ;
			return copy_string ;
		} else if ( current_char == '\\' ) {
			// backslash, use next char literally
			if ( old_string_pointer[1] ) {
				new_string_pointer[0] = old_string_pointer[1] ;
				++ new_string_pointer ;
				old_string_pointer += 2 ;
			} else {
				// unless there is no next char
				*last_char = old_string_pointer + 1 ;
				return copy_string ;
			}
		} else if ( current_char == delim ) {
			// found delimitter match
			// point to it, and null terminate
			*last_char = old_string_pointer ;
			new_string_pointer[1] = '\0' ;
			return copy_string ;
		} else if ( current_char == '"' || current_char == '\'' ) {
			// quotation -- find matching end
			char * quote_last_char ;
			char * quoted_string = string_parse( old_string_pointer+1, current_char, &quote_last_char ) ; 
			if ( quoted_string ) {
				strcpy( new_string_pointer+1, quoted_string ) ;
				new_string_pointer += strlen( quoted_string ) + 1 ;
				if ( quote_last_char[0] == current_char ) {
					old_string_pointer = quote_last_char + 1 ;
				} else {
					old_string_pointer = quote_last_char ;
				}
				owfree( quoted_string ) ;
			} else {
				new_string_pointer[1] = '\0' ;
				*last_char = old_string_pointer ;
				return copy_string ;
			}
		} else {
			++ old_string_pointer ;
			++ new_string_pointer ;
		}
	}
}
	
static char * trim_parse( char * raw_string )
{
	char * start_position ;
	char * end_position ;
	char * return_string ;
	for ( start_position = raw_string ; start_position[0] ; ++ start_position ) {
		if ( start_position[0] != ' ' ) {
			break ;
		}
	}
	return_string = owstrdup ( start_position ) ;
	owfree( raw_string ) ;
	
	for ( end_position = return_string + strlen( return_string ) ; end_position >= return_string ; -- end_position ) {
		if ( end_position[0] == '\0' ) {
			continue ;
		} else if ( end_position[0] != ' ' ) {
			break ;
		} else {
			end_position[0] = '\0' ;
		}
	}
	
	return return_string ;
}

static char * unquote_parse( char * raw_string )
{
	if ( raw_string == NULL ) {
		return NULL ;
	}
	switch ( raw_string[0] ) {		
	case '"':
	case '\'':
		if ( raw_string[1] == '\0' ) {
			owfree( raw_string ) ;
			return owstrdup("") ;
		} else {
			char * unquoted = owstrdup( raw_string+1 ) ;
			char * unquoted_end = unquoted + strlen(unquoted) -1 ;
			if ( unquoted_end[0] == raw_string[0] ) {
				unquoted_end[0] = '\0' ;
			}
			owfree( raw_string ) ;
			return unquoted ;
		}
		break ;
	default:
		return raw_string ;
	}
}	
	
static int LastParam( char * input_string )
{
	if ( input_string == NULL ) {
		return 1 ;
	} else {
		int len = strlen(input_string) ;
		if ( len == 0 ) {
			return 1 ;
		}
		if ( input_string[len-1] != ',' ) {
			return 1 ;
		}
		input_string[len-1] = '\0' ;
		return 0 ;
	}
}

// Gets a string with property,family,structure,read_function,write_function,property_data,extra
// write, data and extra are optional
/*
 * property
 * family
 * structure
 * read
 * write
 * data
 * other
 * */
void AddProperty( char * input_string )
{
	char * s_property ;
	char * s_family ;
	char * s_structure ;
	char * s_read ;
	char * s_write ;
	char * s_data ;
	char * s_other ;
	char * end_pointer ;
	char * start_pointer = input_string ;
	
	if ( input_string == NULL ) {
		return ;
	}
	
	// property
	s_property = string_parse( start_pointer, ',' , &end_pointer ) ;
	start_pointer = end_pointer ;
	if ( !LastParam( s_property ) ) {
		++start_pointer ;
	}
	s_property = unquote_parse(trim_parse(s_property)) ;

	// family
	s_family = string_parse( start_pointer, ',' , &end_pointer ) ;
	start_pointer = end_pointer ;
	if ( !LastParam( s_property ) ) {
		++start_pointer ;
	}
	s_family = unquote_parse(trim_parse(s_family)) ;

	// structure
	s_structure = string_parse( start_pointer, ',' , &end_pointer ) ;
	start_pointer = end_pointer ;
	if ( !LastParam( s_property ) ) {
		++start_pointer ;
	}
	s_structure = unquote_parse(trim_parse(s_structure)) ;

	// read
	s_read = string_parse( start_pointer, ',' , &end_pointer ) ;
	start_pointer = end_pointer ;
	if ( !LastParam( s_property ) ) {
		++start_pointer ;
	}
	s_read = unquote_parse(trim_parse(s_read)) ;

	// write
	s_write = string_parse( start_pointer, ',' , &end_pointer ) ;
	start_pointer = end_pointer ;
	if ( !LastParam( s_property ) ) {
		++start_pointer ;
	}
	s_write = unquote_parse(trim_parse(s_write)) ;

	// data
	s_data = string_parse( start_pointer, ',' , &end_pointer ) ;
	start_pointer = end_pointer ;
	if ( !LastParam( s_property ) ) {
		++start_pointer ;
	}
	s_data = unquote_parse(trim_parse(s_data)) ;

	// other
	s_other = string_parse( start_pointer, ',' , &end_pointer ) ;
	start_pointer = end_pointer ;
	if ( !LastParam( s_property ) ) {
		++start_pointer ;
	}
	s_other = unquote_parse(trim_parse(s_other)) ;

	// test minimums
	if ( strlen( s_family ) > 0 && strlen( s_property ) > 0 ) {
		// Actually add
		AddFamilyToTree( s_family ) ;
		AddPropertyToTree( s_family, s_property, s_structure, s_read, s_write, s_data, s_other ) ;
		create_subdirs( s_family, s_property ) ;
	}

	// Clean up
	owfree( s_property ) ;
	owfree( s_family ) ;
	owfree( s_structure ) ;
	owfree( s_read ) ;
	owfree( s_write ) ;
	owfree( s_data ) ;
	owfree( s_other ) ;
}

// Gets a string with name,family, and description
// description is optional
/*
 * name
 * family
 * description
 * */
void AddSensor( char * input_string )
{
	char * s_name ;
	char * s_family ;
	char * s_description = NULL ;
	char * end_pointer ;
	char * start_pointer ;
	int is_there_more ;
	
	if ( input_string == NULL ) {
		return ;
	}
	
	// name
	s_name = string_parse( input_string, ',' , &end_pointer ) ;
	is_there_more = !LastParam( s_name ) ;
	if ( !is_there_more ) {
		owfree( s_name ) ;
		return ;
	}
	s_name = unquote_parse(trim_parse(s_name)) ;

	// family
	start_pointer = end_pointer+1 ;
	s_family = string_parse( start_pointer+1, ',' , &end_pointer ) ;
	is_there_more = !LastParam( s_family ) ;
	s_family = unquote_parse(trim_parse(s_family)) ;

	// description
	if ( is_there_more ) {
		start_pointer = end_pointer+1 ;
		s_description = string_parse( start_pointer+1, ',' , &end_pointer ) ;
		is_there_more = !LastParam( s_description ) ; // for side effects
		s_description = unquote_parse(trim_parse(s_description)) ;		
	} else {
		s_description = owstrdup("") ;
	}

	// Actually add
	AddFamilyToTree( s_family ) ;
	AddSensorToTree( s_name, s_family, s_description ) ;
	create_just_print( s_family, "family", s_family ) ;
	create_just_print( s_family, "type", "external" ) ;

	// Clean up
	owfree( s_name );
	owfree( s_family ) ;
	owfree( s_description ) ;
}

int sensor_compare( const void * a , const void * b )
{
	const struct sensor_node * na = a ;
	const struct sensor_node * nb = b ;
	return strcmp( na->name, nb->name ) ;
}

int family_compare( const void * a , const void * b )
{
	const struct sensor_node * na = a ;
	const struct sensor_node * nb = b ;
	return strcmp( na->family, nb->family ) ;
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

struct sensor_node * create_sensor_node( char * s_name, char * s_family, char * s_description )
{
	int l_name = strlen(s_name)+1;
	int l_family = strlen(s_family)+1;
	int l_description = strlen(s_description)+1;

	struct sensor_node * s = owmalloc( sizeof(struct sensor_node) 
	+ l_name +  l_family + l_description ) ;

	if ( s==NULL) {
		return NULL ;
	}

	s->name = s->payload ;
	strcpy( s->name, s_name ) ;

	s->family = s->name + l_name ;
	strcpy( s->family, s_family ) ;

	s->description = s->family + l_family ;
	strcpy( s->description, s_description ) ;

	return s ;
}

struct family_node * create_family_node( char * s_family )
{
	int l_family = strlen(s_family)+1;

	struct family_node * s = owmalloc( sizeof(struct sensor_node) 
	+  l_family ) ;

	if ( s==NULL) {
		return NULL ;
	}

	s->family = s->payload ;
	strcpy( s->family, s_family ) ;

	return s ;
}

struct property_node * create_property_node( char * s_property, char * s_family, char * s_structure, char * s_read, char * s_write, char * s_data, char * s_other )
{
	int l_family = strlen( s_family )+1 ;
	int l_property = strlen( s_property )+1 ;
	int l_structure = strlen ( s_structure )+1 ;
	int l_read = strlen( s_read )+1 ;
	int l_write = strlen( s_write )+1 ;
	int l_data = strlen( s_data )+1 ;
	int l_other = strlen( s_other )+1 ;

	struct property_node * s = owmalloc( sizeof(struct property_node) 
	+ l_family + l_property + l_structure + l_read + l_write + l_data + l_other ) ;
	
	if ( s==NULL) {
		return NULL ;
	}

	s->family = s->payload ;
	strcpy( s->family, s_family ) ;

	s->property = s->family + l_family ;
	strcpy( s->property, s_property ) ;

	s->structure = s->family + l_family ;
	strcpy( s->property, s_property ) ;

	s->read = s->structure + l_structure ;
	strcpy( s->read, s_read ) ;

	s->write = s->read + l_read ;
	strcpy( s->write, s_write ) ;

	s->data = s->write + l_write ;
	strcpy( s->data, s_data ) ;

	s->other = s->data + l_data ;
	strcpy( s->other, s_other ) ;

	return s ;
}

void AddSensorToTree( char * s_name, char * s_family, char * s_description )
{
	struct sensor_node * n = create_sensor_node( s_name, s_family, s_description ) ;
	struct sensor_node * s = tsearch( (void *) n, &sensor_tree, sensor_compare ) ;
	
	if ( s != n ) {
		// already exists
		LEVEL_DEBUG("Duplicate sensor entry: %s,%s,%s",s_name,s_family,s_description);
		owfree( n ) ;
	} else {
		LEVEL_DEBUG("New sensor entry: %s,%s,%s",s_name,s_family,s_description);
	}
}

void AddFamilyToTree( char * s_family )
{
	struct family_node * n = create_family_node( s_family ) ;
	struct family_node * s = tsearch( (void *) n, &family_tree, family_compare ) ;
	
	if ( s != n ) {
		// already exists
		LEVEL_DEBUG("Duplicate family entry: %s",s_family);
		owfree( n ) ;
	} else {
		LEVEL_DEBUG("New family entry: %s",s_family);
	}
}

void AddPropertyToTree( char * s_family, char * s_property, char * s_structure, char * s_read, char * s_write, char * s_data, char * s_other )
{
	struct property_node * n = create_property_node( s_family, s_property, s_structure, s_read, s_write, s_data, s_other ) ;
	struct property_node * s = tsearch( (void *) n, &property_tree, property_compare ) ;
	
	if ( s != n ) {
		// already exists
		LEVEL_DEBUG("Duplicate property entry: %s,%s,%s,%s,%s,%s,%s",s_property,s_family,s_structure,s_read,s_write,s_data,s_other);
		owfree( n ) ;
	} else {
		LEVEL_DEBUG("New property entry: %s,%s,%s,%s,%s,%s,%s",s_property,s_family,s_structure,s_read,s_write,s_data,s_other);
	}		
}

static void create_just_print( char * s_family, char * s_prop, char * s_data )
{
	char structure_string[50] ;
	
	snprintf( structure_string, 49, "%c,%.6d,%.6d,%.2s,%.6d,%c,",
		'a',
		0,
		1,
		"ro",
		(int) strlen(s_data),
		'f' ) ;
	AddPropertyToTree( s_family, s_prop, structure_string, "just_print_data", "", s_data, "" ) ;
}

static void create_subdirs( char * s_family, char * s_prop )
{
	char * slash ;
	char * subdir = owstrdup( s_prop ) ;
	
	if ( subdir == NULL ) {
		return ;
	}
	
	while ( (slash = strrchr( subdir, '/' )) != NULL ) {
		slash[0] = '\0' ;
		AddPropertyToTree( s_family, subdir, "D,0,1,oo,0,f","","","","" ) ;
	}
	
	owfree(subdir) ;
}
