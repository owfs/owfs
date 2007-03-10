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
int FS_OWQ_create( const char * path, char * buffer, size_t size, off_t offset, struct one_wire_query * owq )
{
    struct parsedname * pn = &OWQ_pn(owq) ;
    
    LEVEL_DEBUG("FS_OWQ_create of %s\n",path) ;
    if ( FS_ParsedName(path,pn) ) return -ENOENT ;
    OWQ_buffer(owq) = buffer ;
    OWQ_size(  owq) = size ;
    OWQ_offset(owq) = offset ;
    if ( pn->extension == EXTENSION_ALL ) {
        OWQ_array(owq) = calloc((size_t) pn->ft->ag->elements, sizeof(union value_object)) ;
        if (OWQ_array(owq) == NULL ) {
            FS_ParsedName_destroy( pn ) ;
            return -ENOMEM ;
        }
    } else {
        OWQ_I(owq) = 0 ;
    }
    return 0 ;
}

/* Create the Parsename structure (using path and file) and load the relevant fields */
int FS_OWQ_create_plus( const char * path, const char * file, char * buffer, size_t size, off_t offset, struct one_wire_query * owq )
{
    struct parsedname * pn = &OWQ_pn(owq) ;
    
    LEVEL_DEBUG("FS_OWQ_create_plus of %s + %s\n",path,file) ;
    if ( FS_ParsedNamePlus(path,file,pn) ) return -ENOENT ;
    OWQ_buffer(owq) = buffer ;
    OWQ_size(  owq) = size ;
    OWQ_offset(owq) = offset ;
    if ( pn->extension == EXTENSION_ALL ) {
        OWQ_array(owq) = calloc((size_t) pn->ft->ag->elements, sizeof(union value_object)) ;
        if (OWQ_array(owq) == NULL ) {
            FS_ParsedName_destroy( pn ) ;
            return -ENOMEM ;
        }
    } else {
        OWQ_I(owq) = 0 ;
    }
    return 0 ;
}

void FS_OWQ_destroy( struct one_wire_query * owq )
{
    LEVEL_DEBUG("FS_OWQ_destroy of %s\n",OWQ_pn(owq).path) ;
    if ( OWQ_pn(owq).extension == EXTENSION_ALL ) {
        if ( OWQ_array(owq) ) {
            free( OWQ_array(owq) ) ;
            OWQ_array(owq) = NULL ;
        }
    }
    FS_ParsedName_destroy( &OWQ_pn(owq) ) ;
}

static void value_object_pointers( struct one_wire_query * owq )
{
    int elements = OWQ_pn(owq).ft->ag->elements ;
    int comma_length = (OWQ_pn(owq).ft->format==ft_binary) ? 1 : 0 ;
    size_t field_length = OWQ_FileLength(owq) ;
    int extension ;
    char * buffer_pointer = OWQ_buffer(owq) ;

    for ( extension = 0 ; extension < elements ; ++extension ) {
        OWQ_array_mem(owq,extension) = buffer_pointer ;
        OWQ_array_length(owq,extension) = field_length ;
        buffer_pointer += field_length + comma_length ;
    }
}

/* make a "shallow" copy -- but possibly full array size or just an element size */
/* if full array size, allocate extra space for value_object and possible buffer */
int OWQ_create_shallow( struct one_wire_query * owq_shallow, struct one_wire_query * owq_original, int extension )
{
    struct parsedname * pn = &OWQ_pn(owq_original) ;

    memcpy( owq_shallow, owq_original, sizeof(struct one_wire_query) ) ;
    OWQ_pn(owq_shallow).extension = extension;
    
    if ( extension == EXTENSION_ALL ) {
        /* allocate new value_object array space */
        OWQ_array(owq_shallow) = calloc((size_t) pn->ft->ag->elements, sizeof(union value_object)) ;
        if ( OWQ_array(owq_shallow) == NULL ) return -ENOMEM ;
        if ( pn->extension == EXTENSION_ALL ) {
            memcpy( OWQ_array(owq_shallow), OWQ_array(owq_original), pn->ft->ag->elements * sizeof(union value_object) ) ;
        }
        if ( pn->extension>EXTENSION_ALL ) {
            OWQ_offset(owq_shallow) = 0 ;
            switch ( pn->ft->format ) {
                case ft_binary:
                case ft_ascii:
                case ft_vascii:
                    /* allocate larger buffer space */
                    OWQ_size(owq_shallow) = OWQ_FullFileLength(owq_shallow) ;
                    OWQ_buffer(owq_shallow) = malloc( OWQ_size(owq_shallow) ) ;
                    if ( OWQ_buffer(owq_shallow) == NULL ) {
                        free( OWQ_array(owq_shallow) ) ;
                        return -ENOMEM ;
                    }
                    value_object_pointers(owq_shallow) ;
                    break ;
                default:
                    break ;
            }
        }
    }
    return 0 ;
}

void OWQ_destroy_shallow( struct one_wire_query * owq_shallow, struct one_wire_query * owq_original )
{
    if ( OWQ_pn(owq_shallow).extension == EXTENSION_ALL ) {
        if ( OWQ_array(owq_shallow) ) {
            free( OWQ_array(owq_shallow) ) ;
            OWQ_array(owq_shallow) = NULL ;
        }
        if ( OWQ_pn(owq_original).extension>EXTENSION_ALL ) {
            switch ( OWQ_pn(owq_original).ft->format ) {
                case ft_binary:
                case ft_ascii:
                case ft_vascii:
                    free ( OWQ_buffer(owq_shallow) ) ;
                    break ;
                default:
                    break ;
            }
        }
    }
}
