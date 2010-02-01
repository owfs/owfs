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

char *version( ) {
    return VERSION;
}

int init( const char * dev ) {

    if ( API_init(dev) ) {
        return 0 ; // error
    }
    return 1 ;
}

int put( const char * path, const char * value ) {
    int ret = 0 ; /* bad result */
    
    if ( value!=NULL) {
        if ( API_access_start() == 0 ) {
            size_t s = strlen(value) ;
            if ( FS_write(path,value,s,0) < 0  ) {
                ret = 1 ; // success
            }
            API_access_end() ;
        }   
    }
    return ret ;
}

static void getdircallback( void * v, const struct parsedname * const pn_entry ) {
    struct charblob * cb = v ;
    const char * buf = FS_DirName(pn_entry) ;
    CharblobAdd( buf, strlen(buf), cb ) ;
    if ( IsDir(pn_entry) ) CharblobAddChar( '/', cb ) ;
}
/*
  Get a directory,  returning a copy of the contents in *buffer (which must be free-ed elsewhere)
  return length of string, or <0 for error
  *buffer will be returned as NULL on error
 */
static void getdir( struct one_wire_query * owq ) {
    struct charblob cb ;
    
    CharblobInit( &cb ) ;
    if ( FS_dir( getdircallback, &cb, PN(owq) ) >= 0 ) {
        OWQ_buffer(owq) = strdup( (CharblobLength(&cb) >0) ? CharblobData(&cb) : "") ;
    } else {
        OWQ_buffer(owq) = NULL ;
    }
    CharblobClear( &cb ) ;
}

/*
  Get a value,  returning a copy of the contents in *buffer (which must be free-ed elsewhere)
  return length of string, or <0 for error
  *buffer will be returned as NULL on error
 */
static void getval( struct one_wire_query * owq ) {
    ssize_t s = FullFileLength(PN(owq)) ; // fix from Matthias Urlichs
    if ( s <= 0 ) {
		return ;
	}
    if ( (OWQ_buffer(owq) = malloc(s+1))==NULL ) {
		return ;
	}
    OWQ_size(owq) = s ;
    if ( (s = FS_read_postparse( owq )) < 1 ) {
        free(OWQ_buffer(owq)) ;
        OWQ_buffer(owq) = NULL ;
    } else {
        OWQ_buffer(owq)[s] = '\0' ; // shorten to actual returned length
    }
}

char * get( const char * path ) {
    char * buf = NULL ;

    if ( API_access_start() == 0 ) {
        OWQ_allocate_struct_and_pointer(owq);
    
        /* Check the parameters */
        if ( path==NULL ) {
			path="/" ;
		}
    
        if ( strlen(path) > PATH_MAX ) {
            // buf = NULL ;
        } else if ( FS_OWQ_create( path, NULL, (size_t)0, (off_t)0, owq ) ) { /* Can we parse the input string */
            // buf = NULL ;
        } else {
            if ( IsDir( PN(owq) ) ) { /* A directory of some kind */
                getdir( owq ) ;
            } else { /* A regular file */
                getval( owq ) ;
            }
            buf = OWQ_buffer(owq) ;
            FS_OWQ_destroy(owq) ;
        }
        API_access_end() ;
    }
    return buf ;
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
