/*
    OWFS -- One-Wire filesystem
    OWHTTPD -- One-Wire Web Server
    Written 2003 Paul H Alfille
    email: paul.alfille@gmail.com
    Released under the GPL
    See the header file: ow.h for full attribution
    1wire/iButton system from Dallas Semiconductor
*/

/* ow_parse_external -- read the SENSOR and PROPERTY lines */

/* in general the lines are comma separated tokens
 * with backslash escaping and
 * quote and double quote matching
 * */

/* 11/2014 -- considerable testing and patches from Sven Giermann */

#include <config.h>
#include "owfs_config.h"
#include "ow.h"
#include "ow_external.h"

static char * string_parse( char * text_string, char delim, char ** last_char );
static char * trim_parse( char * raw_string );
static char * unquote_parse( char * raw_string );
static int LastParam( char * input_string ) ;
static void create_just_print( char * s_prop, char * s_family, char * s_data );
static void create_subdirs( char * s_prop, char * s_family );

static void AddFamilyToTree( char * s_family ) ;
static struct family_node * create_family_node( char * s_family ) ;

static void AddSensorToTree( char * s_name, char * s_family, char * s_description, char * data ) ;
static struct sensor_node * create_sensor_node( char * s_name, char * s_family, char * s_description, char * s_data ) ;

static void AddPropertyToTree( char * s_property, char * s_family, enum ft_format s_format, size_t s_array, enum ag_combined s_combined, enum ag_index s_index_type, size_t s_length, enum fc_change s_change, char * s_read, char * s_write, char * s_data, char * s_other, enum external_type et ) ;
static struct property_node * create_property_node( char * s_property, char * s_family, enum ft_format s_format, size_t s_array, enum ag_combined s_combined, enum ag_index s_index_type, size_t s_length, enum fc_change s_change, char * s_read, char * s_write, char * s_data, char * s_other, enum external_type et ) ;

void * property_tree = NULL ;
void * family_tree = NULL ;
void * sensor_tree = NULL ;

// For AddSensor and AddProperty
// uses the various tools to get a string into s_name (name is given)
// start_pointer (a char *) is used and updated.
#define GetQuotedString( name ) do {                               \
	char * end_pointer ;                                          \
	s_##name = string_parse( start_pointer, ',', &end_pointer ) ; \
	start_pointer = end_pointer ;                                 \
	if ( ! LastParam( s_##name ) ) {                               \
		++ start_pointer ;                                        \
	}                                                             \
	s_##name = unquote_parse( trim_parse( s_##name ) ) ;          \
	LEVEL_DEBUG( #name" assigned %s",s_##name ) ;               \
} while (0) ;

// Look through text_string, ignore backslash and match quoting varibles
// allocates a new string with the token
static char * string_parse( char * text_string, char delim, char ** last_char )
{
	char * copy_string ;
	char * old_string_pointer = text_string ;
	char * new_string_pointer ;

//printf("->>>STRING (%c) PARSE <%s>\n",delim,text_string);
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
//printf("<<<-STRING (%c) PARSE <%s>, end:<%s>\n",delim,copy_string,*last_char);
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
//printf("<<<-STRING (%c) PARSE <%s>, end:<%s>\n",delim,copy_string,*last_char);
				return copy_string ;
			}
		} else if ( current_char == delim ) {
			// found delimitter match
			// point to it, and null terminate
			*last_char = old_string_pointer ;
			new_string_pointer[1] = '\0' ;
//printf("<<<-STRING (%c) PARSE <%s>, end:<%s>\n",delim,copy_string,*last_char);
			return copy_string ;
		} else if ( current_char == '"' || current_char == '\'' ) {
			// quotation -- find matching end
			char * quote_last_char ;
			char * quoted_string = string_parse( old_string_pointer+1, current_char, &quote_last_char ) ; 
//printf("----STRING (%c) PARSE Quote <%s> end:<%s>\n",delim,quoted_string,quote_last_char);
			if ( quoted_string ) {
				strcpy( new_string_pointer+1, quoted_string ) ;
				new_string_pointer += strlen( quoted_string ) + 1 ;
				if ( quote_last_char[0] == current_char ) {
					quote_last_char[0] = '\0' ; // clear trailing quote
					old_string_pointer = quote_last_char + 1 ;
				} else {
					old_string_pointer = quote_last_char ;
				}
				owfree( quoted_string ) ;
			} else {
				new_string_pointer[1] = '\0' ;
				*last_char = old_string_pointer ;
//printf("<<<-STRING (%c) PARSE <%s>, end:<%s>\n",delim,copy_string,*last_char);
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

	// trim off preamble whitespace
	for ( start_position = raw_string ; start_position[0] ; ++ start_position ) {
		switch ( start_position[0] ) {
			case ' ':
			case '\t':
			// ignore these characters
				continue ;
			case '\n':
			case '\r':
				// end of the line -- empty string return
				owfree( raw_string );
				return owstrdup("") ;
			default:
				break ;
		}
		// getting here means a valid character found
		break ;
	}

	// delete old string and use new copy starting at first non whitespace position
	return_string = owstrdup ( start_position ) ;
	owfree( raw_string ) ;
	
	// trim off 
	for ( end_position = return_string + strlen( return_string ) ; end_position >= return_string ; -- end_position ) {
		switch ( end_position[0] ) {
			case ' ':
			case '\0':
			case '\r':
			case '\n':
			case '\t':
				// ignorable end char -- make null
				end_position[0] = '\0' ;
				continue ;
			default:
				// good char
				break ;			
		}
		// getting here means a valid character found
		break ;
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

// starts with script:
// or property:
/*
 * property
 * family
 * structure
 * read
 * write
 * data
 * other
 * */
void AddProperty( char * input_string, enum external_type et )
{
	char * s_family = NULL ;
	char * s_property = NULL ;
	char * s_dummy ;
	ssize_t s_array ;
	enum ag_combined s_combined ;
	enum ag_index s_index_type ;
	enum ft_format s_format ;
	ssize_t s_length ;
	enum fc_change s_change ;
	char * s_read = NULL ;
	char * s_write = NULL ;
	char * s_data = NULL ;
	char * s_other = NULL;
	char * start_pointer = input_string ;
	
	if ( input_string == NULL ) {
		return ;
	}
	
	if ( ! Globals.allow_external ) {
		LEVEL_DEBUG("External prgroams not supported by %s",Globals.progname) ;
		return ;
	}

	// property
	GetQuotedString( property ) ;
	
	// family
	GetQuotedString( family ) ;

	// type
	GetQuotedString( dummy ) ;
	switch ( s_dummy[0] ) {
		case 'D':
			s_length = PROPERTY_LENGTH_DIRECTORY ;
			s_format = ft_directory ;
			break ;
		case 'i':
			s_length = PROPERTY_LENGTH_INTEGER ;
			s_format = ft_integer ;
			break ;
		case 'u':
			s_length = PROPERTY_LENGTH_UNSIGNED ;
			s_format = ft_unsigned ;
			break ;
		case 'f':
			s_length = PROPERTY_LENGTH_FLOAT ;
			s_format = ft_float ;
			break ;
		case 'a':
			s_length = 1 ;
			s_format = ft_ascii ;
			break ;
		case 'b':
			s_length = 1 ;
			s_format = ft_binary ;
			break ;
		case 'y':
			s_length = PROPERTY_LENGTH_YESNO ;
			s_format = ft_yesno ;
			break ;
		case 'd':
			s_length = PROPERTY_LENGTH_DATE ;
			s_format = ft_date ;
			break ;
		case 't':
			s_length = PROPERTY_LENGTH_TEMP ;
			s_format = ft_temperature ;
			break ;
		case 'g':
			s_length = PROPERTY_LENGTH_TEMPGAP ;
			s_format = ft_tempgap ;
			break ;
		case 'p':
			s_length = PROPERTY_LENGTH_PRESSURE ;
			s_format = ft_pressure ;
			break ;
		default:
			LEVEL_DEFAULT("Unrecognized variable type <%s> for property <%s> family <%s>",s_dummy,s_property,s_family);
			s_format = ft_unknown ;
			return ;
	}
	if ( s_dummy[1] ) { 
		int temp_length ;
		temp_length = strtol( &s_dummy[1], NULL, 0 ) ;
		if ( temp_length < 1 ) {
			LEVEL_DEFAULT("Unrecognized variable length <%s> for property <%s> family <%s>",s_dummy,s_property,s_family);
			return ;
		}
		s_length = temp_length ;
	}

	// array
	GetQuotedString( dummy ) ;
	switch ( s_dummy[0] ) {
		case '1':
		case '0':
		case '\0':
			s_combined = ag_separate ;
			s_index_type = ag_numbers ;
			s_array = 1 ;
			break ;
		case '-':
			s_array = 1 ;
			switch ( s_dummy[1] ) {
				case '1':
					s_combined = ag_sparse ;
					s_index_type = ag_numbers ;
					break ;
				default:
					s_combined = ag_sparse ;
					s_index_type = ag_letters ;
					break ;
			}
			break ;
		case '+':
			if ( isalpha( (int) s_dummy[1] ) ) {
				s_array = toupper( (int) s_dummy[1] ) - 'A' ;
				s_combined = ag_aggregate ;
				s_index_type = ag_letters ;
			} else {
				s_array = strtol( s_dummy, NULL, 0 ) ;
				s_combined = ag_aggregate ;
				s_index_type = ag_numbers ;
			}
			break ;
		default:
			if ( isalpha( (int) s_dummy[0] ) ) {
				s_array = toupper( (int) s_dummy[0] ) - 'A' ;
				s_combined = ag_separate ;
				s_index_type = ag_letters ;
			} else {
				s_array = strtol( s_dummy, NULL, 0 ) ;
				s_combined = ag_separate ;
				s_index_type = ag_numbers ;
			}
			break ;
	}
	if ( s_array < 1 ) {
		LEVEL_DEFAULT("Unrecognized array type <%s> for property <%s> family <%s>",s_dummy,s_property,s_family);
		return ;
	}

	// persistance
	GetQuotedString( dummy ) ;
	switch ( s_dummy[0] ) {
		case 'f':
			s_change = fc_static ;
			break ;
		case 's':
			s_change = fc_stable ;
			break ;
		case 'v':
			s_change = fc_volatile ;
			break ;
		case 't':
			s_change = fc_second ;
			break ;
		case 'u':
			s_change = fc_uncached ;
			break ;
		default:
			LEVEL_DEFAULT("Unrecognized persistance <%s> for property <%s> family <%s>",s_dummy,s_property,s_family);
			return ;
	}

	// read
	GetQuotedString( read ) ;

	// write
	GetQuotedString( write ) ;

	// data
	GetQuotedString( data ) ;

	// other
	GetQuotedString( other ) ;

	// test minimums
	if ( strlen( s_family ) > 0 && strlen( s_property ) > 0 ) {
		// Actually add
		AddFamilyToTree( s_family ) ;
		AddPropertyToTree( s_property, s_family, s_format, s_array, s_combined, s_index_type, s_length, s_change, s_read, s_write, s_data, s_other, et ) ;
		create_subdirs( s_property, s_family ) ;
	}

	// Clean up
	owfree( s_property ) ;
	owfree( s_family ) ;
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
	char * s_name = NULL ;
	char * s_family = NULL ;
	char * s_description = NULL ;
	char * s_data = NULL ;
	char * start_pointer = input_string ;
	
	if ( input_string == NULL ) {
		return ;
	}
	
	if ( ! Globals.allow_external ) {
		LEVEL_DEBUG("External prgroams not supported by %s",Globals.progname) ;
		return ;
	}
	
	// name
	GetQuotedString( name ) ;

	// family
	GetQuotedString( family ) ;

	// description
	GetQuotedString( description ) ;

	// data
	GetQuotedString( data ) ;

	if ( strlen(s_name) > 0 && strlen(s_family) > 0 ) {
		// Actually add
		AddFamilyToTree( s_family ) ;
		AddSensorToTree( s_name, s_family, s_description, s_data ) ;
		create_just_print( "family", s_family, s_family   ) ;
		create_just_print( "type",   s_family, "external" ) ;
	}

	// Clean up
	owfree( s_name );
	owfree( s_family ) ;
	owfree( s_description ) ;
	owfree( s_data ) ;
}

static struct sensor_node * create_sensor_node( char * s_name, char * s_family, char * s_description, char * s_data )
{
	int l_name = strlen(s_name)+1;
	int l_family = strlen(s_family)+1;
	int l_description = strlen(s_description)+1;
	int l_data = strlen(s_data)+1;

	struct sensor_node * s = owmalloc( sizeof(struct sensor_node) 
	+ l_name +  l_family + l_description + l_data ) ;

	if ( s==NULL) {
		return NULL ;
	}
	
	memset( s, 0, sizeof(struct sensor_node) 
	+ l_name +  l_family + l_description + l_data ) ;

	s->name = s->payload ;
	strcpy( s->name, s_name ) ;

	s->family = s->name + l_name ;
	strcpy( s->family, s_family ) ;

	s->description = s->family + l_family ;
	strcpy( s->description, s_description ) ;

	s->data = s->description + l_description ;
	strcpy( s->data, s_data ) ;

	return s ;
}

static struct family_node * create_family_node( char * s_family )
{
	int l_family = strlen(s_family)+1;

	struct family_node * s = owmalloc( sizeof(struct family_node) 
	+  l_family ) ;

	if ( s==NULL) {
		return NULL ;
	}

	memset( s, 0, sizeof(struct family_node) 
	+  l_family ) ;

	s->family = s->payload ;
	strcpy( s->family, s_family ) ;
	
	// fill some of device fields
	s->dev.count_of_filetypes = 0 ; // until known
	s->dev.family_code = s->family ;
	s->dev.readable_name = s->family ;
	s->dev.filetype_array = NULL ; // until known
	s->dev.flags = 0 ;
	s->dev.g_read = NULL ;
	s->dev.g_write = NULL ;

	return s ;
}

static struct property_node * create_property_node( char * s_property, char * s_family, enum ft_format s_format, size_t s_array, enum ag_combined s_combined, enum ag_index s_index_type, size_t s_length, enum fc_change s_change, char * s_read, char * s_write, char * s_data, char * s_other, enum external_type et )
{
	// AddPropertyToTree (the calling routine) ensures these aren't null
	int l_property = strlen( s_property )+1 ;
	int l_family = strlen( s_family )+1 ;
	int l_read = strlen( s_read )+1 ;
	int l_write = strlen( s_write )+1 ;
	int l_data = strlen( s_data )+1 ;
	int l_other = strlen( s_other )+1 ;
	int l_total = sizeof(struct property_node) + l_property + l_family + l_read + l_write + l_data + l_other ;
	
	struct property_node * s = owmalloc( l_total ) ;
	
	if ( s==NULL) {
		return NULL ;
	}

	memset( s, 0, l_total ) ;

	s->property = s->payload ;
	strcpy( s->property, s_property ) ;

	s->family = s->property + l_property ;
	strcpy( s->family, s_family ) ;

	s->read = s->family + l_family ;
	strcpy( s->read, s_read ) ;

	s->write = s->read + l_read ;
	strcpy( s->write, s_write ) ;

	s->data = s->write + l_write ;
	strcpy( s->data, s_data ) ;

	s->other = s->data + l_data ;
	strcpy( s->other, s_other ) ;
	
	s->et = et ;
	
	// Fill in the filetype structure
	s->ft.name = s->property ;
	s->ft.suglen = s_length ;
	s->ft.ag = &(s->ag) ;
	s->ft.format = s_format ;
	s->ft.change = s_change ;
	s->ft.visible = VISIBLE ;
	s->ft.read = NO_READ_FUNCTION ;
	s->ft.write = NO_WRITE_FUNCTION ;
	s->ft.data.a = s->data ;
	
	// Check read function
	if ( l_read > 1 ) {
		s->ft.read = FS_r_external ;
	}
	
	// Check write function
	if ( l_write > 1 ) {
		s->ft.write = FS_w_external ;
	}
	
	// Fill in aggregate structure
	s->ag.elements = s_array ;
	s->ag.letters = s_index_type ;
	s->ag.combined = s_combined ;

	// Flag scalars
	if ( s_array == 1 ) {
		s->ft.ag = NON_AGGREGATE ;
	}

	return s ;
}

static void AddSensorToTree( char * s_name, char * s_family, char * s_description, char * s_data )
{
	struct sensor_node * n = create_sensor_node( s_name, s_family, s_description, s_data ) ;
	struct {
		struct sensor_node * key ;
		char other[0] ;
	} * opaque = tsearch( (void *) n, &sensor_tree, sensor_compare ) ;
	
	if ( opaque->key != n ) {
		// already exists
		LEVEL_DEBUG("Duplicate sensor entry: %s,%s,%s,%s",s_name,s_family,s_description,s_data);
		owfree( n ) ;
	} else {
		LEVEL_DEBUG("New sensor entry: %s,%s,%s,%s",s_name,s_family,s_description,s_data);
	}
}

static void AddFamilyToTree( char * s_family )
{
	struct family_node * n = create_family_node( s_family ) ;
	struct {
		struct family_node * key ;
		char other[0] ;
	} * opaque = tsearch( (void *) n, &family_tree, family_compare ) ;
	
	if ( opaque->key != n ) {
		// already exists
		LEVEL_DEBUG("Duplicate family entry: %s",s_family);
		owfree( n ) ;
	} else {
		ARG_External(NULL);
		LEVEL_DEBUG("New family entry: %s",s_family);
	}
}

#define NonNull(x)  if(x==NULL) { x=""; }

static void AddPropertyToTree( char * s_property, char * s_family, enum ft_format s_format, size_t s_array, enum ag_combined s_combined, enum ag_index s_index_type, size_t s_length, enum fc_change s_change, char * s_read, char * s_write, char * s_data, char * s_other, enum external_type et )
{
	struct property_node * n  ;
	struct {
		struct property_node * key ;
		char other[0] ;
	} * opaque ;
	
	NonNull( s_property )
	NonNull( s_family )
	NonNull( s_read )
	NonNull( s_write )
	NonNull( s_data )
	NonNull( s_other )

	n = create_property_node( s_property, s_family, s_format, s_array, s_combined, s_index_type, s_length, s_change, s_read, s_write, s_data, s_other, et ) ;
	opaque = tsearch( (void *) n, &property_tree, property_compare ) ;
	
	if ( opaque->key != n ) {
		// already exists
		LEVEL_DEBUG("Duplicate property entry: %s,%s,%s,%s,%s,%s",s_property,s_family,s_read,s_write,s_data,s_other);
		owfree( n ) ;
	} else {
		LEVEL_DEBUG("New property entry: %s,%s,%s,%s,%s,%s",s_property,s_family,s_read,s_write,s_data,s_other);
	}		
}

static void create_just_print( char * s_prop, char * s_family, char * s_data )
{
	AddPropertyToTree( s_prop, s_family, ft_ascii, 1, ag_separate, ag_numbers, strlen(s_data), fc_static, "just_print_data", "", s_data, "", et_internal ) ;
}

static void create_subdirs( char * s_prop, char * s_family )
{
	char * slash ;
	char * subdir = owstrdup( s_prop ) ;
	
	if ( subdir == NULL ) {
		return ;
	}
	while ( (slash = strrchr( subdir, '/' )) != NULL ) {
		slash[0] = '\0' ;
		AddPropertyToTree( subdir, s_family, ft_subdir, 1, ag_separate, ag_numbers, 0, fc_subdir, "", "", "", "", et_none ) ;
	}
	
	owfree(subdir) ;
}
