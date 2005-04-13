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

#include <linux/stddef.h>
#include <syslog.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <signal.h>

#include <fuse.h>

#include "owfs_config.h"
#include "ow.h"
#include "owfs.h"
//#include <mcheck.h>

/* Stuff from helper.h */
#define FUSE_MOUNTED_ENV        "_FUSE_MOUNTED"
#define FUSE_UMOUNT_CMD_ENV     "_FUSE_UNMOUNT_CMD"

static void ow_exit( int e ) ;

static void exit_handler(int i) {
    (void) i ;
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
//    int multithreaded = 1;

#if FUSE_MAJOR_VERSION == 1
    char ** opts = NULL ;
    char * tok ;
    int i = 0 ;
#endif

    /* grab our executable name */
    if ( argc>0 ) progname = strdup(argv[0]) ;

//    mtrace() ;
    /* Set up owlib */
    LibSetup() ;

    while ( (c=getopt_long(argc,argv,OWLIB_OPT,owopts_long,NULL)) != -1 ) {
        switch (c) {
        case 'h':
            fprintf(stderr,
            "Usage: %s mountpoint -d devicename [options] \n"
            "   or: %s [options] portname mountpoint \n"
            "   --fuse-opt=\"-x -f\"  options to send to fuse_mount (must be quoted)\n",
            argv[0],argv[0] ) ;
            break ;
        case 'V':
            fprintf(stderr,
            "%s version:\n\t" VERSION "\n",argv[0] ) ;
            break ;
        case 260: /* fuse-opt */
            fuse_opt = strdup( optarg ) ;
            if ( fuse_opt == NULL ) fprintf(stderr,"Insufficient memory to store FUSE options: %s\n",optarg) ;
            break ;
        }
        if ( owopt(c,optarg) ) ow_exit(0) ; /* rest of message */
    }

    /* non-option arguments */
    if ( optind == argc ) {
        fprintf(stderr,"No mount point specified.\nTry '%s -h' for help.\n",argv[0]) ;
        ow_exit(1) ;
    } else if ( outdevices ) {
        fprintf(stderr,"Network output not supported\n");
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
    if ( fuse_opt ) {
        opts = malloc( (1+strlen(fuse_opt)) * sizeof(char *) ) ; // oversized
        if ( opts == NULL ) {
            fprintf(stderr,"Memory allocation problem\n") ;
            ow_exit(1) ;
        }
        tok = strtok(fuse_opt," ") ;
        while ( tok ) {
            opts[i++] = tok ;
            tok = strtok(NULL," ") ;
        }
        opts[i] = NULL ;
    }

    if ( (fuse_fd = fuse_mount(fuse_mountpoint, opts)) == -1 ) ow_exit(1) ;
    if (opts) free( opts ) ;
#else /* FUSE_MAJOR_VERSION == 1 */
    if ( (fuse_fd = fuse_mount(fuse_mountpoint, fuse_opt)) == -1 ) ow_exit(1) ;
#endif /* FUSE_MAJOR_VERSION == 1 */

    set_signal_handlers(exit_handler);

    /*
     * Now we drop privledges and become a daemon.
     */
    if ( LibStart() ) ow_exit(1) ;

#if (FUSE_MAJOR_VERSION == 1)
    fuse = fuse_new(fuse_fd, 0, &owfs_oper);
#elif (FUSE_MAJOR_VERSION == 2) && (FUSE_MINOR_VERSION < 2)
    fuse = fuse_new(fuse_fd, NULL, &owfs_oper);
#elif (FUSE_MAJOR_VERSION == 2) && (FUSE_MINOR_VERSION >= 2)
    fuse = fuse_new(fuse_fd, NULL, &owfs_oper, sizeof(owfs_oper));
#else
    fuse = fuse_new(fuse_fd, NULL, &owfs_oper);
#endif
    if (multithreading) {
        fuse_loop_mt(fuse) ;
    } else {
        fuse_loop(fuse) ;
    }
    close(fuse_fd) ;
    ow_exit(0);
    return 0 ;
}

static void ow_exit( int e ) {
    LibClose() ;
    if(fuse != NULL) {
      fuse_exit(fuse);
    }
    
    /* Should perhaps only use fuse_teardown() if FUSE 2
     * It doesn't check for NULL pointers though */
    // fuse_teardown(fuse, fuse_fd, fuse_mountpoint);
#if (FUSE_MAJOR_VERSION == 2)
    if(fuse != NULL) {
      /* this will unlink all inodes.. Could take some time on slow
       * processors... eg. Coldfire 54MHz */
      fuse_destroy(fuse);
    }
#endif
    fuse = NULL;
    if(fuse_fd != -1) {
      close(fuse_fd);
    }
    if(fuse_mountpoint != NULL) {
        fuse_unmount(fuse_mountpoint);
        free(fuse_mountpoint) ;
        fuse_mountpoint = NULL;
    } else if(umount_cmd[0] != '\0') {
        system(umount_cmd);
    }
    exit( e ) ;
}
