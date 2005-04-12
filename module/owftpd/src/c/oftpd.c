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


void *connection_acceptor(struct ftp_listener_t *f);

struct ftp_listener_t ftp_listener;

static void ow_exit( int e ) {
    syslog(LOG_INFO, "all connections finished, FTP server exiting");
    LibClose() ;
    if(ftp_listener.fd) ftp_listener_stop(&ftp_listener);
    exit( e ) ;
}

static void exit_handler(int i) {
  return ow_exit( ((i<0) ? 1 : 0) ) ;
}


/* just a test function to find fuse mount */
static int find_fusedev(char *owfs_dir)
{
  char tmp[128];
  char *p = NULL, *p2;
  FILE *fp;
  int found = 0;
  struct stat st;

  if(!(fp = fopen("/proc/mounts", "r"))) return 0;
  while(!feof(fp)) {
    if(!(p = fgets(tmp, 80, fp))) {
      break;
    }
    p2 = strtok(p, " ");
    if(p2) p2 = strtok(NULL, " ");
    if(p2) {
      if(owfs_dir) strncpy(owfs_dir, p2, 128);
      p2 = strtok(NULL, " ");
    }
    if(p2 && !strncmp(p2, "fuse", 4)) {
      found = 1;
      break;
    }
  }
  fclose(fp);

  return found;
}


int main(int argc, char *argv[]) {
    char c ;
    int max_clients;
    int log_facility;
    char rootdir[128];
    /* hard code the user as "nobody" */
    const char* username = "nobody";
    char *address;
    struct passwd *user_info = NULL;
    error_code_t err;
    sigset_t term_signal;
    int sig;
    int portnum ;
#ifdef OW_MT
    pthread_t thread ;
#ifndef __UCLIBC__
    pthread_attr_t attr ;
    pthread_attr_init(&attr) ;
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED) ;
#endif /* __UCLIBC__ */
#endif /* OW_MT */

    memset(&ftp_listener, 0, sizeof(struct ftp_listener_t));

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
        default:
            break;
        }
        if ( owopt(c,optarg) ) ow_exit(0) ; /* rest of message */
    }

    /* non-option arguments */
    while ( optind < argc-1 ) {
      OW_ArgGeneric(argv[optind]) ;
      ++optind ;
    }

    if(ServerAddr(outdevice)<0) {
      printf("ServerAddr() failed\n");
      ow_exit(1);
    }

    //if(!outdevice->host) outdevice->host = strdup("0.0.0.0");
    portnum = atoi(outdevice->service) ;
    address = outdevice->host ;

    //printf("portnum=%d\n", portnum);
    //printf("address=[%s]\n", address);

    set_signal_handlers(exit_handler);

    /*
     * Now we drop privledges and become a daemon.
     */
    if ( LibStart() ) ow_exit(1) ;

    /* log the start time */
    openlog(NULL, LOG_NDELAY, log_facility);
    syslog(LOG_INFO,"Starting, version %s, as PID %d", VERSION, getpid());

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

    /* fix this... quick hack to test ftp-server only. require owfs to
     * to run on the same server */
    if(!find_fusedev(rootdir)) {
      printf("Couldn't find fuse-dir\n");
      ow_exit(0);
    }
    chroot(rootdir);

    /* create our main listener */
    if (!ftp_listener_init(&ftp_listener,
                           address,
                           portnum,
                           max_clients,
                           INACTIVITY_TIMEOUT,
			   "/",
                           &err))
    {
        syslog(LOG_ERR, "error initializing FTP listener on port %d; %s",
          portnum,error_get_desc(&err));
        exit(1);
    }

    connection_acceptor( &ftp_listener ) ;

    ow_exit(0);
}

