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
#include <fuse.h>

/* Just in case version fuse-1.4 is used.. Use oldest api */
#ifndef FUSE_USE_VERSION
#define FUSE_USE_VERSION 11
#endif

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
    OW -- Onw Wire
    Global variables -- each invokation will have it's own data
*/
struct fuse *fuse;
char *fuse_mountpoint = NULL;
int fuse_fd = -1 ;
char umount_cmd[1024] = "";

/* ---------------------------------------------- */
/* Command line parsing and mount handler to FUSE */
/* ---------------------------------------------- */
int main(int argc, char *argv[]) {
    int c ;

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
        case 260: /* fuse_opt */
            fuse_mnt_opt = strdup( optarg ) ;
            if ( fuse_mnt_opt == NULL ) {
                LEVEL_DEFAULT("Insufficient memory to store FUSE options: %s\n",optarg)
                ow_exit(1) ;
            }
            break ;
        case 267: /* fuse_open_opt */
            fuse_open_opt = strdup( optarg ) ;
            if ( fuse_open_opt == NULL ) {
                LEVEL_DEFAULT("Insufficient memory to store FUSE options: %s\n",optarg)
                ow_exit(1) ;
            }
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

#if FUSE_MAJOR_VERSION == 1
    {
      char ** opts = NULL ;
      if ( fuse_mnt_opt ) {
	char * tok ;
	int i = 0 ;

        opts = malloc( (1+strlen(fuse_mnt_opt)) * sizeof(char *) ) ; // oversized
        if ( opts == NULL ) {
            LEVEL_DEFAULT("Memory allocation problem in fuse options\n")
            ow_exit(1) ;
        }
        tok = strtok(fuse_mnt_opt," ") ;
        while ( tok ) {
            opts[i++] = tok ;
            tok = strtok(NULL," ") ;
        }
        opts[i] = NULL ;
      }

      if ( (fuse_fd = fuse_mount(fuse_mountpoint, opts)) == -1 ) ow_exit(1) ;
      if (opts) free( opts ) ;
    }
#elif (FUSE_USE_VERSION == 21) || (FUSE_USE_VERSION == 22)
    /* Time to cleanup the main-loop and use the fuse-helper functions soon.
     * Should perhaps call fuse_setup(), fuse_loop_mt(), fuse_teardown() only.
     */
    if ( (fuse_fd = fuse_mount(fuse_mountpoint, fuse_mnt_opt)) == -1 ) ow_exit(1) ;
#else 
    /* Time to cleanup the main-loop and use the fuse-helper functions soon.
     * Should perhaps call fuse_setup(), fuse_loop_mt(), fuse_teardown() only.
    */
    if ( (fuse_fd = fuse_mount(fuse_mountpoint, fuse_mnt_opt)) == -1 ) ow_exit(1) ;
#endif


    set_signal_handlers(exit_handler);

    /*
     * Now we drop privledges and become a daemon.
     */
    if ( LibStart() ) ow_exit(1) ;

#ifdef OW_MT
    main_threadid = pthread_self() ;
#endif


#if (FUSE_MAJOR_VERSION == 1)
    fuse = fuse_new(fuse_fd, 0, &owfs_oper);
#elif FUSE_USE_VERSION == 21
    fuse = fuse_new(fuse_fd, fuse_open_opt, &owfs_oper);
#elif FUSE_USE_VERSION == 22
    /* Fuse open options fuse-2.4 is
     * debug, use_ino, readdir_ino, direct_io, kernel_cache, umask=
     * uid=, gid=, entry_timeout=, attr_timeout=
     *
     * NOTE: direct_io was Fuse mount option in fuse-2.3 */
    fuse = fuse_new(fuse_fd, fuse_open_opt, &owfs_oper, sizeof(owfs_oper));
#endif

    if (multithreading) {
        fuse_loop_mt(fuse) ;
    } else {
        fuse_loop(fuse) ;
    }

#if (FUSE_MAJOR_VERSION == 2) && (FUSE_MINOR_VERSION >= 4)
    /* fuse_teardown() is called in ow_exit() */
#else  /* FUSE < 2.4 */
    /* Seem to be be better to close file-descriptor before calling
     * fuse_exit() on older fuse versions. */
    close(fuse_fd) ;
    fuse_fd = -1;
#endif  /* FUSE < 2.4 */
    ow_exit(0);
    return 0 ;
}

static void ow_exit( int e ) {
    if(IS_MAINTHREAD) {
      LibClose() ;

#if (FUSE_MAJOR_VERSION == 2) && (FUSE_MINOR_VERSION >= 4)
      if(fuse != NULL) {
	fuse_teardown(fuse, fuse_fd, fuse_mountpoint);
      }
      fuse_fd = -1;
      fuse_mountpoint = NULL;
#else /* FUSE < 2.4 */
      if(fuse != NULL) {
	fuse_exit(fuse);
      }

#if (FUSE_MAJOR_VERSION == 2)
      if(fuse != NULL) {
	fuse_destroy(fuse);
      }
#endif /* FUSE == 2 */

      fuse = NULL;
      if(fuse_fd != -1) {
        close(fuse_fd);
        fuse_fd = -1;
      }
      if(fuse_mountpoint != NULL) {
        fuse_unmount(fuse_mountpoint);
        free(fuse_mountpoint) ;
        fuse_mountpoint = NULL;
      } else if(umount_cmd[0] != '\0') {
        system(umount_cmd);
      }
#endif /* FUSE < 2.4*/
    }
    /* Process never die on WRT54G router with uClibc if exit() is used */
    _exit( e ) ;
}
