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

/* Create the Parsename structure and load the relevant fields */
int FS_OWQ_create(const char *path, char *buffer, size_t size, off_t offset, struct one_wire_query *owq)
{
	struct parsedname *pn = PN(owq);
	int return_code ;

	LEVEL_DEBUG("%s", path);

	return_code = FS_ParsedName(path, pn) ;
	if ( return_code == 0 ) {
		return_code = FS_OWQ_create_postparse( buffer, size, offset, pn, owq ) ;
		if ( return_code == 0 ) {
			OWQ_cleanup(owq) |= owq_cleanup_pn ;
			return 0 ;
		}
		FS_ParsedName_destroy(pn);
	}
	return return_code ;
}

/* Create the Parsename structure and create the buffer */
/* Destroy with FS_OWQ_destroy_sibling */
struct one_wire_query * FS_OWQ_create_from_path(const char *path, size_t size)
{
	struct one_wire_query * owq ;
	struct parsedname * pn ;
	
	LEVEL_DEBUG("%s", path);

	owq = owmalloc( sizeof( struct one_wire_query ) + size ) ;
	OWQ_cleanup(owq) = owq_cleanup_owq ;
	if ( owq == NULL ) {
		return NULL ;
	}
	pn = PN(owq) ; // for convenience
	if ( FS_ParsedName( path, pn ) ) {
		owfree(owq) ;
		return NULL ;
	}
	OWQ_cleanup(owq) |= owq_cleanup_pn ;
	if (pn->extension == EXTENSION_ALL && pn->type != ePN_structure) {
		OWQ_array(owq) = owcalloc((size_t) pn->selected_filetype->ag->elements, sizeof(union value_object));
		if (OWQ_array(owq) == NULL) {
			FS_ParsedName_destroy(pn) ;
			owfree(owq) ;
			return NULL ;
		}
		OWQ_cleanup(owq) |= owq_cleanup_array ;
	} else {
		OWQ_I(owq) = 0;
	}

	OWQ_buffer(owq) = (char*)&owq[1] ;
	OWQ_read_buffer(owq) = NULL ;
	OWQ_write_buffer(owq) = NULL ;
	OWQ_size(owq) = size ;
	OWQ_offset(owq) = 0 ;
	return owq ;
}

/* Create the Parsename structure and load the relevant fields */
/* Clean up with FS_OWQ_destroy_sibling */
struct one_wire_query *FS_OWQ_create_sibling(const char *sibling, struct one_wire_query *owq_original)
{
	char path[PATH_MAX] ;
	struct parsedname s_pn ;
	int dirlength = PN(owq_original)->dirlength ;

	strncpy(path,PN(owq_original)->path,dirlength) ;
	strcpy(&path[dirlength],sibling) ;

	LEVEL_DEBUG("sibling %s", sibling);

	if ( FS_ParsedName( path, &s_pn ) == 0 ) {
		struct one_wire_query *owq = FS_OWQ_from_pn( &s_pn ) ;
		if ( owq != NULL ) {
			OWQ_cleanup(owq) |= owq_cleanup_pn ;
			return owq ;
		}
		FS_ParsedName_destroy( &s_pn );
	}
	return NULL ;
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

// Creates an owq from pn, making the buffer be filesize and part of the allocation
// Destroy with FS_OWQ_destroy_not_pn
struct one_wire_query *FS_OWQ_from_pn(const struct parsedname *pn)
{
	size_t size = FullFileLength(pn);
	struct one_wire_query *owq = owmalloc(sizeof(struct one_wire_query)+size); // owq + buffer
	OWQ_cleanup(owq) = owq_cleanup_owq ;
	if (owq != NULL) {
//		if (Globals.error_level>=e_err_debug) { // keep valgrind happy
//			memset(buffer, 0, size);
//		}
		if ( FS_OWQ_create_postparse( &((char *)owq)[sizeof(struct one_wire_query)], size, 0, pn, owq ) == 0 ) {
			// buffer was actually allocated with owq, so don't free it independently
			OWQ_cleanup(owq) ^= owq_cleanup_buffer ;
			return owq ;
		}
		owfree(owq);
	}
	return NULL;
}

/* Create the Parsename structure (using path and file) and load the relevant fields */
int FS_OWQ_create_plus(const char *path, const char *file, char *buffer, size_t size, off_t offset, struct one_wire_query *owq)
{
	struct parsedname *pn = PN(owq);
	int return_code ;

	LEVEL_DEBUG("%s + %s", path, file);

	return_code = FS_ParsedNamePlus(path, file, pn) ;
	if ( return_code == 0 ) {
		return_code = FS_OWQ_create_postparse( buffer, size, offset, pn, owq ) ;
		if ( return_code == 0 ) {
			OWQ_cleanup(owq) |= owq_cleanup_pn ;
			return 0 ;
		}
		FS_ParsedName_destroy(pn);
	}
	return return_code ;
}

// Safe to pass a NULL
void FS_OWQ_destroy_not_pn(struct one_wire_query *owq)
{
	struct parsedname *pn = PN(owq);

	LEVEL_DEBUG("%s", pn->path);

	if (pn->extension == EXTENSION_ALL && pn->type != ePN_structure) {
		if (OWQ_array(owq)) {
			owfree(OWQ_array(owq));
			OWQ_array(owq) = NULL;
		}
	}
//	if ( OWQ_buffer(owq) != NULL ) {
//		owfree( OWQ_buffer(owq) ) ;
//	}
	owfree(owq) ;
}

void FS_OWQ_destroy(struct one_wire_query *owq)
{
	if ( owq == NULL) {
		return ;
	}
	
	if ( OWQ_cleanup(owq) | owq_cleanup_buffer ) {
		owfree(OWQ_buffer(owq)) ;
	}
	
	if ( OWQ_cleanup(owq) | owq_cleanup_rbuffer ) {
		owfree(OWQ_read_buffer(owq)) ;
	}
	
	if ( OWQ_cleanup(owq) | owq_cleanup_array ) {
		owfree(OWQ_array(owq)) ;
	}
	
	if ( OWQ_cleanup(owq) | owq_cleanup_pn ) {
		FS_ParsedName_destroy(PN(owq)) ;
	}

	if ( OWQ_cleanup(owq) | owq_cleanup_owq ) {
		owfree(owq) ;
	} else {
		OWQ_cleanup(owq) = owq_cleanup_none ;
	}
}

void FS_OWQ_destroy_sibling(struct one_wire_query *owq)
{
	FS_OWQ_destroy(owq) ;
	owfree(owq) ;
}
