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

	LEVEL_DEBUG("%s\n", path);

	return_code = FS_ParsedName(path, pn) ;
	if ( return_code == 0 ) {
		return_code = FS_OWQ_create_postparse( buffer, size, offset, pn, owq ) ;
		if ( return_code == 0 ) {
			return 0 ;
		}
		FS_ParsedName_destroy(pn);
	}
	return return_code ;
}

/* Create the Parsename structure and load the relevant fields */
/* Clean up with FS_OWQ_destroy */
struct one_wire_query *FS_OWQ_create_sibling(const char *sibling, struct one_wire_query *owq_original)
{
	char path[PATH_MAX] ;
	struct parsedname s_pn ;
	int dirlength = PN(owq_original)->dirlength ;

	strncpy(path,PN(owq_original)->path,dirlength) ;
	strcpy(&path[dirlength],sibling) ;

	LEVEL_DEBUG("sibling %s\n", sibling);

	if ( FS_ParsedName( path, &s_pn ) == 0 ) {
		struct one_wire_query *owq = FS_OWQ_from_pn( &s_pn ) ;
		if ( owq != NULL ) {
			return owq ;
		}
		FS_ParsedName_destroy( &s_pn );
	}
	return NULL ;
}

static int FS_OWQ_create_postparse(char *buffer, size_t size, off_t offset, const struct parsedname * pn, struct one_wire_query *owq)
{
	OWQ_buffer(owq) = buffer;
	OWQ_size(owq) = size;
	OWQ_offset(owq) = offset;
	if ( PN(owq) != pn ) {
		memcpy(PN(owq), pn, sizeof(struct parsedname));
	}
	if (pn->extension == EXTENSION_ALL && pn->type != ePN_structure) {
		OWQ_array(owq) = owcalloc((size_t) pn->selected_filetype->ag->elements, sizeof(union value_object));
		if (OWQ_array(owq) == NULL) {
			return -ENOMEM;
		}
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
	if (owq != NULL) {
//		if (Globals.error_level>=e_err_debug) { // keep valgrind happy
//			memset(buffer, 0, size);
//		}
		if ( FS_OWQ_create_postparse( &((char *)owq)[sizeof(struct one_wire_query)], size, 0, pn, owq ) == 0 ) {
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

	LEVEL_DEBUG("%s + %s\n", path, file);

	return_code = FS_ParsedNamePlus(path, file, pn) ;
	if ( return_code == 0 ) {
		return_code = FS_OWQ_create_postparse( buffer, size, offset, pn, owq ) ;
		if ( return_code == 0 ) {
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

	LEVEL_DEBUG("%s\n", pn->path);

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
	struct parsedname *pn = PN(owq);

	LEVEL_DEBUG("%s\n", pn->path);
	if (pn->extension == EXTENSION_ALL && pn->type != ePN_structure) {
		if (OWQ_array(owq)) {
			owfree(OWQ_array(owq));
			OWQ_array(owq) = NULL;
		}
	}
	FS_ParsedName_destroy(pn);
}
