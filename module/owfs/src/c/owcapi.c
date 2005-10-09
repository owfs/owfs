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
#else
#define IS_MAINTHREAD 1
#endif

/* Prototypes */
static int OW_get_validated( const char * path, char * buffer, size_t buffer_length ) ;
static void CheckName( char * path, const char * name, size_t name_length ) ;

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

int OW_init_params( int argc, char ** argv ) {
    int ret = 0 ;
    int c ;

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

    background = 1 ; // Cannot enter background mode, since this is a called library
    if ( ret==0 ) ret = LibStart() ;

    if ( ret ) LibClose() ;

    return ret ;
}

/* Code */
int OW_init( const char * device ) {
    int ret = 0 ;

    if ( device==NULL ) return -ENODEV

    /* Set up owlib */
    LibSetup() ;
    
    switch( device[0] ) {
    case 0:
        ret = -ENODEV ;
        break ;
    case 'u':
        ret = OW_ArgUSB(&device[1]) ;
        break ;
    default:
        ret = OW_ArgGeneric(device) ;
        break ;
    }

    background = 1 ; // Cannot enter background mode, since this is a called library
    if ( ret==0 ) ret = LibStart() ;

    if ( ret ) LibClose() ;

    return ret ;
}

int OW_finish( void ) {
    return LibClose() ;
}

/* Allocates and copies path if not NULL terminated string */
static void CheckName( char * path, const char * name, size_t name_length ) {
    if (name==NULL || name_length==0) {
        path = strdup("/") ;
    } else if ( name[name_length]=='\0' ) {
        path = name ;
    } else {
        path = malloc(name_length+1) ;
        if ( path ) {
            memcpy(path,name,name_length) ;
            path[name_length] = '\0' ;
        }
    }
}

int OW_put( const char * name, size_t name_length, const char * buffer, size_t buffer_length ) {
    char * path ;
    int ret ;

    CheckName(path,name,name_length) ;
    if ( path==NULL ) return -ENOMEM ;

    ret = FS_write(path,buffer,buffer_length,0) ;  
  
    if ( path != name ) free(path) ;
    return ret ;
}

static int OW_get_validated( const char * path, char * buffer, size_t buffer_length ) {
    struct parsedname pn ;
    struct stateinfo si ;
    int s = 0 ; /* current buffer string length */
    /* Embedded callback function */
    void directory( const struct parsedname * const pn2 ) {
        if ( s && s<buffer_length-2) strcpy( &buffer[s++], "," ) ;
        FS_DirName( &buffer[s], buffer_length-s-2, pn2 ) ;
        if (
            pn2->dev ==NULL
            || pn2->ft ==NULL
            || pn2->ft->format ==ft_subdir
            || pn2->ft->format ==ft_directory
        ) strcat( &buffer[s], "/" );
        s = strlen( buffer ) ;
    }

    /* Parse the input string */
    pn.si = &si ;
    if ( FS_ParsedName( path, &pn ) ) return -ENOENT ;

    if ( pn.dev==NULL || pn.ft == NULL || pn.subdir ) { /* A directory of some kind */
        buffer[buffer_length-1] = '\0' ; /* protect end of buffer */
        FS_dir( directory, &pn ) ;
    } else { /* A regular file */
        s =  FS_read_3times( buffer, buffer_length, 0, &pn ) ;
    }
    FS_ParsedName_destroy(&pn) ;
    return s ;
}

int OW_get( const char * name, size_t name_length, char * buffer, size_t buffer_length ) {
    char * path ;
    int ret ;

    if ( buffer==NULL || buffer_length==0 ) return -EINVAL ;

    CheckName(path,name,name_length) ;
    if ( path==NULL ) return -ENOMEM ;
 
    ret = OW_get_validated(path,buffer,buffer_length) ;

    if ( ret<0 ) buffer[0] = '\0' ;

    if ( path != name ) free(path) ;
    return ret ;
}
