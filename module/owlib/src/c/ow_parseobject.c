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
    
    printf("FS_OWQ_create of %s\n",path) ;
    if ( FS_ParsedName(path,pn) ) return -ENOENT ;
    OWQ_buffer(owq) = buffer ;
    OWQ_size(  owq) = size ;
    OWQ_offset(owq) = offset ;
    if ( pn->extension == -1 ) {
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
    
    printf("FS_OWQ_create_plus of %s + %s\n",path,file) ;
    if ( FS_ParsedNamePlus(path,file,pn) ) return -ENOENT ;
    OWQ_buffer(owq) = buffer ;
    OWQ_size(  owq) = size ;
    OWQ_offset(owq) = offset ;
    if ( pn->extension == -1 ) {
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
    printf("FS_OWQ_destroy of %s\n",OWQ_pn(owq).path) ;
    if ( OWQ_pn(owq).extension == -1 ) {
        if ( OWQ_array(owq) ) {
            free( OWQ_array(owq) ) ;
            OWQ_array(owq) = NULL ;
        }
    }
    FS_ParsedName_destroy( &OWQ_pn(owq) ) ;
}
