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

int now_background = 0 ;
char * SimpleBusName = "None" ;

/* All ow library setup */
void LibSetup( void ) {
    /* special resort in case static data (devices and filetypes) not properly sorted */
    DeviceSort() ;

    /* DB cache code */
#ifdef OW_CACHE
//printf("CacheOpen\n");
    Cache_Open() ;
//printf("CacheOpened\n");
#endif /* OW_CACHE */

#ifndef __UCLIBC__
    /* Setup the multithreading synchronizing locks */
    LockSetup();
#endif /* __UCLIBC__ */

    start_time = time(NULL) ;
    errno = 0 ; /* set error level none */
}

#if defined(__UCLIBC__)
 #if (defined(__UCLIBC_HAS_MMU__) || defined(__ARCH_HAS_MMU__))
  #define HAVE_DAEMON 1
 #else
  #undef HAVE_DAEMON
 #endif
#endif /* __UCLIBC__ */

#ifndef HAVE_DAEMON
#include <sys/types.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <signal.h>

static void catchchild( int sig ) {
    pid_t pid;
    int status;

    pid = wait4(-1, &status, WUNTRACED, 0);
#if 0
    if (WIFSTOPPED(status)) {
        LEVEL_CONNECT("owlib: %d: Child %d stopped\n", getpid(), pid)
    } else {
        LEVEL_CONNECT("owlib: %d: Child %d died\n", getpid(), pid)
    } 
#endif /* 0 */
}

/*
  This is a test to implement daemon()
*/
static int my_daemon(int nochdir, int noclose) {
    struct sigaction act;
    int pid;
    int fd;

    signal(SIGCHLD, SIG_DFL);

#if defined(__UCLIBC__)
    pid = vfork();
#else
    pid = fork();
#endif
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
#if defined(__UCLIBC__)
    pid = vfork();
#else
    pid = fork();
#endif
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
#endif /* HAVE_DAEMON */

#ifdef __UCLIBC__
#ifdef OW_MT
extern char *__pthread_initial_thread_bos ;
void __pthread_initialize(void) ;
#endif
#endif

/* Start the owlib process -- actually only tests for backgrounding */
int LibStart( void ) {
    struct connection_in * in = indevice ;
    int ret = 0 ;

#ifdef __UCLIBC__
    /* First call to pthread should be done after daemon() in uClibc, so
     * I moved it here to avoid calling __pthread_initialize() */
    if ( background ) {
        if(
 #ifdef HAVE_DAEMON
            daemon(1, 0)
 #else
            my_daemon(1, 0)
 #endif
        ) {
            LEVEL_DEFAULT("Cannot enter background mode, quitting.\n")
            return 1 ;
        }
        now_background = 1;
    }

    /* Have to re-initialize pthread since the main-process is gone.
     *
     * This workaround will probably be fixed in uClibc-0.9.28
     * Other uClibc developers have noticed similar problems which are
     * trigged when pthread functions are used in shared libraries. */
    __pthread_initial_thread_bos = NULL ;
    __pthread_initialize();

    /* global mutex attribute */
    pthread_mutexattr_init(&mattr);
    pthread_mutexattr_settype(&mattr, PTHREAD_MUTEX_ADAPTIVE_NP);
    pmattr = &mattr ;

    /* Setup the multithreading synchronizing locks */
    LockSetup();
#endif /* __UCLIBC__ */

    if ( indevice==NULL ) {
    LEVEL_DEFAULT( "No device port/server specified (-d or -u or -s)\n%s -h for help\n",progname) ;
        BadAdapter_detect(NewIn()) ;
        return 1;
    }
    do {
        BadAdapter_detect(in) ; /* default "NOTSUP" calls */
        switch( get_busmode(in) ) {
            case bus_remote:
                ret = Server_detect(in) ;
                break ;
            case bus_serial:
                if ( DS2480_detect(in) ) { /* Set up DS2480/LINK interface */
                    LEVEL_CONNECT("Cannot detect DS2480 or LINK interface on %s.\n",in->name) ;
                    BUS_close( in ) ;
                    BadAdapter_detect(in) ; /* reset the methods */
                    if ( DS9097_detect(in) ) {
                        LEVEL_DEFAULT("Cannot detect DS9097 (passive) interface on %s.\n",in->name) ;
                        ret = -ENODEV ;
                    }
                }
                break ;
            case bus_parallel:
#ifndef OW_PARPORT
                ret =  -ENOPROTOOPT ;
#else /* OW_PARPORT */
                if ( (ret=DS1410_detect(in)) ) {
                    LEVEL_DEFAULT("Cannot detect the DS1410E parallel adapter\n") ;
                }
#endif /* OW_PARPORT */
                break ;
            case bus_usb:
    #ifdef OW_USB
                ret = DS9490_detect(in) ;
    #else /* OW_USB */
                LEVEL_DEFAULT("Cannot setup USB port. Support not compiled into %s\n",progname) ;
                ret = 1 ;
    #endif /* OW_USB */
                /* in->connin.usb.ds1420_address should be set to identify the
                    * adapter just in case it's disconnected. It's done in the
                    * DS9490_next_both() if not set. */
                // in->name should be set to something, even if DS9490_detect fails
                if(!in->name) in->name = strdup("-1/-1") ;
                break ;
            default:
                break ;
        }
        if ( ret ) { /* flag that that the adapter initiation was unsuccessful */
            STAT_ADD1_BUS(BUS_detect_errors,in) ;
            BUS_close(in) ; /* can use adapter's close */
            BadAdapter_detect(in) ; /* Set to default null assignments */
        }
    } while ( (in=in->next) ) ;
    Asystem.elements = indevices ;

#ifndef __UCLIBC__
    if ( background ) {
 #ifdef HAVE_DAEMON
        if(daemon(1, 0)) {
            LEVEL_DEFAULT("Cannot enter background mode, quitting.\n")
            return 1 ;
        }
#else /* HAVE_DAEMON */
        if(my_daemon(1, 0)) {
            LEVEL_DEFAULT("Cannot enter background mode, quitting.\n")
            return 1 ;
        }
#endif /* HAVE_DAEMON */
        now_background = 1;
    }
#endif /* __UCLIBC__ */

    /* store the PID */
    pid_num = getpid() ;

    if (pid_file) {
        FILE * pid = fopen(  pid_file, "w+" ) ;
        if ( pid == NULL ) {
            LEVEL_CONNECT("Cannot open PID file: %s Error=%s\n",pid_file,strerror(errno) )
            free( pid_file ) ;
            pid_file = NULL ;
            return 1 ;
        }
        fprintf(pid,"%lu",(long unsigned int)pid_num ) ;
        fclose(pid) ;
    }

    /* Use first bus for http bus name */
    SimpleBusName = indevice->name ;
    
    return 0 ;
}

/* All ow library closeup */
void LibClose( void ) {
    if ( pid_file ) {
        if ( unlink( pid_file ) ) LEVEL_CONNECT("Cannot remove PID file: %s error=%s\n",pid_file,strerror(errno))
        free( pid_file ) ;
        pid_file = NULL ;
    }
#ifdef OW_CACHE
    Cache_Close() ;
#endif /* OW_CACHE */
    FreeIn() ;
    FreeOut() ;
    if ( log_available ) closelog() ;
    DeviceDestroy() ;

#ifdef OW_MT
    if ( pmattr ) {
        pthread_mutexattr_destroy(pmattr) ;
        pmattr = NULL ;
    }
#endif /* OW_MT */

    if ( progname && progname[0] ) {
        free(progname) ;
        progname = NULL ;
    }
}

struct s_timeout timeout = {1,DEFAULT_TIMEOUT,10*DEFAULT_TIMEOUT,5*DEFAULT_TIMEOUT,} ;
void Timeout( const char * c ) {
    timeout.vol = strtol( c,NULL,10 ) ;
    if ( errno || timeout.vol<1 ) timeout.vol = DEFAULT_TIMEOUT ;
    timeout.stable = 10*timeout.vol ;
    timeout.dir = 5*timeout.vol ;
}

#if 0
static void segv_handler(int sig) {
    pid_t pid = getpid() ;
#ifdef OW_MT
    pthread_t tid = pthread_self() ;
    (void) sig ;
    LEVEL_DEFAULT("owlib: SIGSEGV received... pid=%d tid=%ld\n", pid, tid)
#else /* OW_MT */
    (void) sig ;
    LEVEL_DEFAULT("owlib: SIGSEGV received... pid=%d\n", pid)
#endif /* OW_MT */
    _exit(1) ;
}
#endif

void set_signal_handlers( void (*exit_handler)(int errcode) ) {
    struct sigaction sa;

    memset(&sa, 0, sizeof(struct sigaction));
    sigemptyset(&(sa.sa_mask));
    sa.sa_flags = 0;

    sa.sa_handler = exit_handler;
    if ((sigaction(SIGHUP,  &sa, NULL) == -1) ||
   (sigaction(SIGINT,  &sa, NULL) == -1) ||
   (sigaction(SIGTERM, &sa, NULL) == -1)) {
        LEVEL_DEFAULT("Cannot set exit signal handlers")
        exit_handler(-1);
    }

#if 0
    sa.sa_handler = segv_handler;
    if (sigaction(SIGSEGV, &sa, NULL) == -1) {
        LEVEL_DEFAULT("Cannot set exit signal handlers" )
        exit_handler(-1);
    }
#endif

    sa.sa_handler = SIG_IGN;
    if(sigaction(SIGPIPE, &sa, NULL) == -1) {
        LEVEL_DEFAULT("Cannot set ignored signals")
        exit_handler(-1);
    }
}
