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

#include "owfs_config.h"
#include "ow.h"
#include "owcapi.h"

#ifdef OW_MT
    pthread_t main_threadid ;
    #define IS_MAINTHREAD (main_threadid == pthread_self())
#else /* OW_MT */
    #define IS_MAINTHREAD 1
#endif /* OW_MT */

#define MAX_ARGS 20
int OW_init_string( const char * params ) {
    char * prms = strdup(params) ;
    char * p = prms ;
    int argc = 0 ;
    int ret ;
    char * argv[MAX_ARGS+1] ;
    while ( argc < MAX_ARGS ) {
        argv[argc] = strdup(strsep(&p," ")) ;
        if ( argv[argc] == NULL ) break ;
        ++argc ;
    }
    argv[argc+1]=NULL ;

    ret = OW_init_args( argc, argv ) ;

    while(argc>=0) {
        if(argv[argc]) free(argv[argc]) ;
        argc-- ;
    }
    if ( prms ) free( prms ) ;
    return ret ;
}

int OW_init_args( int argc, char ** argv ) {
    int ret = 0 ;
    int c ;

    if ( OWLIB_can_init_start() ) {
        OWLIB_can_init_end() ;
        return -EALREADY ;
    }
    
    /* Proceed with init while lock held */    
    /* grab our executable name */
    if (argc > 0) progname = strdup(argv[0]);

    /* Set up owlib */
    LibSetup() ;

    while ( 
            ( (c=getopt_long(argc,argv,OWLIB_OPT,owopts_long,NULL)) != -1 )
            && ( ret==0)
          ) {
        ret = owopt(c,optarg,opt_server) ;
    }

    /* non-option arguments */
    while ( optind < argc ) {
        OW_ArgGeneric(argv[optind]) ;
        ++optind ;
    }

    delay_background = 1 ; // Cannot enter background mode, since this is a called library
    if ( ret==0 ) ret = LibStart() ;

    if ( ret ) LibClose() ;

    OWLIB_can_init_end() ;    
    return ret ;
}

/* Code */
int OW_init( const char * device ) {
    int ret = 0 ;

    if ( device==NULL ) return -ENODEV ;

    if ( OWLIB_can_init_start() ) {
        OWLIB_can_init_end() ;
        return -EALREADY ;
    }
    
    /* Proceed with init while lock held */

    /* Set up owlib */
    LibSetup() ;
    
    ret = OW_ArgGeneric(device) ;

    background = 0 ; // Cannot enter background mode, since this is a called library
    if ( ret==0 ) ret = LibStart() ;

    if ( ret ) LibClose() ;

    OWLIB_can_init_end() ;
    return ret ;
}

int OW_get( const char * path, char ** buffer, size_t * buffer_length ) {
    struct parsedname pn ;
    char * buf = NULL ;
    size_t sz ; /* current buffer size */
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
            if (
                pn2->dev ==NULL
                || pn2->ft ==NULL
                || pn2->ft->format ==ft_subdir
                || pn2->ft->format ==ft_directory
               ) strcat( &buf[s], "/" );
            s = strlen( buf ) ;
//printf("buf=%s len=%d\n",buf,s);
        }
    }

    /* Check the parameters */
    if ( buffer==NULL ) return -EINVAL ;
    if ( path==NULL ) path="/" ;
    if ( strlen(path) > PATH_MAX ) return -EINVAL ;

    if ( OWLIB_can_access_start() ) { /* Check for prior init */
        s = -ENETDOWN ;
    } else if ( FS_ParsedName( path, &pn ) ) { /* Can we parse the input string */
        s = -ENOENT ;
    } else {
        if ( pn.dev==NULL || pn.ft == NULL || pn.subdir ) { /* A directory of some kind */
            s=sz=0 ;
            FS_dir( directory, &pn ) ;
        } else { /* A regular file */
            s = FS_size_postparse(&pn) ;
            if ( (buf=(char *) malloc( s+1 )) ) {
                int r =  FS_read_3times( buf, s, 0, &pn ) ;
                if ( r<0 ) {
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
    return s ;
}

int OW_put( const char * path, const char * buffer, size_t buffer_length ) {
    int ret ;
        
    /* Check the parameters */
    if ( buffer==NULL || buffer_length==0 ) return -EINVAL ;
    if ( path==NULL ) return -EINVAL ;
    if ( strlen(path) > PATH_MAX ) return -EINVAL ;

    /* Check for prior init */
    if ( OWLIB_can_access_start() ) {
        ret = -ENETDOWN ;
    } else {
        ret = FS_write(path,buffer,buffer_length,0) ;
    }
    OWLIB_can_access_end() ;
    return ret ;
}

void OW_finish( void ) {
    OWLIB_can_finish_start() ;
        LibClose() ;
    OWLIB_can_finish_end() ;
}
