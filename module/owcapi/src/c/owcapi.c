/*
$Id$
     OW -- One-Wire filesystem
    version 0.4 7/2/2003

    Function naming scheme:
    OW -- Generic call to interaface
    LI -- LINK commands
    L1 -- 2480B commands
    FS -- filesystem commands
    UT -- utility functions
    COM - serial port functions
    DS2480 -- DS9097 serial connector

    Written 2003 Paul H Alfille
*/

#include <config.h>
#include "owfs_config.h"
#include "ow.h"
#include "owcapi.h"
#include <limits.h>

#if OW_MT
    pthread_t main_threadid ;
    #define IS_MAINTHREAD (main_threadid == pthread_self())
#else /* OW_MT */
    #define IS_MAINTHREAD 1
#endif /* OW_MT */

#define MAX_ARGS 20

static ssize_t ReturnAndErrno( ssize_t ret ) {
    if ( ret < 0 ) {
        errno = -ret ;
        return -1 ;
    } else {
        errno = 0 ;
        return ret ;
    }
}

ssize_t OW_init( const char * params ) {
    ssize_t ret = 0 ;

    if ( OWLIB_can_init_start() ) {
        OWLIB_can_init_end() ;
        return -EALREADY ;
    }
    
    /* Set up owlib */
    LibSetup(opt_c) ;

    /* Proceed with init while lock held */
    /* grab our executable name */
    Global.progname = strdup("OWCAPI");

    ret = owopt_packed( params ) ;
          
    if ( ret || (ret=LibStart()) ) {
        LibClose() ;
    }

    OWLIB_can_init_end() ;
    return ReturnAndErrno(ret) ;
}

ssize_t OW_init_args( int argc, char ** argv ) {
    ssize_t ret = 0 ;
    int c ;

    if ( OWLIB_can_init_start() ) {
        OWLIB_can_init_end() ;
        return ReturnAndErrno(-EALREADY) ;
    }
    
    /* Set up owlib */
    LibSetup(opt_c) ;

    /* Proceed with init while lock held */
    /* grab our executable name */
    if (argc > 0) Global.progname = strdup(argv[0]);

    while ( 
            ( (c=getopt_long(argc,argv,OWLIB_OPT,owopts_long,NULL)) != -1 )
            && ( ret==0)
          ) {
        ret = owopt(c,optarg) ;
    }

    /* non-option arguments */
    while ( optind < argc ) {
        OW_ArgGeneric(argv[optind]) ;
        ++optind ;
    }

    if ( ret || (ret=LibStart()) ) {
        LibClose() ;
    }

    OWLIB_can_init_end() ;    
    return ReturnAndErrno(ret) ;
}

static void getdircallback( void * v, const struct parsedname * const pn2 ) {
    struct charblob * cb = v ;
    char buf[OW_FULLNAME_MAX+2] ;
    FS_DirName( buf, OW_FULLNAME_MAX, pn2 ) ;
    CharblobAdd( buf, strlen(buf), cb ) ;
    if ( IsDir(pn2) ) CharblobAddChar( '/', cb ) ;
}
/*
  Get a directory,  returning a copy of the contents in *buffer (which must be free-ed elsewhere)
  return length of string, or <0 for error
  *buffer will be returned as NULL on error
 */
static ssize_t getdir( char ** buffer, const struct parsedname * pn ) {
    struct charblob cb ;
    int ret ;

    CharblobInit( &cb ) ;
    ret = FS_dir2( getdircallback, &cb, pn ) ;
    if ( ret < 0 ) {
        // continue ;
    } else if ( (*buffer = strdup( cb.blob )) ) {
        ret = cb.used ;
    } else {
        ret = -ENOMEM ;
    }
    CharblobClear( &cb ) ;
    return ret ;
}

/*
  Get a value,  returning a copy of the contents in *buffer (which must be free-ed elsewhere)
  return length of string, or <0 for error
  *buffer will be returned as NULL on error
 */
static ssize_t getval( char ** buffer, const struct parsedname * pn ) {
    ssize_t ret ;
    ssize_t s = FullFileLength(&pn) ;
    if ( s <= 0 ) return -ENOENT ;
    if ( (*buffer = malloc(s+1))==NULL ) return -ENOMEM ;
    ret = FS_read_postparse( *buffer, s, 0, &pn ) ;
    if ( ret < 0 ) {
        free(*buffer) ;
        *buffer = NULL ;
    }
    return ret ;
}

ssize_t OW_get( const char * path, char ** buffer, size_t * buffer_length ) {
    struct parsedname pn ;
    ssize_t ret = 0 ; /* current buffer string length */

    /* Check the parameters */
    if ( buffer==NULL ) return ReturnAndErrno(-EINVAL) ;
    if ( path==NULL ) path="/" ;
    if ( strlen(path) > PATH_MAX ) return ReturnAndErrno(-EINVAL) ;

    *buffer = NULL ; // default return string on error
    if ( buffer_length ) *buffer_length = 0 ;
    
    if ( OWLIB_can_access_start() ) { /* Check for prior init */
        ret = -ENETDOWN ;
    } else if ( FS_ParsedName( path, &pn ) ) { /* Can we parse the input string */
        ret = -ENOENT ;
    } else {
        if ( IsDir( &pn ) ) { /* A directory of some kind */
            ret = getdir( buffer, &pn ) ;
            if ( ret>0 && buffer_length ) *buffer_length = ret ;
        } else { /* A regular file */
            ret = getval( buffer, &pn ) ;
            if ( ret>0 && buffer_length ) *buffer_length = ret ;
        }
        FS_ParsedName_destroy(&pn) ;
    }
    OWLIB_can_access_end() ;
    return ReturnAndErrno(ret) ;
}

ssize_t OW_lread( const char * path, char * buf, const size_t size, const off_t offset ) {
    return ReturnAndErrno( FS_read( path, buf, size, offset ) ) ;
}

ssize_t OW_lwrite( const char * path, const char * buf, const size_t size, const off_t offset ) {
    return ReturnAndErrno( FS_write( path, buf, size, offset ) ) ;
}

ssize_t OW_put( const char * path, const char * buffer, size_t buffer_length ) {
    ssize_t ret ;
        
    /* Check the parameters */
    if ( buffer==NULL || buffer_length==0 ) return ReturnAndErrno(-EINVAL) ;
    if ( path==NULL ) return ReturnAndErrno(-EINVAL) ;
    if ( strlen(path) > PATH_MAX ) return ReturnAndErrno(-EINVAL) ;

    /* Check for prior init */
    if ( OWLIB_can_access_start() ) {
        ret = -ENETDOWN ;
    } else {
        ret = FS_write(path,buffer,buffer_length,0) ;
    }
    OWLIB_can_access_end() ;
    return ReturnAndErrno(ret) ;
}

void OW_finish( void ) {
    OWLIB_can_finish_start() ;
        LibClose() ;
    OWLIB_can_finish_end() ;
}
