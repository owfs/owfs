/*
 * $Id$
 */
 /* Modified 8/2004 by Paul Alfille for OWFS the 1-Wire Filesystem */

#include "owfs_config.h"
#include "ow.h"
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <pwd.h>
#include <syslog.h>
#include <pthread.h>
#include <stdlib.h>
#include "oftpd.h"
#include "ftp_listener.h"
#include "ftp_error.h"

static void ow_exit( int e ) ;

int main(int argc, char *argv[]) {
    char c ;
    int max_clients;
    int log_facility;
#ifdef OW_MT
    pthread_t thread ;
    pthread_attr_t attr ;

    pthread_attr_init(&attr) ;
    pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_DETACHED) ;
#endif /* OW_MT */

    LibSetup() ;

    /* hard code the user as "nobody" */
    const char* username = "nobody";
    char *address;

    struct passwd *user_info;
    error_code_t err;

    struct ftp_listener_t ftp_listener;

    sigset_t term_signal;
    int sig;

    /* grab our executable name */
    if (argc > 0) progname = argv[0];

    /* Set up owlib */
    LibSetup() ;

    /* verify we're running as root */
    if (geteuid() != 0) {
        fprintf(stderr, "%s: program needs root permission to run\n", progname);
        ow_exit(1);
    }

    /* default command-line arguments */
    max_clients = MAX_CLIENTS;
    log_facility = LOG_FTP;

    while ( (c=getopt_long(argc,argv,OWLIB_OPT,owopts_long,NULL)) != -1 ) {
        switch (c) {
        case 'h':
            fprintf(stderr,
            "Usage: %s ttyDevice [options] \n"
            "   or: %s [options] -d ttyDevice \n"
            "    -p port   -- tcp port for ftp comnmand channel (default 21)\n" ,
        progname,progname ) ;
            break ;
        case 'V':
            fprintf(stderr,
            "%s version:\n\t" VERSION "\n",progname ) ;
            break ;
        }
        if ( owopt(c,optarg) ) ow_exit(0) ; /* rest of message */
    }

    /* non-option arguments */
    if ( optind == argc-1 ) {
        ComSetup(argv[optind]) ;
        ++optind ;
    }

    /* Need default port? */
    if ( outdevice->name==NULL ) outdevice->name = DEFAULT_PORTNAME ;

    if ( ServerAddr( outdevice->name, &outdevice ) || (ServerListen( &outdevice )<0) ) {
        fprintf(stderr, "socket problems: %s failed to start\n", progname);
        fprintf(stderr,"Cannot start server.\n");
        exit(1);
    }

    /* become a daemon */
    if ( LibStart() ) ow_exit(1) ;

    /* avoid SIGPIPE on socket activity */
    signal(SIGPIPE, SIG_IGN);

    /* log the start time */
    openlog(NULL, LOG_NDELAY, log_facility);
    syslog(LOG_INFO,"Starting, version %s, as PID %d", VERSION, getpid());

    /* create our main listener */
    if (!ftp_listener_init(&ftp_listener,
                           address,
                           portnum,
                           max_clients,
                           INACTIVITY_TIMEOUT,
                           &err))
    {
        syslog(LOG_ERR, "error initializing FTP listener on port %d; %s",
          portnum,error_get_desc(&err));
        exit(1);
    }

    /* set user to be as inoffensive as possible */
    if ( user_info ) {
        if (setgid(user_info->pw_gid) != 0) {
            syslog(LOG_ERR, "error changing group; %s", strerror(errno));
            ow_exit(1);
        }
        if (setuid(user_info->pw_uid) != 0) {
            syslog(LOG_ERR, "error changing group; %s", strerror(errno));
            ow_exit(1);
        }
    }

    /*
     * Now we drop privledges and become a daemon.
     */
    if ( LibStart() ) ow_exit(1) ;

    signal(SIGCHLD, handle_sigchild);
    signal(SIGHUP, SIG_IGN);
    signal(SIGHUP, handle_sighup);
    signal(SIGTERM, handle_sigterm);

    for(;;) {
        ACCEPTLOCK
#ifdef OW_MT
#ifdef __UCLIBC__
        if ( pthread_create( &thread, NULL, ThreadedAccept, &outdevice ) ) ow_exit(1) ;
        pthread_detach(thread);
#else
        if ( pthread_create( &thread, &attr, ThreadedAccept, &outdevice ) ) ow_exit(1) ;
#endif
#else /* OW_MT */
        Acceptor( server.listenfd ) ;
#endif /* OW_MT */
    }

    /* start our listener */
    if (ftp_listener_start(&ftp_listener, &err) == 0) {
        syslog(LOG_ERR, "error starting FTP service; %s", error_get_desc(&err));
        ow_exit(1);
    }

    /* wait for a SIGTERM and exit gracefully */
    sigemptyset(&term_signal);
    sigaddset(&term_signal, SIGTERM);
    sigaddset(&term_signal, SIGINT);
    pthread_sigmask(SIG_BLOCK, &term_signal, NULL);
    sigwait(&term_signal, &sig);
    if (sig == SIGTERM) {
        syslog(LOG_INFO, "SIGTERM received, shutting down");
    } else {
        syslog(LOG_INFO, "SIGINT received, shutting down");
    }
    ftp_listener_stop(&ftp_listener);
    syslog(LOG_INFO, "all connections finished, FTP server exiting");
    ow_exit(0);
}

static void ow_exit( int e ) {
    LibClose() ;
    exit( e ) ;
}
