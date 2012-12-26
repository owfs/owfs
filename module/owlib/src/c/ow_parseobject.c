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

static GOOD_OR_BAD OWQ_allocate_array( struct one_wire_query * owq ) ;
static GOOD_OR_BAD OWQ_parsename(const char *path, struct one_wire_query *owq);
static GOOD_OR_BAD OWQ_parsename_plus(const char *path, const char * file, struct one_wire_query *owq);

#define OWQ_DEFAULT_READ_BUFFER_SIZE  1

/* Create the Parsename structure and create the buffer */
struct one_wire_query * OWQ_create_from_path(const char *path)
{
	int sz = sizeof( struct one_wire_query ) + OWQ_DEFAULT_READ_BUFFER_SIZE;
	struct one_wire_query * owq = owmalloc( sz );
	
	LEVEL_DEBUG("%s", path);

	if ( owq== NO_ONE_WIRE_QUERY) {
		LEVEL_DEBUG("No memory to create object for path %s",path) ;
		return NO_ONE_WIRE_QUERY ;
	}
	
	memset(owq, 0, sz);
	OWQ_cleanup(owq) = owq_cleanup_owq ;
	
	if ( GOOD( OWQ_parsename(path,owq) ) ) {
		if ( GOOD( OWQ_allocate_array(owq)) ) {
			/*   Add a 1 byte buffer by default. This distinguishes from filesystem calls at end of buffer */
			/*   Read bufer is provided by OWQ_assign_read_buffer or OWQ_allocate_read_buffer */
			OWQ_buffer(owq) = (char *) (& owq[1]) ; // point just beyond the one_wire_query struct
			OWQ_size(owq) = OWQ_DEFAULT_READ_BUFFER_SIZE ;
			return owq ;
		}
		OWQ_destroy(owq);
	}
	return NO_ONE_WIRE_QUERY ;
}

/* Create the Parsename structure and load the relevant fields */
struct one_wire_query * OWQ_create_sibling(const char *sibling, struct one_wire_query *owq_original)
{
	char path[PATH_MAX] ;
	struct parsedname * pn_original = PN(owq_original) ;
	int dirlength = pn_original->dirlength ;
	struct one_wire_query * owq_sib ;

	strncpy(path, pn_original->path,dirlength) ;
	strcpy(&path[dirlength],sibling) ;
	
	if ( pn_original->selected_filetype == NO_FILETYPE ) {
		if ( pn_original->subdir == NO_SUBDIR ) {
			// not a filetype or a subdir
			return NO_ONE_WIRE_QUERY ;
		}
	// Add extension only if original property is aggregate
	} else if ( pn_original->selected_filetype->ag != NON_AGGREGATE ) {
		// search for sibling in the filetype array
		struct filetype * sib_filetype = bsearch(sibling, pn_original->selected_device->filetype_array,
				 (size_t) pn_original->selected_device->count_of_filetypes, sizeof(struct filetype), filetype_cmp) ;
		// see if sibling is also an aggregate property
		LEVEL_DEBUG("Path %s is an agggregate",SAFESTRING(pn_original->path));
		if ( sib_filetype != NO_FILETYPE && sib_filetype->ag != NON_AGGREGATE ) {
			char * aggregate_point = path + strlen(path) ;
			LEVEL_DEBUG("Sibling is also an aggregate",sibling);
			if (pn_original->extension == EXTENSION_BYTE ) {
				strcpy( aggregate_point, ".BYTE" ) ;
			} else if (pn_original->extension == EXTENSION_ALL ) {
				strcpy( aggregate_point, ".ALL" ) ;
			} else if (sib_filetype->ag->letters == ag_letters) {
				UCLIBCLOCK;
				snprintf(aggregate_point, OW_FULLNAME_MAX, ".%c", pn_original->extension + 'A');
				UCLIBCUNLOCK;
			} else {
				UCLIBCLOCK;
				snprintf(aggregate_point, OW_FULLNAME_MAX, ".%d", pn_original->extension );
				UCLIBCUNLOCK;
			}
		}			
	}
	
	LEVEL_DEBUG("Create sibling %s from %s as %s", sibling, pn_original->path,path);

	owq_sib = OWQ_create_from_path(path) ;
	if ( owq_sib != NO_ONE_WIRE_QUERY ) {
		// Sib has no offset
		OWQ_offset(owq_sib) = 0 ;
		// Make uncached as restrictive as original
		// Make unaliased as restrictive as original
		PN(owq_sib)->state |= (pn_original->state & (ePS_uncached|ePS_unaliased) ) ;
		return owq_sib ;
	}
	return NO_ONE_WIRE_QUERY ;
}

/* Use an aggregate OWQ as a template for a single element */
struct one_wire_query * OWQ_create_separate( int extension, struct one_wire_query * owq_aggregate )
{
    int sz = sizeof( struct one_wire_query ) + OWQ_DEFAULT_READ_BUFFER_SIZE;
	struct one_wire_query * owq_sep = owmalloc( sz );
	
	LEVEL_DEBUG("%s with extension %d", PN(owq_aggregate)->path,extension);

	if ( owq_sep== NO_ONE_WIRE_QUERY) {
		LEVEL_DEBUG("No memory to create object for extension %d",extension) ;
		return NO_ONE_WIRE_QUERY ;
	}
	
	memset(owq_sep, 0, sz);
	OWQ_cleanup(owq_sep) = owq_cleanup_owq ;
	
	memcpy( PN(owq_sep), PN(owq_aggregate), sizeof(struct parsedname) ) ;
	PN(owq_sep)->extension = extension ;
	OWQ_buffer(owq_sep) = (char *) (& owq_sep[1]) ; // point just beyond the one_wire_query struct
	OWQ_size(owq_sep) = OWQ_DEFAULT_READ_BUFFER_SIZE ;
	OWQ_offset(owq_sep) = 0 ;
	return owq_sep ;
}

/* Use an single OWQ as a template for the aggregate one */
struct one_wire_query * OWQ_create_aggregate( struct one_wire_query * owq_single )
{
    int sz = sizeof( struct one_wire_query ) + OWQ_DEFAULT_READ_BUFFER_SIZE;
	struct one_wire_query * owq_all = owmalloc( sz );
	
	LEVEL_DEBUG("%s with extension ALL", PN(owq_single)->path);

	if ( owq_all == NO_ONE_WIRE_QUERY) {
		LEVEL_DEBUG("No memory to create object for extension ALL") ;
		return NO_ONE_WIRE_QUERY ;
	}
	
	memset(owq_all, 0, sz);
	OWQ_cleanup(owq_all) = owq_cleanup_owq ;
	
	memcpy( PN(owq_all), PN(owq_single), sizeof(struct parsedname) ) ;
	PN(owq_all)->extension = EXTENSION_ALL ;
	OWQ_buffer(owq_all) = (char *) (& owq_all[1]) ; // point just beyond the one_wire_query struct
	OWQ_size(owq_all) = OWQ_DEFAULT_READ_BUFFER_SIZE ;
	OWQ_offset(owq_all) = 0 ;
	if ( BAD( OWQ_allocate_array(owq_all)) ) {
		OWQ_destroy(owq_all);
		return NO_ONE_WIRE_QUERY ;
	}
	return owq_all ;
}

/* Create the Parsename structure and load the relevant fields */
GOOD_OR_BAD OWQ_create(const char *path, struct one_wire_query *owq)
{
	LEVEL_DEBUG("%s", path);

	if ( GOOD( OWQ_parsename(path,owq) ) ) {
		if ( GOOD( OWQ_allocate_array(owq)) ) {
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
		if ( GOOD( OWQ_allocate_array(owq)) ) {
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

static GOOD_OR_BAD OWQ_allocate_array( struct one_wire_query * owq )
{
	struct parsedname * pn = PN(owq) ;
	if (pn->extension == EXTENSION_ALL && pn->type != ePN_structure) {
		OWQ_array(owq) = owcalloc((size_t) pn->selected_filetype->ag->elements, sizeof(union value_object));
		if (OWQ_array(owq) == NO_ONE_WIRE_QUERY) {
			return gbBAD ;
		}
		OWQ_cleanup(owq) |= owq_cleanup_array ;
	} else {
		OWQ_I(owq) = 0;
	}
	return gbGOOD ;
}

void OWQ_assign_read_buffer(char *buffer, size_t size, off_t offset, struct one_wire_query *owq)
{
	OWQ_buffer(owq) = buffer;
	OWQ_size(owq) = size;
	OWQ_offset(owq) = offset;
}

void OWQ_assign_write_buffer(const char *buffer, size_t size, off_t offset, struct one_wire_query *owq)
{
	// OWQ_buffer used for both read (non-const) and write (const)
#if ( __GNUC__ > 4 ) || (__GNUC__ == 4 && __GNUC_MINOR__ > 4 )
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-qual"
	OWQ_buffer(owq) = (char *) buffer;
#pragma GCC diagnostic pop
#else
	OWQ_buffer(owq) = (char *) buffer;
#endif
	OWQ_size(owq) = size;
	OWQ_offset(owq) = offset;
}

// create the buffer of size filesize
GOOD_OR_BAD OWQ_allocate_read_buffer(struct one_wire_query * owq )
{
	struct parsedname * pn = PN(owq) ;
	size_t size = FullFileLength(pn);

	if ( size > 0 ) {
		char * buffer = owmalloc(size+1) ;
		if ( buffer == NULL ) {
			return gbBAD ;
		}
		memset(buffer,0,size+1) ;
		OWQ_buffer(owq) = buffer ;
		OWQ_size(owq) = size ;
		OWQ_offset(owq) = 0 ;
		OWQ_cleanup(owq) |= owq_cleanup_buffer ;
	}
	return gbGOOD;
}

GOOD_OR_BAD OWQ_allocate_write_buffer( const char * write_buffer, size_t buffer_length, off_t offset, struct one_wire_query * owq )
{
	char * buffer_copy ;
	
	if ( buffer_length == 0 ) {
		// Buffer size is zero. Allowed, but make it NULL and no cleanup needed.
		OWQ_size(owq) = 0 ;
		OWQ_offset(owq) = 0 ;
		return gbGOOD ;
	}
	
	buffer_copy = owmalloc( buffer_length+1) ;
	if ( buffer_copy == NULL) {
		// cannot allocate space for buffer
		LEVEL_DEBUG("Cannot allocate %ld bytes for buffer", buffer_length) ;
		OWQ_size(owq) = 0 ;
		OWQ_offset(owq) = 0 ;
		return gbBAD ;
	}
	
	memcpy( buffer_copy, write_buffer, buffer_length) ;
	buffer_copy[buffer_length] = '\0' ; // make sure it's zero-ended
	OWQ_buffer(owq) = buffer_copy ;
	OWQ_size(owq)   = buffer_length ;
	OWQ_length(owq) = buffer_length ;
	OWQ_offset(owq) = offset ;
	OWQ_cleanup(owq) |= owq_cleanup_buffer ; // buffer needs cleanup
	return gbGOOD ;
}

void OWQ_destroy(struct one_wire_query *owq)
{
	if ( owq == NO_ONE_WIRE_QUERY) {
		return ;
	}
	
	if ( OWQ_cleanup(owq) & owq_cleanup_buffer ) {
		owfree(OWQ_buffer(owq)) ;
	}
	
	if ( OWQ_cleanup(owq) & owq_cleanup_rbuffer ) {
		//owfree(OWQ_read_buffer(owq)) ;
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
