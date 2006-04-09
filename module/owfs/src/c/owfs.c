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
#include "owfs.h"

/* Stuff from helper.h */
#define FUSE_MOUNTED_ENV        "_FUSE_MOUNTED"
#define FUSE_UMOUNT_CMD_ENV     "_FUSE_UNMOUNT_CMD"

static void ow_exit( int e ) ;

#ifdef OW_MT
pthread_t main_threadid ;
#define IS_MAINTHREAD (main_threadid == pthread_self())
#else
#define IS_MAINTHREAD 1
#endif

static void exit_handler(int i) {
    return ow_exit( ((i<0) ? 1 : 0) ) ;
}

/*
    OW -- One Wire
    Global variables -- each invokation will have it's own data
*/
struct fuse *fuse;
char *fuse_mountpoint = NULL;
int fuse_fd = -1 ;
char umount_cmd[1024] = "";
char * fuse_mnt_opt = NULL ;
char * fuse_open_opt = NULL ;

/* ---------------------------------------------- */
/* Command line parsing and mount handler to FUSE */
/* ---------------------------------------------- */
int main(int argc, char *argv[]) {
    int c ;
    struct Fuse_option fuse_options ;

    /* grab our executable name */
    if ( argc>0 ) progname = strdup(argv[0]) ;

    //mtrace() ;
    /* Set up owlib */
    LibSetup() ;

    while ( (c=getopt_long(argc,argv,OWLIB_OPT,owopts_long,NULL)) != -1 ) {
        switch (c) {
        case 'V':
            fprintf(stderr,
            "%s version:\n\t" VERSION "\n",argv[0] ) ;
            break ;
        case 260: /* fuse_mnt_opt */
            if ( fuse_mnt_opt ) free(fuse_mnt_opt ) ;
            fuse_mnt_opt = Fuse_arg(optarg,"FUSE mount options") ;
            if ( fuse_mnt_opt == NULL ) ow_exit(0) ;
            break ;
        case 267: /* fuse_open_opt */
            if ( fuse_open_opt ) free(fuse_open_opt ) ;
            fuse_open_opt = Fuse_arg(optarg,"FUSE open options") ;
            if ( fuse_open_opt == NULL ) ow_exit(0) ;
            break ;
        }
        if ( owopt(c,optarg,opt_owfs) ) ow_exit(0) ; /* rest of message */
    }

    /* non-option arguments */
    if ( optind == argc ) {
        LEVEL_DEFAULT("No mount point specified.\nTry '%s -h' for help.\n",argv[0])
        ow_exit(1) ;
    } else if ( outdevices ) {
        LEVEL_DEFAULT("Network output not supported\n")
        ow_exit(1) ;
    } else {
        while ( optind < argc-1 ) {
            OW_ArgGeneric(argv[optind]) ;
            ++optind ;
        }
    }

    // FUSE directory mounting
    fuse_mountpoint = strdup(argv[optind]);
    LEVEL_CONNECT("fuse mount point: %s\n",fuse_mountpoint) ;
    set_signal_handlers(exit_handler);

   /* Now we drop privledges and become a daemon. */
    if ( LibStart() ) ow_exit(1) ;
    //printf("Lib started\n");
#ifdef OW_MT
    main_threadid = pthread_self() ;
#endif

#if FUSE_VERSION >= 20
    /* Set up "command line" for main fuse routines */
    Fuse_setup( &fuse_options ) ; // command line setup
    Fuse_add(fuse_mountpoint , &fuse_options) ; // mount point
    Fuse_add("-o" , &fuse_options) ; // add "-o direct_io" to prevent buffering
    Fuse_add("direct_io" , &fuse_options) ;
    if ( !background ) {
        Fuse_add("-f", &fuse_options) ; // foreground for fuse too
        if ( error_level > 2 ) Fuse_add("-d", &fuse_options ) ; // debug for fuse too
    }
    Fuse_parse(fuse_mnt_opt, &fuse_options) ;
    Fuse_parse(fuse_open_opt, &fuse_options) ;
 #ifdef OW_MT
    Fuse_add("-s" , &fuse_options) ; // single threaded
 #endif /* OW_MT */
 #if FUSE_VERSION >= 26 // requires extra parameter
    printf("fuse_main=%d\n",fuse_main(fuse_options.argc, fuse_options.argv, &owfs_oper, NULL )) ;
 #else
    fuse_main(fuse_options.argc, fuse_options.argv, &owfs_oper ) ;
 #endif
    Fuse_cleanup( &fuse_options ) ;
   ow_exit(0) ;
#endif

    return 0 ;
}

static void ow_exit( int e ) {
    if(IS_MAINTHREAD) {
        LibClose() ;
    }
    /* Process never die on WRT54G router with uClibc if exit() is used */
    _exit( e ) ;
}

