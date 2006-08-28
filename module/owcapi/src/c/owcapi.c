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

ssize_t OW_get( const char * path, char ** buffer, size_t * buffer_length ) {
    struct parsedname pn ;
    char * buf = NULL ;
    size_t sz = 0 ; /* current buffer size */
    int s = 0 ; /* current buffer string length */
    /* Embedded callback function */
    void directory( const struct parsedname * const pn2 ) {
        size_t sn = s+OW_FULLNAME_MAX+2 ; /* next buffer limit */
        if ( sz<sn ) {
            void * temp = buf ;
            sz = sn ;
            buf = realloc( temp, sn ) ;
            if ( buf==NULL && temp ) free(temp) ;
        }
        if ( buf ) {
            if ( s ) strcpy( &buf[s++], "," ) ; // add a comma
            FS_DirName( &buf[s], OW_FULLNAME_MAX, pn2 ) ;
            if ( IsDir(pn2) ) strcat( &buf[s], "/" );
            s += strlen( &buf[s] ) ;
	    //LEVEL_DEBUG("buf=[%s] len=%d\n", buf, s);
        }
    }

    /* Check the parameters */
    if ( buffer==NULL ) return ReturnAndErrno(-EINVAL) ;
    if ( path==NULL ) path="/" ;
    if ( strlen(path) > PATH_MAX ) return ReturnAndErrno(-EINVAL) ;

    if ( OWLIB_can_access_start() ) { /* Check for prior init */
        s = -ENETDOWN ;
    } else if ( FS_ParsedName( path, &pn ) ) { /* Can we parse the input string */
        s = -ENOENT ;
    } else {
        if ( pn.dev==NULL || pn.ft == NULL || pn.subdir ) { /* A directory of some kind */
            int ret = FS_dir( directory, &pn ) ;
	    if(ret < 0) s = ret;
	    //LEVEL_DEBUG("OW_get(): FS_dir returned %d\n", ret);
        } else { /* A regular file */
            s = FullFileLength(&pn) ;
	    //LEVEL_DEBUG("OW_get(): size=%d\n", s);
            if ( (s>=0) && (buf=(char *) malloc( s+1 )) ) {
                int r =  FS_read_postparse( buf, s, 0, &pn ) ;
                if ( r<0 ) {
		    LEVEL_DEBUG("OW_get(): failed after FS_read_postparse %d\n", r);
                    free(buf) ;
                    s = r ;
                } else {
                    buf[s] = '\0' ;
                }
            }
        }
        FS_ParsedName_destroy(&pn) ;
        if ( s<0 ) {
            if ( buf ) free(buf) ;
            buf = NULL ;
            if ( buffer_length ) *buffer_length = 0 ;
        } else {
            if ( buffer_length ) *buffer_length = s ;
        }
        buffer[0] = buf ;
    }
    OWLIB_can_access_end() ;
    return ReturnAndErrno(s) ;
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
