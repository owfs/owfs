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
#include "error.h"

/* put our executable name here where everybody can see it */
static const char *exe_name = "owftpd";

static void ow_exit( int e ) ;

int main(int argc, char *argv[]) {
    char c ;
    int max_clients;
    int log_facility;

    /* hard code the user as "nobody" */
    const char* user_ptr = "nobody";
    char *address;
    
    struct passwd *user_info;
    error_t err;

    struct ftp_listener_t ftp_listener;

    sigset_t term_signal;
    int sig;

    /* grab our executable name */
    if (argc > 0) {
        exe_name = argv[0];
    }

    /* Set up owlib */
    LibSetup() ;

    /* verify we're running as root */
    if (geteuid() != 0) {
        fprintf(stderr, "%s: program needs root permission to run\n", exe_name);
        ow_exit(1);
    }

    /* default command-line arguments */
    portnum = FTP_PORT;
    user_ptr = NULL;
    address = FTP_ADDRESS;
    max_clients = MAX_CLIENTS;
    log_facility = LOG_FTP;

    while ( (c=getopt_long(argc,argv,OWLIB_OPT,owopts_long,NULL)) != -1 ) {
        switch (c) {
        case 'h':
            fprintf(stderr,
            "Usage: %s ttyDevice [options] \n"
            "   or: %s [options] -d ttyDevice \n"
            "    -p port   -- tcp port for ftp comnmand channel (default 21)\n" ,
        exe_name,exe_name ) ;
            break ;
        case 'V':
            fprintf(stderr,
            "%s version:\n\t" VERSION "\n",exe_name ) ;
            break ;
        }
        if ( owopt(c,optarg) ) ow_exit(0) ; /* rest of message */
    }

    /* non-option arguments */
    if ( optind == argc-1 ) {
        ComSetup(argv[optind]) ;
        ++optind ;
    }

    if ( devfd==-1 && devusb==0 ) {
        fprintf(stderr, "No device port specified (-d or -u)\n%s -h for help\n",argv[0]);
        ow_exit(1);
    }

    /*
            } else if (strcmp(argv[i], "-i") == 0
                       || strcmp(argv[i], "--interface") == 0) {
                if (++i >= argc) {
                    print_usage("missing interface");
                    exit(1);
                }
                address = argv[i];
            } else if (strcmp(argv[i], "-m") == 0
                       || strcmp(argv[i], "--max-clients") == 0) {
                if (++i >= argc) {
                    print_usage("missing number of max clients");
                    exit(1);
                }
                num = strtol(argv[i], &endptr, 0);
                if ((num < MIN_NUM_CLIENTS) || (num > MAX_NUM_CLIENTS) 
                    || (*endptr != '\0')) {

                    snprintf(temp_buf, sizeof(temp_buf),
                        "max clients must be a number between %d and %d",
                        MIN_NUM_CLIENTS, MAX_NUM_CLIENTS);
                    print_usage(temp_buf);

                    exit(1);
                }
                max_clients = num;
*/
    
    /* Test portnum */
    if ((portnum < MIN_PORT) || (portnum > MAX_PORT) ) {
        fprintf(stderr,"%s: port(=%d) must be a number between %d and %d",exe_name,portnum,MIN_PORT, MAX_PORT);
        ow_exit(1);
    }
    
    /* nobody user */
    if ( (user_info=getpwnam(user_ptr)) == NULL) {
        fprintf(stderr, "%s: invalid user name: %s, will run as root!\n", exe_name,user_ptr);
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

