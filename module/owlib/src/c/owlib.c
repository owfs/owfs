/*
$Id$
    OWFS -- One-Wire filesystem
    OWHTTPD -- One-Wire Web Server
    Written 2003 Paul H Alfille
    email: palfille@earthlink.net
    Released under the GPL
    See the header file: ow.h for full attribution
    1wire/iButton system from Dallas Semiconductor
*/

#include "owfs_config.h"
#include "ow.h"
#include "ow_devices.h"

/* All ow library setup */
void LibSetup( void ) {
    /* All output to syslog */
    openlog( "OWFS" , LOG_PID , LOG_DAEMON ) ;

    /* special resort in case static data (devices and filetypes) not properly sorted */
//printf("LibSetup\n");
    DeviceSort() ;

    /* DB cache code */
#ifdef OW_CACHE
//printf("CacheOpen\n");
    Cache_Open() ;
//printf("CacheOpened\n");
#endif /* OW_CACHE */

    /* global mutex attribute */
#ifdef OW_MT
 #ifdef __UCLIBC__
    pthread_mutexattr_init(&mattr);
    pthread_mutexattr_settype(&mattr, PTHREAD_MUTEX_ADAPTIVE_NP);
    pmattr = &mattr ;
 #endif /* __UCLIBC__ */
#endif /* OW_MT */
    
    start_time = time(NULL) ;
}

#ifdef __UCLIBC__

static void catchchild() ;
static int my_daemon(int nochdir, int noclose) ;

#include <sys/types.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/wait.h>

static void catchchild() {
    pid_t pid;
    int status;

    /*signal(SIGCHLD, catchchild);*/ /* Unneeded */

    pid = wait4(-1, &status, WUNTRACED, 0);
#if 0
    if (WIFSTOPPED(status))
        sprintf(buf, "owlib: %d: Child %d stopped\n", getpid(), pid);
    else
        sprintf(buf, "owlib: %d: Child %d died\n", getpid(), pid);
#endif /* 0 */
}

/*
  This is a test to implement daemon() with uClibc and without MMU...
  I'm not sure it works correctly, so don't use it yet.
*/
static int my_daemon(int nochdir, int noclose) {
    struct sigaction act;
    int pid;
    int fd;

    //printf("owlib: Warning, my_daemon() is used instead of daemon().\n");
    //printf("owlib: Run application with --foreground instead.\n");

    signal(SIGCHLD, SIG_DFL);

//#if defined(__UCLIBC__) && !defined(__UCLIBC_HAS_MMU__)
#ifdef __UCLIBC_HAS_MMU__
    pid = fork();
#else /* __UCLIBC_HAS_MMU__ */
    pid = vfork();
#endif /* __UCLIBC_HAS_MMU__ */
    switch(pid) {
    case -1:
        memset(&act, 0, sizeof(act));
        act.sa_handler = catchchild;
        act.sa_flags = SA_RESTART;
        sigaction(SIGCHLD, &act, NULL);
//printf("owlib: my_daemon: pid=%d fork error\n", getpid());

        return(-1);
    case 0:
        break;
    default:
    //signal(SIGCHLD, SIG_DFL);
//printf("owlib: my_daemon: pid=%d exit parent\n", getpid());
        _exit(0);
    }

    if(setsid() < 0) {
        perror("setsid:");
        return -1;
    }

    /* Make certain we are not a session leader, or else we
     * might reacquire a controlling terminal */
#ifdef __UCLIBC_HAS_MMU__
    pid = fork();
#else /* __UCLIBC_HAS_MMU__ */
    pid = vfork();
#endif /* __UCLIBC_HAS_MMU__ */
    if(pid) {
      //printf("owlib: my_daemon: _exit() pid=%d\n", getpid());
        _exit(0);
    }

    memset(&act, 0, sizeof(act));
    act.sa_handler = catchchild;
    act.sa_flags = SA_RESTART;
    sigaction(SIGCHLD, &act, NULL);

    if(!nochdir) {
        chdir("/");
    }

#if 0
    if (!noclose && (fd = open("/dev/null", O_RDWR, 0)) != -1) {
        dup2(fd, STDIN_FILENO);
        dup2(fd, STDOUT_FILENO);
        dup2(fd, STDERR_FILENO);
        if (fd > 2)
            close(fd);
    }
#else /* 0 */
    if(!noclose) {
        close(STDIN_FILENO);
        close(STDOUT_FILENO);
        close(STDERR_FILENO);
        if (dup(dup(open("/dev/null", O_APPEND)))==-1){
            perror("dup:");
            return -1;
        }
    }
#endif /* 0 */
    return 0;
}
#endif /* __UCLIBC__ */

/* Start the owlib process -- actually only tests for backgrounding */
int LibStart( void ) {
    struct connection_in * in = indevice ;
    if ( indevice==NULL ) {
        fprintf(stderr, "No device port/server specified (-d or -u or -s)\n%s -h for help\n",progname);
        BadAdapter_detect(NewIn()) ;
        return 1;
    }
    do {
        int ret = 0 ;
        switch( in->busmode ) {
        case bus_remote:
            ret = Server_detect(in) ;
            break ;
        case bus_serial:
        {
            /** Actually COM and Parallel */
            struct stat s ;
            if ( (ret=stat( in->name, &s )) ) {
                syslog( LOG_ERR, "Cannot stat port: %s error=%s\n",in->name,strerror(ret)) ;
            } else if ( ! S_ISCHR(s.st_mode) ) {
                syslog( LOG_INFO , "Not a character device: %s\n",in->name) ;
                ret = -EBADF ;
            } else if ((in->fd = open(in->name, O_RDWR | O_NONBLOCK)) < 0) {
                ret = errno ;
                syslog( LOG_ERR,"Cannot open port: %s error=%s\n",in->name,strerror(ret)) ;
            } else if ( major(s.st_rdev) == 99 ) { /* parport device */
#ifndef OW_PARPORT
                ret =  -ENOPROTOOPT ;
#else /* OW_PARPORT */
                if ( (ret=DS1410_detect(in)) ) {
                    syslog(LOG_WARNING, "Cannot detect the DS1410E parallel adapter\n");
                }
#endif /* OW_PARPORT */
            } else if ( COM_open(in) ) { /* serial device */
                ret = -ENODEV ;
            } else if ( DS2480_detect(in) ) { /* Set up DS2480/LINK interface */
                syslog(LOG_WARNING,"Cannot detect DS2480 or LINK interface on %s.\n",in->name) ;
                if ( DS9097_detect(in) ) {
                    syslog(LOG_WARNING,"Cannot detect DS9097 (passive) interface on %s.\n",in->name) ;
                    ret = -ENODEV ;
                }
            }
        }
            break ;
        case bus_usb:
#ifdef OW_USB
            ret = DS9490_detect(in) ;
#else /* OW_USB */
            fprintf(stderr,"Cannot setup USB port. Support not compiled into %s\n",progname);
            ret = 1 ;
#endif /* OW_USB */
            break ;
        default:
            ret = 1 ;
            break ;
        }
        if (ret) BadAdapter_detect(in) ;
    } while ( (in=in->next) ) ;

    /* daemon() should work for embedded systems with MMU, but
     * I noticed that the WRT54G router somethimes had problem with this.
     * If owfs was started in background AND owfs was statically linked,
     *   it seemed to work.
     * If owfs was started in background AND owfs was dynamically linked,
     *   daemon() hanged in systemcall dup2() for some reason???
     * I tried to replace uClibc's daemon() with the one above. It worked
     * in both cases sometimes, but not always... ?!?
     *
     * The best would be to use a statically linked owfs-binary, and then it
     * works in both cases.
     */
    //if(background) printf("Call daemon\n");
    if ( background &&
#if defined(__UCLIBC__)
        my_daemon(1, 0)
#else /* defined(__UCLIBC__) */
        daemon(1, 0)
#endif /* defined(__UCLIBC__) */
    ) {
        fprintf(stderr,"Cannot enter background mode, quitting.\n") ;
        return 1 ;
    }
    //if(background) printf("Call daemon done\n");
    /* store the PID */
    pid_num = getpid() ;

    if (pid_file) {
        FILE * pid = fopen(  pid_file, "w+" ) ;
        if ( pid == NULL ) {
            syslog(LOG_WARNING,"Cannot open PID file: %s Error=%s\n",pid_file,strerror(errno) ) ;
            free( pid_file ) ;
            pid_file = NULL ;
            return 1 ;
        }
        fprintf(pid,"%lu",pid_num ) ;
        fclose(pid) ;
    }
    /* Setup the multithreading synchronizing locks */
    LockSetup() ;
    return 0 ;
}

/* All ow library closeup */
void LibClose( void ) {
    if ( pid_file ) {
       if ( unlink( pid_file ) ) syslog(LOG_WARNING,"Cannot remove PID file: %s error=%s\n",pid_file,strerror(errno)) ;
       free( pid_file ) ;
       pid_file = NULL ;
    }
#ifdef OW_CACHE
    Cache_Close() ;
#endif /* OW_CACHE */
    FreeIn() ;
    FreeOut() ;
    closelog() ;
    
#ifdef OW_MT
    if ( pmattr ) pthread_mutexattr_destroy(pmattr);
#endif /* OW_MT */

    if ( progname && progname[0] ) free(progname) ;
}

struct s_timeout timeout = {1,DEFAULT_TIMEOUT,10*DEFAULT_TIMEOUT,5*DEFAULT_TIMEOUT,} ;
void Timeout( const char * c ) {
    timeout.vol = strtol( c,NULL,10 ) ;
    if ( errno || timeout.vol<1 ) timeout.vol = DEFAULT_TIMEOUT ;
    timeout.stable = 10*timeout.vol ;
    timeout.dir = 5*timeout.vol ;
}
