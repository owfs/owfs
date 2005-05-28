/*
 * $Id$
 */
 /* Modified 8/2004 by Paul Alfille for OWFS the 1-Wire Filesystem */

//#include "owfs_config.h"
//#include "ow.h"
//#include <stdio.h>
//#include <unistd.h>
//#include <sys/types.h>
//#include <sys/stat.h>
//#include <fcntl.h>
//#include <string.h>
//#include <errno.h>
//#include <signal.h>
//#include <pwd.h>
//#include <syslog.h>
//#include <pthread.h>
//#include <stdlib.h>
//#include "oftpd.h"
//#include "ftp_listener.h"
//#include "ftp_error.h"

#include <owftpd.h>

struct ftp_listener_t ftp_listener;
#ifdef OW_MT
pthread_t main_threadid ;
#define IS_MAINTHREAD (main_threadid == pthread_self())
#else
#define IS_MAINTHREAD 1
#endif

void ow_exit( int e ) {
    if(IS_MAINTHREAD) {
        if(ftp_listener.fd) {
            LEVEL_CONNECT("Waiting for all connections to finish")
            ftp_listener_stop(&ftp_listener);
        }
        LEVEL_CONNECT("FTP server exiting, all connections finished")
        LibClose() ;
    }
    exit( e ) ;
}

static void exit_handler(int i) {
    return ow_exit( ((i<0) ? 1 : 0) ) ;
}


extern pthread_mutex_t time_lock ;
extern pthread_mutex_t passive_lock ;

int main(int argc, char *argv[]) {
    int c ;
    int max_clients;
    int log_facility;
    char *address;
    //struct passwd *user_info = NULL;
    error_code_t err;
    int port;

    memset(&ftp_listener, 0, sizeof(struct ftp_listener_t));

    /* grab our executable name */
    if (argc > 0) progname = strdup(argv[0]);

    /* Set up owlib */
    LibSetup() ;

#ifdef __UCLIBC__
    pthread_mutex_init(&time_lock, pmattr);
    pthread_mutex_init(&passive_lock, pmattr);
#endif

    /* verify we're running as root */
    if (geteuid() != 0) {
        LEVEL_DEFAULT("%s: program needs root permission to run\n", progname)
        ow_exit(1);
    }

    /* default command-line arguments */
    max_clients = MAX_CLIENTS;
    log_facility = LOG_FTP;

    while ( (c=getopt_long(argc,argv,OWLIB_OPT,owopts_long,NULL)) != -1 ) {
        switch (c) {
        case 'V':
            fprintf(stderr,
            "%s version:\n\t" VERSION "\n",progname ) ;
            break ;
        default:
            break;
        }
        if ( owopt(c,optarg, opt_ftpd) ) ow_exit(0) ; /* rest of message */
    }

    /* non-option arguments */
    while ( optind < argc-1 ) {
      OW_ArgGeneric(argv[optind]) ;
      ++optind ;
    }

    /* no port was defined, so listen on default port instead */
    if(!outdevices) {
        if(OW_ArgServer( DEFAULT_PORTNAME )) {
            LEVEL_DEFAULT("Error using default address (%d)\n",DEFAULT_PORTNAME)
            ow_exit(1);
         }
    }
    if(ServerAddr(outdevice)<0) {
      LEVEL_DEFAULT("Failed to initialize port. Use '-p' argument.\n")
      ow_exit(1);
    }

    port = atoi(outdevice->service) ;
    address = outdevice->host ;

    set_signal_handlers(exit_handler);

    /*
     * Now we drop privledges and become a daemon.
     */
    if ( LibStart() ) ow_exit(1) ;

#ifdef OW_MT
    main_threadid = pthread_self() ;
#endif

    /* log the start time */
    LEVEL_CONNECT("Starting %s, version %s, as PID %d", progname, VERSION, getpid())

    /* create our main listener */
    if (!ftp_listener_init(&ftp_listener,
                           address,
                           port,
                           max_clients,
                           INACTIVITY_TIMEOUT,
                           &err))
    {
        LEVEL_CONNECT("error initializing FTP listener on port %s:%d; %s\n",address,port, error_get_desc(&err))
        ow_exit(1);
    }

    connection_acceptor( &ftp_listener ) ;

    ow_exit(0);
}
