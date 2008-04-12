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

/* Create the Parsename structure and load the relevant fields */
int FS_OWQ_create(const char *path, char *buffer, size_t size, off_t offset, struct one_wire_query *owq)
{
	struct parsedname *pn = PN(owq);

	LEVEL_DEBUG("FS_OWQ_create of %s\n", path);
    if (FS_ParsedName(path, pn)) {
		return -ENOENT;
    }
    
	OWQ_buffer(owq) = buffer;
	OWQ_size(owq) = size;
	OWQ_offset(owq) = offset;
	if (pn->extension == EXTENSION_ALL && pn->type != ePN_structure) {
		OWQ_array(owq) = calloc((size_t) pn->selected_filetype->ag->elements, sizeof(union value_object));
		if (OWQ_array(owq) == NULL) {
			FS_ParsedName_destroy(pn);
			return -ENOMEM;
		}
	} else {
		OWQ_I(owq) = 0;
	}
	return 0;
}

struct one_wire_query * FS_OWQ_from_pn( const struct parsedname * pn )
{
    struct one_wire_query * owq ;
    size_t size = FullFileLength( pn ) ;
    char * buffer = malloc( size ) ;

    if ( buffer != NULL ) {
        owq = malloc ( sizeof(struct one_wire_query) ) ;
        if ( owq != NULL ) {
            memcpy( PN(owq), pn, sizeof(struct parsedname) ) ;
            OWQ_buffer(owq) = buffer ;
            OWQ_size(owq) = size ;
            OWQ_offset(owq) = 0 ;
            if (pn->extension == EXTENSION_ALL && pn->type != ePN_structure) {
                OWQ_array(owq) = calloc((size_t) pn->selected_filetype->ag->elements, sizeof(union value_object));
                if (OWQ_array(owq) != NULL) {
                    return owq ;
                }
            } else {
                OWQ_I(owq) = 0;
                return owq ;
            }
            free(owq) ;
        }
        free(buffer) ;
    }
    return NULL ;
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
}

/* Create the Parsename structure (using path and file) and load the relevant fields */
int FS_OWQ_create_plus(const char *path, const char *file, char *buffer, size_t size, off_t offset, struct one_wire_query *owq)
{
	struct parsedname *pn = PN(owq);

	LEVEL_DEBUG("FS_OWQ_create_plus of %s + %s\n", path, file);
	if (FS_ParsedNamePlus(path, file, pn))
		return -ENOENT;
	OWQ_buffer(owq) = buffer;
	OWQ_size(owq) = size;
	OWQ_offset(owq) = offset;
	if (pn->extension == EXTENSION_ALL && pn->type != ePN_structure) {
		OWQ_array(owq) = calloc((size_t) pn->selected_filetype->ag->elements, sizeof(union value_object));
		if (OWQ_array(owq) == NULL) {
			FS_ParsedName_destroy(pn);
			return -ENOMEM;
		}
	} else {
		OWQ_I(owq) = 0;
	}
	return 0;
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

/* make a "shallow" copy -- but possibly full array size or just an element size */
/* if full array size, allocate extra space for value_object and possible buffer */
void OWQ_create_shallow_single(struct one_wire_query *owq_shallow, struct one_wire_query *owq_original)
{
	memcpy(owq_shallow, owq_original, sizeof(struct one_wire_query));
}

/* make a "shallow" copy -- but possibly full array size or just an element size */
/* if full array size, allocate extra space for value_object and possible buffer */
int OWQ_create_shallow_aggregate(struct one_wire_query *owq_shallow, struct one_wire_query *owq_original)
{
	struct parsedname *pn = PN(owq_original);

	memcpy(owq_shallow, owq_original, sizeof(struct one_wire_query));

	/* allocate new value_object array space */
	OWQ_array(owq_shallow) = calloc((size_t) pn->selected_filetype->ag->elements, sizeof(union value_object));
	if (OWQ_array(owq_shallow) == NULL)
		return -ENOMEM;
	if (pn->extension == EXTENSION_ALL) {
		memcpy(OWQ_array(owq_shallow), OWQ_array(owq_original), pn->selected_filetype->ag->elements * sizeof(union value_object));
	}

	OWQ_offset(owq_shallow) = 0;
	OWQ_pn(owq_shallow).extension = EXTENSION_ALL;

	switch (pn->selected_filetype->format) {
	case ft_binary:
	case ft_ascii:
	case ft_vascii:
		/* allocate larger buffer space */
		OWQ_size(owq_shallow) = FullFileLength(PN(owq_shallow));
		OWQ_buffer(owq_shallow) = malloc(OWQ_size(owq_shallow));
		if (OWQ_buffer(owq_shallow) == NULL) {
			free(OWQ_array(owq_shallow));
			return -ENOMEM;
		}
		break;
	default:
		break;
	}

	return 0;
}

void OWQ_destroy_shallow_aggregate(struct one_wire_query *owq_shallow)
{
	if (OWQ_array(owq_shallow)) {
		free(OWQ_array(owq_shallow));
		OWQ_array(owq_shallow) = NULL;
	}
	switch (OWQ_pn(owq_shallow).selected_filetype->format) {
	case ft_binary:
	case ft_ascii:
	case ft_vascii:
		free(OWQ_buffer(owq_shallow));
		break;
	default:
		break;
	}
}

void OWQ_create_temporary(struct one_wire_query *owq_temporary, char *buffer, size_t size, off_t offset, struct parsedname *pn)
{
	OWQ_buffer(owq_temporary) = buffer;
	OWQ_size(owq_temporary) = size;
	OWQ_offset(owq_temporary) = offset;
	memcpy(PN(owq_temporary), pn, sizeof(struct parsedname));
}
