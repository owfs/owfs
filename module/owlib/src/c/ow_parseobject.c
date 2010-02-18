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

static int FS_OWQ_create_postparse(char *buffer, size_t size, off_t offset, const struct parsedname * pn, struct one_wire_query *owq) ;
static int FS_OWQ_allocate_array( struct one_wire_query * owq ) ;
static int FS_OWQ_allocate_read_buffer(struct one_wire_query * owq ) ;
static void FS_OWQ_assign_read_buffer(char *buffer, size_t size, off_t offset, struct one_wire_query *owq) ;
static void FS_OWQ_assign_write_buffer(const char *buffer, size_t size, off_t offset, struct one_wire_query *owq) ;
static int FS_OWQ_parsename(const char *path, struct one_wire_query *owq);
static int FS_OWQ_parsename_plus(const char *path, const char * file, struct one_wire_query *owq);

/* Create the Parsename structure and create the buffer */
struct one_wire_query * FS_OWQ_create_from_path(const char *path, size_t size)
{
	struct one_wire_query * owq = owmalloc( sizeof( struct one_wire_query ) ) ;
	
	(void) size ;
	LEVEL_DEBUG("%s", path);

	if ( owq== NULL) {
		return NULL ;
	}
	
	OWQ_cleanup(owq) = owq_cleanup_owq ;
	if ( FS_OWQ_parsename(path,owq) == 0 ) {
		if ( FS_OWQ_allocate_array(owq) == 0 ) {
			if ( FS_OWQ_allocate_read_buffer(owq) == 0 ) {
				return owq ;
			}
		}
		FS_OWQ_destroy(owq);
	}
	return NULL ;
}

/* Create the Parsename structure and load the relevant fields */
struct one_wire_query *FS_OWQ_create_sibling(const char *sibling, struct one_wire_query *owq_original)
{
	char path[PATH_MAX] ;
	int dirlength = PN(owq_original)->dirlength ;

	strncpy(path,PN(owq_original)->path,dirlength) ;
	strcpy(&path[dirlength],sibling) ;

	LEVEL_DEBUG("sibling %s", sibling);

	return FS_OWQ_create_from_path(path,0) ;
}


// Creates an owq from pn, making the buffer be filesize and part of the allocation
struct one_wire_query *FS_OWQ_from_pn(const struct parsedname *pn)
{
	return FS_OWQ_create_from_path(pn->path,0) ;
}

/* Create the Parsename structure and load the relevant fields */
int FS_OWQ_create(const char *path, char *buffer, size_t size, off_t offset, struct one_wire_query *owq)
{
	LEVEL_DEBUG("%s", path);

	if ( FS_OWQ_parsename(path,owq) == 0 ) {
		if ( FS_OWQ_allocate_array(owq) == 0 ) {
			FS_OWQ_assign_read_buffer(buffer, size,offset,owq) ;
			return 0 ;
		}
		FS_OWQ_destroy(owq);
	}
	return 1 ;
}

/* Create the Parsename structure (using path and file) and load the relevant fields */
/* Starts with a statically allocated owq space */
int FS_OWQ_create_plus(const char *path, const char *file, char *buffer, size_t size, off_t offset, struct one_wire_query *owq)
{
	LEVEL_DEBUG("%s + %s", path, file);

	if ( FS_OWQ_parsename_plus(path,file,owq) == 0 ) {
		if ( FS_OWQ_allocate_array(owq) == 0 ) {
			FS_OWQ_assign_read_buffer(buffer, size,offset,owq) ;
			return 0 ;
		}
		FS_OWQ_destroy(owq);
	}
	return 1 ;
}

/* Create the Parsename structure in owq */
static int FS_OWQ_parsename(const char *path, struct one_wire_query *owq)
{
	struct parsedname *pn = PN(owq);

	if ( FS_ParsedName(path, pn) != 0 ) {
		return 1 ;
	}
	OWQ_cleanup(owq) |= owq_cleanup_pn ;
	return 0 ;
}

static int FS_OWQ_parsename_plus(const char *path, const char * file, struct one_wire_query *owq)
{
	struct parsedname *pn = PN(owq);

	if ( FS_ParsedNamePlus(path, file, pn) != 0 ) {
		return 1 ;
	}
	OWQ_cleanup(owq) |= owq_cleanup_pn ;
	return 0 ;
}

static int FS_OWQ_allocate_array( struct one_wire_query * owq )
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

/* Create the Parsename structure and create the buffer */
struct one_wire_query * FS_OWQ_create_read_from_path(const char *path)
{
	return FS_OWQ_create_from_path(path,0) ;
}

static int FS_OWQ_create_postparse(char *buffer, size_t size, off_t offset, const struct parsedname * pn, struct one_wire_query *owq)
{
	OWQ_buffer(owq) = buffer;
	OWQ_read_buffer(owq) = NULL ;
	OWQ_write_buffer(owq) = NULL ;
	OWQ_size(owq) = size;
	OWQ_offset(owq) = offset;
	OWQ_cleanup(owq) |= owq_cleanup_buffer ;

	if ( PN(owq) != pn ) {
		memcpy(PN(owq), pn, sizeof(struct parsedname));
	}
	if (pn->extension == EXTENSION_ALL && pn->type != ePN_structure) {
		OWQ_array(owq) = owcalloc((size_t) pn->selected_filetype->ag->elements, sizeof(union value_object));
		if (OWQ_array(owq) == NULL) {
			return -ENOMEM;
		}
		OWQ_cleanup(owq) |= owq_cleanup_array ;
	} else {
		OWQ_I(owq) = 0;
	}
	return 0;
}

static void FS_OWQ_assign_read_buffer(char *buffer, size_t size, off_t offset, struct one_wire_query *owq)
{
	OWQ_buffer(owq) = buffer;
	OWQ_read_buffer(owq) = NULL ;
	OWQ_write_buffer(owq) = NULL ;
	OWQ_size(owq) = size;
	OWQ_offset(owq) = offset;
}

static void FS_OWQ_assign_write_buffer(const char *buffer, size_t size, off_t offset, struct one_wire_query *owq)
{
	OWQ_buffer(owq) = buffer;
	OWQ_read_buffer(owq) = NULL ;
	OWQ_write_buffer(owq) = NULL ;
	OWQ_size(owq) = size;
	OWQ_offset(owq) = offset;
}

// create the buffer of size filesize
static int FS_OWQ_allocate_read_buffer(struct one_wire_query * owq )
{
	struct parsedname * pn = PN(owq) ;
	size_t size = FullFileLength(pn);

	if ( size > 0 ) {
		char * buffer = owmalloc(size) ;
		if ( buffer == NULL ) {
			return 1 ;
		}
		OWQ_buffer(owq) = buffer ;
		OWQ_size(owq) = size ;
		OWQ_offset(owq) = 0 ;
		OWQ_cleanup(owq) |= owq_cleanup_buffer ;
	}
	return 0;
}

void FS_OWQ_destroy(struct one_wire_query *owq)
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
