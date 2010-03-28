/* File: ow.i */
/* $Id$ */

%module OW

%include "typemaps.i"

%init %{
#if 0
// includes not needed here... it just results into duplicate includes
#include "config.h"
#include "owfs_config.h"
#include "ow.h"
#endif
API_setup(opt_swig) ;
%}

%{
#include "config.h"
#include "owfs_config.h"
#include "ow.h"

// define this to add debug-output from newer swig-versions.
//#define SWIGRUNTIME_DEBUG 1

#if OW_MT
    pthread_t main_threadid ;
    #define IS_MAINTHREAD (main_threadid == pthread_self())
#else /* OW_MT */
    #define IS_MAINTHREAD 1
#endif /* OW_MT */

char *version( ) 
{
	return VERSION;
}

int init( const char * dev ) 
{
	if ( API_init(dev) ) {
		return 0 ; // error
	}
	return 1 ;
}

int put( const char * path, const char * value ) 
{
	int ret = 0 ; /* bad result */

	if ( API_access_start() != 0 ) {
		return 0 ; // bad result
	}

	if ( value!=NULL) {
		if ( FS_write( path, value, strlen(value), 0 ) < 0  ) {
			ret = 1 ; // success
		}
	}
	API_access_end() ;
	return ret ;
}

static void getdircallback( void * v, const struct parsedname * const pn_entry ) 
{
	struct charblob * cb = v ;
	const char * buf = FS_DirName(pn_entry) ;
	CharblobAdd( buf, strlen(buf), cb ) ;
	if ( IsDir(pn_entry) ) {
		CharblobAddChar( '/', cb ) ;
	}
}

/*
  Get a directory,  returning a copy of the contents in *buffer (which must be free-ed elsewhere)
  return length of string, or <0 for error
  *buffer will be returned as NULL on error
 */
static void getdir( struct charblob * cb, struct one_wire_query * owq ) 
{  
	CharblobInit( cb ) ;
	if ( FS_dir( getdircallback, cb, PN(owq) ) != 0 ) {
		CharblobClear( cb ) ;
	} else if ( CharblobLength(cb) == 0 ) {
		CharblobAddChar( '\0', cb) ;
	}
}

char * copy_buffer( char * data, int size )
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

char * get( const char * path ) 
{
	char * return_buffer = NULL ;
	struct one_wire_query * owq = NULL ;

	if ( API_access_start() != 0 ) {
		return NULL ; // error
	}

	/* Check the parameters */
	if ( path==NULL ) {
		path="/" ;
	}

	if ( strlen(path) > PATH_MAX ) {
		// return_buffer = NULL ;
	} else if ( (owq = OWQ_create_from_path(path)) != NULL ) { /* Can we parse the input string */
		if ( IsDir( PN(owq) ) ) { /* A directory of some kind */
			struct charblob cb ;
			CharblobInit(&cb) ;
			getdir( &cb, owq ) ;
			return_buffer = copy_buffer( CharblobData(&cb), CharblobLength(&cb)) ;
			CharblobClear( &cb ) ;
		} else { /* A regular file  -- so read */
			if ( GOOD(OWQ_allocate_read_buffer(owq)) ) { // make the space in the buffer
				SIZE_OR_ERROR size = FS_read_postparse(owq) ;
				return_buffer = copy_buffer( OWQ_buffer(owq), size ) ;
			}
		}
		OWQ_destroy(owq) ;
	}

	API_access_end() ;
	return return_buffer ;
}

void finish( void ) {
	API_finish() ;
}


void set_error_print(int val) {
	Globals.error_print = val;
}

int get_error_print(void) {
	return Globals.error_print;
}

void set_error_level(int val) {
	Globals.error_level = val;
}

int get_error_level(void) {
	return Globals.error_level;
}

int opt(const char option_char, const char *arg) {
	return owopt(option_char, arg);
}

%}
%typemap(newfree) char * { if ($1) free($1) ; }
%newobject get ;

extern char *version( );
extern int init( const char * dev ) ;
extern char * get( const char * path ) ;
extern int put( const char * path, const char * value ) ;
extern void finish( void ) ;
extern void set_error_print(int);
extern int get_error_print(void);
extern void set_error_level(int);
extern int get_error_level(void);
extern int opt(const char, const char *);
