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
	if ( BAD(API_init(dev)) ) {
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

/*
  Get a directory,  returning a copy of the contents in *buffer (which must be free-ed elsewhere)
  return length of string, or <0 for error
  *buffer will be returned as NULL on error
 */

char * get( const char * path ) 
{
	char * return_buffer = NULL ;
	if ( API_access_start() != 0 ) {
		return NULL ; // error
	}

	FS_get( path, &return_buffer, NULL ) ;

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
	return GOOD( owopt(option_char, arg) ) ? 0 : 1 ;
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
