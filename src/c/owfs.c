   /*
    FUSE: Filesystem in Userspace
    OW: Module for reading 1-wire devices
*/

/*
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

#include "owlib_config.h"
#include "owfs_config.h"
#include "ow.h"
#include "owfs.h"

static void ow_exit( int e ) ;
void exit_handler(int i) ;
void set_signal_handlers( void ) ;

struct fuse_operations owfs_oper = {
	getattr:	FS_getattr,
	readlink:	NULL,
	getdir:     FS_getdir,
	mknod:	NULL,
	mkdir:	NULL,
	symlink:	NULL,
	unlink:	NULL,
	rmdir:	NULL,
	rename:     NULL,
	link:	NULL,
	chmod:	NULL,
	chown:	NULL,
	truncate:	FS_truncate,
	utime:	FS_utime,
	open:	FS_open,
	read:	FS_read,
	write:	FS_write,
	statfs:	FS_statfs,
	release:	NULL
};


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
    char c ;
//    int multithreaded = 1;

    /* All output to syslog */
    openlog( "OWFS" , LOG_PID , LOG_DAEMON ) ;

    devport[0] = '\0' ; /* initialize the serial port name to blank */

    while ( (c=getopt(argc,argv,"d:ht:CFRK")) != -1 ) {
        switch (c) {
//        case 's':
//            multithreaded = 0 ;
//            break ;
         case 'h':
             fprintf(stderr,
             "Usage: %s mountpoint -d devicename [options] \n"
             "   or: %s [options] portname mountpoint \n"
             "  Required:\n"
             "    -d device -- serial port connecting to 1-wire network (e.g. /dev/ttyS0)\n"
             "  Options:\n"
//             "    -s      disable multithreaded operation\n"
             "    -t      cache timeout (in seconds)\n"
             "    -h      print help\n"
             "    -C | -F | -K | -R Temperature scale Celsius(default)|Fahrenheit|Kelvin|Rankine\n",
             argv[0],argv[0] ) ;
             ow_exit(1);
         case 't':
             Timeout( optarg ) ;
             break ;
//         case 'd':
//             flags |= FUSE_DEBUG;
//             break ;
         case 'd':
		     strncpy( devport, optarg, PATH_MAX ) ;
             break ;
        case 'C':
		    tempscale = temp_celsius ;
			break ;
        case 'F':
		    tempscale = temp_fahrenheit ;
			break ;
        case 'R':
		    tempscale = temp_rankine ;
			break ;
        case 'K':
		    tempscale = temp_kelvin ;
			break ;
	 	default:
            fprintf(stderr, "invalid option: %c, try -h for help\n", c);
            ow_exit(1) ;
         }
    }

    /* non-option arguments */
    if ( optind == argc ) {
        fprintf(stderr,"No mount point specified.\nTry '%s -h' for help.\n",argv[0]) ;
        ow_exit(1) ;
    } else if ( optind == argc-2 ) {
	    strncpy( devport, argv[optind], PATH_MAX ) ;
        ++optind ;
    }

    if ( devport[0] == '\0' ) {
        fprintf(stderr, "No port specified (-p portname)\n%s -h for help\n",argv[0]);
        ow_exit(1);
    } else if ( LibSetup(devport) ) { /* 1-wire bus specific setup */
        fprintf(stderr, "Cannot open serial port, see system log for details.\n");
        ow_exit(1);
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
        fuse_mountpoint = strdup(argv[optind]);
        fuse_fd = fuse_mount(fuse_mountpoint, NULL);
        if(fuse_fd == -1) ow_exit(1);
    } else {
        fuse_mountpoint = strdup(argv[optind]);
        fuse_fd = fuse_mount(fuse_mountpoint, NULL);
        if(fuse_fd == -1) ow_exit(1);
    }

    set_signal_handlers();

//     printf( "argc=%d\nport=%s\noptind=%d\nav0=%s\nav1=%s\nav2=%s\nav3=%s\n", argc,devport,optind,argv[0],argv[1],argv[2],argv[3]);
    fuse = fuse_new(fuse_fd, flags, &owfs_oper);

//    if(multithreaded) {
//        fuse_loop_mt(fuse);
//    } else {
        fuse_loop(fuse);
//    }

    close(fuse_fd);

    ow_exit(0);
    return 0 ;
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
