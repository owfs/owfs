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

/* ow_interate.c */
/* routines to split reads and writes if longer than page */

#include <config.h>
#include "owfs_config.h"
#include "ow.h"

/*
  Get a value,  returning a copy of the contents in *buffer (which must be free-ed elsewhere)
  return length of string, or <0 for error
  *buffer will be returned as NULL on error
 */

/*
  Get a directory,  returning a copy of the contents in *buffer (which must be free-ed elsewhere)
  return length of string, or <0 for error
  *buffer will be returned as NULL on error
 */
static void getdircallback( void * v, const struct parsedname * const pn_entry ) 
{
	struct charblob * cb = v ;
	const char * buf = FS_DirName(pn_entry) ;
	CharblobAdd( buf, strlen(buf), cb ) ;
	if ( IsDir(pn_entry) ) {
		CharblobAddChar( '/', cb ) ;
	}
}

static void getdir( struct charblob * cb, struct one_wire_query * owq ) 
{  
	if ( FS_dir( getdircallback, cb, PN(owq) ) != 0 ) {
		CharblobClear( cb ) ;
	} else if ( CharblobLength(cb) == 0 ) {
		CharblobAddChar( '\0', cb) ;
	}
}

static char * copy_buffer( char * data, int size )
{
	char * data_copy = NULL ; // default
	if ( size < 1 ) {
		return NULL ;
	}
	data_copy = malloc( size+1 ) ;
	if ( data_copy == NULL ) {
		return NULL ;
	}
	memcpy( data_copy, data, size ) ;
	data_copy[size] = '\0' ;
	return data_copy ;
}

SIZE_OR_ERROR FS_get(const char *path, char **return_buffer, size_t * buffer_length)
{
	SIZE_OR_ERROR size = 0 ;		/* current buffer string length */
	OWQ_allocate_struct_and_pointer( owq ) ;

	/* Check the parameters */
	if (return_buffer == NULL) {
		//  No buffer for read result.
		return -EINVAL;
	}

	if (path == NULL) {
		path = "/";
	}

	*return_buffer = NULL;				// default return string on error

	if (OWQ_create(path, owq)) {	/* Can we parse the input string */
		return -ENOENT;
	}

	if ( IsDir( PN(owq) ) ) { /* A directory of some kind */
		struct charblob cb ;
		CharblobInit(&cb) ;
		getdir( &cb, owq ) ;
		size = CharblobLength(&cb) ;
		*return_buffer = copy_buffer( CharblobData(&cb), size ) ;
		CharblobClear( &cb ) ;
	} else { /* A regular file  -- so read */
		if ( GOOD(OWQ_allocate_read_buffer(owq)) ) { // make the space in the buffer
			size = FS_read_postparse(owq) ;
			*return_buffer = copy_buffer( OWQ_buffer(owq), size ) ;
		}
	}
	// the buffer is allocated by getdir or getval
	OWQ_destroy(owq);

	/* Check the parameters */
	if (*return_buffer == NULL) {
		//  no data.
		return -EINVAL;
	}

	if ( buffer_length != NULL ) {
		*buffer_length = size ;
	}
	return size ;
}
