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
#include "ow_counters.h"
#include "ow_connection.h"

static int OWQ_allocate_array( struct one_wire_query * owq ) ;
static GOOD_OR_BAD OWQ_parsename(const char *path, struct one_wire_query *owq);
static GOOD_OR_BAD OWQ_parsename_plus(const char *path, const char * file, struct one_wire_query *owq);

#define OWQ_DEFAULT_READ_BUFFER_SIZE  1

/* Create the Parsename structure and create the buffer */
struct one_wire_query * OWQ_create_from_path(const char *path)
{
	struct one_wire_query * owq = owmalloc( sizeof( struct one_wire_query ) + OWQ_DEFAULT_READ_BUFFER_SIZE ) ;
	
	LEVEL_DEBUG("%s", path);

	if ( owq== NULL) {
		LEVEL_DEBUG("No memory to create object for path %s",path) ;
		return NULL ;
	}
	
	memset(owq, 0, sizeof(owq));
	OWQ_cleanup(owq) = owq_cleanup_owq ;
	
	if ( GOOD( OWQ_parsename(path,owq) ) ) {
		if ( OWQ_allocate_array(owq) == 0 ) {
			/*   Add a 1 byte buffer by default. This distinguishes from filesystem calls at end of buffer */
			/*   Read bufer is provided by OWQ_assign_read_buffer or OWQ_allocate_read_buffer */
			OWQ_buffer(owq) = (char *) (& owq[1]) ; // point just beyond the one_wire_query struct
			OWQ_size(owq) = OWQ_DEFAULT_READ_BUFFER_SIZE ;
			return owq ;
		}
		OWQ_destroy(owq);
	}
	return NULL ;
}

/* Create the Parsename structure and load the relevant fields */
struct one_wire_query * OWQ_create_sibling(const char *sibling, struct one_wire_query *owq_original)
{
	char path[PATH_MAX] ;
	int dirlength = PN(owq_original)->dirlength ;
	struct one_wire_query * owq_sib ;

	strncpy(path,PN(owq_original)->path,dirlength) ;
	strcpy(&path[dirlength],sibling) ;

	LEVEL_DEBUG("Create sibling %s from %s as %s", sibling,PN(owq_original)->path,path);

	owq_sib = OWQ_create_from_path(path) ;
	if ( owq_sib != NULL ) {
		OWQ_offset(owq_sib) = 0 ;
		return owq_sib ;
	}
	return NULL ;
}

/* Use an aggregate OWQ as a template for a single element */
struct one_wire_query * OWQ_create_separate( int extension, struct one_wire_query * owq_aggregate )
{
	struct one_wire_query * owq_sep = owmalloc( sizeof( struct one_wire_query ) + OWQ_DEFAULT_READ_BUFFER_SIZE ) ;
	
	LEVEL_DEBUG("%s with extension %d", PN(owq_aggregate)->path,extension);

	if ( owq_sep== NULL) {
		LEVEL_DEBUG("No memory to create object for extension %d",extension) ;
		return NULL ;
	}
	
	memset(owq_sep, 0, sizeof(owq_sep));
	OWQ_cleanup(owq_sep) = owq_cleanup_owq ;
	
	memcpy( PN(owq_sep), PN(owq_aggregate), sizeof(struct parsedname) ) ;
	PN(owq_sep)->extension = extension ;
	OWQ_buffer(owq_sep) = (char *) (& owq_sep[1]) ; // point just beyond the one_wire_query struct
	OWQ_size(owq_sep) = OWQ_DEFAULT_READ_BUFFER_SIZE ;
	OWQ_offset(owq_sep) = 0 ;
	return owq_sep ;
}

/* Create the Parsename structure and load the relevant fields */
GOOD_OR_BAD OWQ_create(const char *path, struct one_wire_query *owq)
{
	LEVEL_DEBUG("%s", path);

	if ( GOOD( OWQ_parsename(path,owq) ) ) {
		if ( OWQ_allocate_array(owq) == 0 ) {
			return gbGOOD ;
		}
		OWQ_destroy(owq);
	}
	return gbBAD ;
}

/* Create the Parsename structure (using path and file) and load the relevant fields */
/* Starts with a statically allocated owq space */
GOOD_OR_BAD OWQ_create_plus(const char *path, const char *file, struct one_wire_query *owq)
{
	LEVEL_DEBUG("%s + %s", path, file);

	OWQ_cleanup(owq) = owq_cleanup_none ;
	if ( GOOD( OWQ_parsename_plus(path,file,owq) ) ) {
		if ( OWQ_allocate_array(owq) == 0 ) {
			return gbGOOD ;
		}
		OWQ_destroy(owq);
	}
	return gbBAD ;
}

/* Create the Parsename structure in owq */
static GOOD_OR_BAD OWQ_parsename(const char *path, struct one_wire_query *owq)
{
	struct parsedname *pn = PN(owq);

	if ( FS_ParsedName(path, pn) != 0 ) {
		return gbBAD ;
	}
	OWQ_cleanup(owq) |= owq_cleanup_pn ;
	return gbGOOD ;
}

static GOOD_OR_BAD OWQ_parsename_plus(const char *path, const char * file, struct one_wire_query *owq)
{
	struct parsedname *pn = PN(owq);

	if ( FS_ParsedNamePlus(path, file, pn) != 0 ) {
		return gbBAD ;
	}
	OWQ_cleanup(owq) |= owq_cleanup_pn ;
	return gbGOOD ;
}

static int OWQ_allocate_array( struct one_wire_query * owq )
{
	struct parsedname * pn = PN(owq) ;
	if (pn->extension == EXTENSION_ALL && pn->type != ePN_structure) {
		OWQ_array(owq) = owcalloc((size_t) pn->selected_filetype->ag->elements, sizeof(union value_object));
		if (OWQ_array(owq) == NULL) {
			return 1 ;
		}
		OWQ_cleanup(owq) |= owq_cleanup_array ;
	} else {
		OWQ_I(owq) = 0;
	}
	return 0 ;
}

void OWQ_assign_read_buffer(char *buffer, size_t size, off_t offset, struct one_wire_query *owq)
{
	OWQ_buffer(owq) = buffer;
	OWQ_read_buffer(owq) = NULL ;
	OWQ_write_buffer(owq) = NULL ;
	OWQ_size(owq) = size;
	OWQ_offset(owq) = offset;
}

void OWQ_assign_write_buffer(const char *buffer, size_t size, off_t offset, struct one_wire_query *owq)
{
	OWQ_buffer(owq) = buffer;
	OWQ_read_buffer(owq) = NULL ;
	OWQ_write_buffer(owq) = NULL ;
	OWQ_size(owq) = size;
	OWQ_offset(owq) = offset;
}

// create the buffer of size filesize
int OWQ_allocate_read_buffer(struct one_wire_query * owq )
{
	struct parsedname * pn = PN(owq) ;
	size_t size = FullFileLength(pn);

	if ( size > 0 ) {
		char * buffer = owmalloc(size+1) ;
		if ( buffer == NULL ) {
			return 1 ;
		}
		memset(buffer,0,size+1) ;
		OWQ_buffer(owq) = buffer ;
		OWQ_size(owq) = size ;
		OWQ_offset(owq) = 0 ;
		OWQ_cleanup(owq) |= owq_cleanup_buffer ;
	}
	return 0;
}

int OWQ_allocate_write_buffer( const char * write_buffer, size_t buffer_length, struct one_wire_query * owq )
{
	char * buffer_copy ;
	
	if ( buffer_length == 0 ) {
		// Buffer size is zero. Allowed, but make it NULL and no cleanup needed.
		OWQ_buffer(owq) = NULL ;
		OWQ_size(owq) = 0 ;
		return 0 ;
	}
	
	buffer_copy = owmalloc( buffer_length) ;
	if ( buffer_copy == NULL) {
		// cannot allocate space for buffer
		LEVEL_DEBUG("Cannot allocate %ld bytes for buffer", buffer_length) ;
		OWQ_buffer(owq) = NULL ;
		OWQ_size(owq) = 0 ;
		return 1 ;
	}
	
	memcpy( buffer_copy, write_buffer, buffer_length) ;
	OWQ_buffer(owq) = buffer_copy ;
	OWQ_length(owq) = buffer_length ;
	OWQ_cleanup(owq) |= owq_cleanup_buffer ;
	return 0 ;
}

void OWQ_destroy(struct one_wire_query *owq)
{
	if ( owq == NULL) {
		return ;
	}
	
	if ( OWQ_cleanup(owq) & owq_cleanup_buffer ) {
		owfree(OWQ_buffer(owq)) ;
	}
	
	if ( OWQ_cleanup(owq) & owq_cleanup_rbuffer ) {
		owfree(OWQ_read_buffer(owq)) ;
	}
	
	if ( OWQ_cleanup(owq) & owq_cleanup_array ) {
		owfree(OWQ_array(owq)) ;
	}
	
	if ( OWQ_cleanup(owq) & owq_cleanup_pn ) {
		FS_ParsedName_destroy(PN(owq)) ;
	}

	if ( OWQ_cleanup(owq) & owq_cleanup_owq ) {
		owfree(owq) ;
	} else {
		OWQ_cleanup(owq) = owq_cleanup_none ;
	}
}
