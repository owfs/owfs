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

static void ow_exit( int e ) ;
static void fuser_mount_wrapper( void ) ;
void exit_handler(int i) ;
void set_signal_handlers( void ) ;

/*
	OW -- Onw Wire
	Global variables -- each invokation will have it's own data
*/
struct fuse *fuse;
time_t scan_time ;
char *fuse_mountpoint = NULL;
int fuse_fd = -1 ;
char umount_cmd[1024] = "";

/* ---------------------------------------------- */
/* Command line parsing and mount handler to FUSE */
/* ---------------------------------------------- */
int main(int argc, char *argv[]) {
    int flags = 0 ; /* flags to fuse */
    int c ;
//    int multithreaded = 1;

    /* All output to syslog */
    openlog( "OWFS" , LOG_PID , LOG_DAEMON ) ;

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
        }
        if ( owopt(c,optarg) ) ow_exit(0) ; /* rest of message */
    }

    /* non-option arguments */
    if ( optind == argc ) {
        fprintf(stderr,"No mount point specified.\nTry '%s -h' for help.\n",argv[0]) ;
        ow_exit(1) ;
    } else if ( optind == argc-2 ) {
        ComSetup(argv[optind]) ;
        ++optind ;
    }

    if ( devfd==-1 && devusb==0 ) {
        fprintf(stderr, "No port specified (-d or -u) or error opening port\nSee system log for details\n%s -h for help\n",argv[0]);
        ow_exit(1);
    }

    // FUSE magic. I don't understand it.
    fuse_mountpoint = strdup(argv[optind]);
    fuser_mount_wrapper() ;

    if ( LibStart() ) ow_exit(1) ;

    set_signal_handlers();
    fuse = fuse_new(fuse_fd, flags, &owfs_oper);
//    if(multithreaded) {
//        fuse_loop_mt(fuse);
//    } else {
//printf("FUSELOOP\n");
        fuse_loop(fuse);
//    }
    close(fuse_fd);

    ow_exit(0);
    return 0 ;
}

static void fuser_mount_wrapper( void ) {
    char ** opts = NULL ;

    if ( fuse_opt ) {
        char * tok ;
        int i = 0 ;
        opts = (char **) malloc( (1+strlen(fuse_opt)) * sizeof(char *) ) ; // oversized
        tok = strtok(fuse_opt," ") ;
        while ( tok ) {
            opts[i++] = tok ;
            tok = strtok(NULL," ") ;
        }
        opts[i] = NULL ;
    }

    // FUSE magic. I don't understand it.
    if(getenv(FUSE_MOUNTED_ENV)) {
        char *tmpstr = getenv(FUSE_UMOUNT_CMD_ENV);

         /* Old (obsolescent) way of doing the mount:
             fusermount [options] mountpoint [program [args ...]]
           fusermount execs this program and passes the control file
           descriptor dup()-ed to stdin */
        fuse_fd = 0;
        if(tmpstr != NULL)
            strncpy(umount_cmd, tmpstr, sizeof(umount_cmd) - 1);
//        fuse_mountpoint = strdup(argv[optind]);
        fuse_fd = fuse_mount(fuse_mountpoint, opts);
        if(fuse_fd == -1) ow_exit(1);
    } else {
//        fuse_mountpoint = strdup(argv[optind]);
        fuse_fd = fuse_mount(fuse_mountpoint, opts);
        if(fuse_fd == -1) ow_exit(1);
    }
    if ( opts ) { /* just to be anal, clear the string memory */
        free( opts ) ;
        free( fuse_opt ) ;
        fuse_opt = NULL ;
    }
}

static void ow_exit( int e ) {
    LibClose() ;
	closelog() ;
    if(fuse != NULL)
        fuse_exit(fuse);
    if(fuse_mountpoint != NULL) {
        fuse_unmount(fuse_mountpoint);
        free(fuse_mountpoint) ;
    } else if(umount_cmd[0] != '\0') {
        system(umount_cmd);
    }
    exit( e ) ;
}

void exit_handler(int i) {
    (void) i ;
    return ow_exit(1) ;
}

void set_signal_handlers( void ) {
    struct sigaction sa;

    sa.sa_handler = exit_handler;
    sigemptyset(&(sa.sa_mask));
    sa.sa_flags = 0;

    if (sigaction(SIGHUP, &sa, NULL) == -1 ||
	sigaction(SIGINT, &sa, NULL) == -1 ||
	sigaction(SIGTERM, &sa, NULL) == -1) {

	perror("Cannot set exit signal handlers");
        ow_exit(1);
    }

    sa.sa_handler = SIG_IGN;

    if(sigaction(SIGPIPE, &sa, NULL) == -1) {
	perror("Cannot set ignored signals");
        exit(1);
    }
}
