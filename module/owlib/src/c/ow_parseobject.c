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

	LEVEL_DEBUG("FS_OWQ_create of %s\n", path);
	
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

static int FS_OWQ_create_postparse(char *buffer, size_t size, off_t offset, const struct parsedname * pn, struct one_wire_query *owq)
{
	OWQ_buffer(owq) = buffer;
	OWQ_size(owq) = size;
	OWQ_offset(owq) = offset;
	memcpy(PN(owq), pn, sizeof(struct parsedname));
	if (pn->extension == EXTENSION_ALL && pn->type != ePN_structure) {
		OWQ_array(owq) = calloc((size_t) pn->selected_filetype->ag->elements, sizeof(union value_object));
		if (OWQ_array(owq) == NULL) {
			return -ENOMEM;
		}
	} else {
		OWQ_I(owq) = 0;
	}
	return 0;
}

struct one_wire_query *FS_OWQ_from_pn(const struct parsedname *pn)
{
	size_t size = FullFileLength(pn);
	char *buffer = malloc(size);

	if (buffer != NULL) {
		struct one_wire_query *owq = malloc(sizeof(struct one_wire_query));
		if (owq != NULL) {
			if ( FS_OWQ_create_postparse( buffer, size, 0, pn, owq ) == 0 ) {
				return owq ;
			}
			free(owq);
		}
		free(buffer);
	}
	return NULL;
}

void FS_OWQ_from_pn_destroy(struct one_wire_query *owq)
{
	struct parsedname *pn = PN(owq);

	LEVEL_DEBUG("FS_OWQ_from_pn_destroy of %s\n", pn->path);
	
	if (pn->extension == EXTENSION_ALL && pn->type != ePN_structure) {
		if (OWQ_array(owq)) {
			free(OWQ_array(owq));
			OWQ_array(owq) = NULL;
		}
	}
	if ( OWQ_buffer(owq) != NULL ) {
		free( OWQ_buffer(owq) ) ;
	}
	if ( owq ) {
		free(owq) ;
	}
}

/* Create the Parsename structure (using path and file) and load the relevant fields */
int FS_OWQ_create_plus(const char *path, const char *file, char *buffer, size_t size, off_t offset, struct one_wire_query *owq)
{
	struct parsedname *pn = PN(owq);
	int return_code ;

	LEVEL_DEBUG("FS_OWQ_create_plus of %s + %s\n", path, file);

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

void FS_OWQ_destroy(struct one_wire_query *owq)
{
	struct parsedname *pn = PN(owq);

	LEVEL_DEBUG("FS_OWQ_destroy of %s\n", pn->path);
	if (pn->extension == EXTENSION_ALL && pn->type != ePN_structure) {
		if (OWQ_array(owq)) {
			free(OWQ_array(owq));
			OWQ_array(owq) = NULL;
		}
	}
	FS_ParsedName_destroy(pn);
}
