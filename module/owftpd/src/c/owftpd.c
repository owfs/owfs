/*
 * $Id$
 */

#include "owftpd.h"
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

/* put our executable name here where everybody can see it */
static const char *exe_name = "owftpd";

static void daemonize(void);
static void print_usage(const char *error);

int main(int argc, char *argv[])
{
    int i;
    
    long num;
    char *endptr;
    int port;
    int max_clients;
    int log_facility;

    char *user_ptr;
    char *dir_ptr;
    char *address;
    
    char temp_buf[256];

    struct passwd *user_info;
    struct error_code_s err;

    struct ftp_listener_s ftp_listener;

    int detach;

    sigset_t term_signal;
    int sig;

    /* grab our executable name */
    if (argc > 0) {
        exe_name = argv[0];
    }

    /* verify we're running as root */
    if (geteuid() != 0) {
        fprintf(stderr, "%s: program needs root permission to run\n", exe_name);
        exit(1);
    }

    /* default command-line arguments */
    port = FTP_PORT;
    user_ptr = NULL;
    dir_ptr = NULL;
    address = FTP_ADDRESS;
    max_clients = MAX_CLIENTS;
    detach = 1;
    log_facility = LOG_FTP;

    /* check our command-line arguments */
    /* we're stubbornly refusing to use getopt(), because we can */
    /* :) */ 
    for (i=1; i<argc; i++) {
        
        /* flags/optional arguments */
        if (argv[i][0] == '-') {
            if (strcmp(argv[i], "-p") == 0 
                || strcmp(argv[i], "--port") == 0) {
                if (++i >= argc) {
                    print_usage("missing port number");
                    exit(1);
                }
                num = strtol(argv[i], &endptr, 0);
                if ((num < MIN_PORT) || (num > MAX_PORT) || (*endptr != '\0')) {

                    snprintf(temp_buf, sizeof(temp_buf), 
                             "port must be a number between %d and %d",
                             MIN_PORT, MAX_PORT);
                    print_usage(temp_buf);

                    exit(1);
                }
                port = num;
            } else if (strcmp(argv[i], "-h") == 0
                       || strcmp(argv[i], "--help") == 0) {
                print_usage(NULL);
                exit(0);
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
            } else if (strcmp(argv[i], "-N") == 0 
                       || strcmp(argv[i], "--nodetach") == 0) {
                detach = 0;
            } else if (strcmp(argv[i], "-l") == 0
                       || strcmp(argv[i], "--local") == 0) {
                if (++i >= argc) {
                    print_usage("missing number for local facility logging");
                    exit(1);
                }
                switch (argv[i][0]) {
                    case '0': 
                        log_facility = LOG_LOCAL0;
                        break;
                    case '1': 
                        log_facility = LOG_LOCAL1;
                        break;
                    case '2': 
                        log_facility = LOG_LOCAL2;
                        break;
                    case '3': 
                        log_facility = LOG_LOCAL3;
                        break;
                    case '4': 
                        log_facility = LOG_LOCAL4;
                        break;
                    case '5': 
                        log_facility = LOG_LOCAL5;
                        break;
                    case '6': 
                        log_facility = LOG_LOCAL6;
                        break;
                    case '7': 
                        log_facility = LOG_LOCAL7;
                        break;
                }
            } else {
                print_usage("unknown option");
                exit(1);
            }

        /* required parameters */
        } else {
            if (user_ptr == NULL) {
                user_ptr = argv[i];
            } else if (dir_ptr == NULL) {
                dir_ptr = argv[i];
            } else {
                print_usage("too many arguments on the command line");
                exit(1);
            }
        }
    }
    if ((user_ptr == NULL) || (dir_ptr == NULL)) {
        print_usage("missing user and/or directory name");
        exit(1);
    }

    user_info = getpwnam(user_ptr);
    if (user_info == NULL) {
        fprintf(stderr, "%s: invalid user name\n", exe_name);
        exit(1);
    }

    /* become a daemon */
    if (detach) {
        daemonize();
    }

    /* avoid SIGPIPE on socket activity */
    signal(SIGPIPE, SIG_IGN);         

    /* log the start time */
    openlog(NULL, LOG_NDELAY, log_facility);
    syslog(LOG_INFO,"Starting, version %s, as PID %d", VERSION, getpid());

    /* change to root directory */
    if (chroot(dir_ptr) != 0) {
        syslog(LOG_ERR, "error with root directory; %s %s\n", exe_name, 
          strerror(errno));
        exit(1);
    }
    if (chdir("/") != 0) {
        syslog(LOG_ERR, "error changing directory; %s\n", strerror(errno));
        exit(1);
    }

    /* create our main listener */
    if (!ftp_listener_init(&ftp_listener, 
                           address,
                           port,
                           max_clients,
                           INACTIVITY_TIMEOUT, 
                           &err)) 
    {
        syslog(LOG_ERR, "error initializing FTP listener; %s",
          error_get_desc(&err));
        exit(1);
    }

    /* set user to be as inoffensive as possible */
    if (setgid(user_info->pw_gid) != 0) {
        syslog(LOG_ERR, "error changing group; %s", strerror(errno));
        exit(1);
    }
    if (setuid(user_info->pw_uid) != 0) {
        syslog(LOG_ERR, "error changing group; %s", strerror(errno));
        exit(1);
    }

    /* start our listener */
    if (ftp_listener_start(&ftp_listener, &err) == 0) {
        syslog(LOG_ERR, "error starting FTP service; %s", error_get_desc(&err));
        exit(1);
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
    exit(0);
}

static void print_usage(const char *error)
{
    if (error != NULL) {
        fprintf(stderr, "%s: %s\n", exe_name, error);
    }
    fprintf(stderr, 
           " Syntax: %s [ options... ] user_name root_directory\n", exe_name);
    fprintf(stderr, 
           " Options:\n"
           " -p, --port <num>\n"
           "     Set the port to listen on (Default: %d)\n"
           " -i, --interface <IP Address>\n"
           "     Set the interface to listen on (Default: all)\n"
           " -m, --max-clients <num>\n"
           "     Set the number of clients allowed at one time (Default: %d)\n"
           "-l, --local <local-logging>\n"
           "     Use LOCAL facility for syslog, local-logging is 0 to 7\n"
           " -N, --nodetach\n"
           "     Do not detach from TTY and become a daemon\n",
           DEFAULT_FTP_PORT, MAX_CLIENTS);
}

static void daemonize( void )
{
    int fork_ret;
    int max_fd;
    int null_fd;
    int fd;

    null_fd = open("/dev/null", O_RDWR);
    if (null_fd == -1) {
        fprintf(stderr, "%s: error opening null output device; %s\n", exe_name, 
          strerror(errno));
        exit(1);
    }

    max_fd = sysconf(_SC_OPEN_MAX);
    if (max_fd == -1) {
        fprintf(stderr, "%s: error getting maximum open file; %s\n", exe_name, 
          strerror(errno));
        exit(1);
    }


    fork_ret = fork();
    if (fork_ret == -1) {
        fprintf(stderr, "%s: error forking; %s\n", exe_name, strerror(errno));
        exit(1);
    }
    if (fork_ret != 0) {
        exit(0);
    }
    if (setsid() == -1) {
        fprintf(stderr, "%s: error creating process group; %s\n", exe_name, 
          strerror(errno));
        exit(1);
    }
    fork_ret = fork();
    if (fork_ret == -1) {
        fprintf(stderr, "%s: error forking; %s\n", exe_name, strerror(errno));
        exit(1);
    }
    if (fork_ret != 0) {
        exit(0);
    }
    if (dup2(null_fd, 0) == -1) {
        syslog(LOG_ERR, "error setting input to null; %s", 
          strerror(errno));
        exit(1);
    }
    if (dup2(null_fd, 1) == -1) {
        syslog(LOG_ERR, "error setting output to null; %s", 
          strerror(errno));
        exit(1);
    }
    if (dup2(null_fd, 2) == -1) {
        syslog(LOG_ERR, "error setting error output to null; %s", 
          strerror(errno));
        exit(1);
    }
    for (fd=3; fd<max_fd; fd++) {
        close(fd);
    }
}

