/* File: ow.i */

%module OW

%include "typemaps.i"

%init %{
API_setup(program_type_swig) ;
%}

%{
#include "config.h"
#include "owfs_config.h"
#include "ow.h"

// define this to add debug-output from newer swig-versions.
//#define SWIGRUNTIME_DEBUG 1

// Opposite of internal code
#define SWIG_BAD   0
#define SWIG_GOOD  1

char *version( ) 
{
	return OWFS_VERSION;
}

int init( const char * dev ) 
{
	if ( BAD(API_init(dev, restart_if_repeat)) ) {
		return SWIG_BAD ; // error
	}
	return SWIG_GOOD ;
}

int put( const char * path, const char * value ) 
{
	int ret = SWIG_BAD ; /* bad result */

	if ( API_access_start() == 0 ) {
		if ( value!=NULL) {
			if ( FS_write( path, value, strlen(value), 0 ) >= 0  ) {
				ret = SWIG_GOOD ; // success
			}
		}
		API_access_end() ;
	}
		
	return ret ;
}

/*
  Get a directory,  returning a copy of the contents in *buffer (which must be free-ed elsewhere)
  *buffer will be returned as NULL on error
  Also functions as a read
 */

char * get( const char * path ) 
{
	char * return_buffer = NULL ;
	if ( API_access_start() == 0 ) {
		FS_get( path, &return_buffer, NULL ) ;
		API_access_end() ;
	}
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
