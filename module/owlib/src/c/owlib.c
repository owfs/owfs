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

#include <config.h>
#include "owfs_config.h"
#include "ow.h"
#include "ow_devices.h"
#include "ow_pid.h"

/* All ow library setup */
void LibSetup( enum opt_program opt ) {
    // global structure of configuration parameters
    memset( &Global, 0, sizeof(struct global) ) ;
    Global.opt = opt ;
    Global.want_background = 1 ;
    Global.SimpleBusName = "None" ;
    Global.max_clients = 250 ;
    
    Global.timeout_volatile =  15 ;
    Global.timeout_stable   = 300 ;
    Global.timeout_directory=  60 ;
    Global.timeout_presence = 120 ;
    Global.timeout_serial   =   5 ;
    Global.timeout_usb      =   5 ;
    Global.timeout_network  =   1 ;
    Global.timeout_server   =  10 ;
    Global.timeout_ftp      = 900 ;

    /* special resort in case static data (devices and filetypes) not properly sorted */
    DeviceSort() ;

    /* DB cache code */
#if OW_CACHE
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
#include <sys/resource.h>
#include <sys/wait.h>

static void catchchild( int sig ) {
    pid_t pid;
    int status;

    pid = wait4(-1, &status, WUNTRACED, 0);
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

    if(!noclose) {
        close(STDIN_FILENO);
        close(STDOUT_FILENO);
        close(STDERR_FILENO);
        if (dup(dup(open("/dev/null", O_APPEND)))==-1){
            perror("dup:");
            return -1;
        }
    }
    return 0;
}
#endif /* HAVE_DAEMON */

#ifdef __UCLIBC__
 #if OW_MT
extern char *__pthread_initial_thread_bos ;
void __pthread_initialize(void) ;
 #endif /* OW_MT */
#endif /* __UCLIBC */

/* Start the owlib process -- actually only tests for backgrounding */
int LibStart( void ) {
    struct connection_in * in = indevice ;
    int ret = 0 ;

#ifdef __UCLIBC__
    /* First call to pthread should be done after daemon() in uClibc, so
     * I moved it here to avoid calling __pthread_initialize() */
    if ( Global.want_background ) {
        switch ( Global.opt) {
            case opt_httpd:
            case opt_ftpd:
            case opt_server:
                 if(
 #ifdef HAVE_DAEMON
                     daemon(1, 0)
 #else /* HAVE_DAEMON */
                     my_daemon(1, 0)
 #endif /* HAVE_DAEMON */
                 ) {
                     LEVEL_DEFAULT("Cannot enter background mode, quitting.\n")
                     return 1 ;
                 }
                 Global.now_background = 1;
            default:
                 break ;
        }
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
    if ( indevice==NULL && !Global.autoserver ) {
    LEVEL_DEFAULT( "No device port/server specified (-d or -u or -s)\n%s -h for help\n",SAFESTRING(Global.progname)) ;
        BadAdapter_detect(NewIn(NULL)) ;
        return 1;
    }
    while (in) {
        BadAdapter_detect(in) ; /* default "NOTSUP" calls */
        switch( get_busmode(in) ) {
            case bus_zero:
            case bus_server:
                if ( (ret = Server_detect(in)) ) {
                    LEVEL_CONNECT("Cannot open server at %s\n",in->name) ;
                }
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
            case bus_i2c:
#if OW_I2C
                if ( DS2482_detect(in) ) {
                    LEVEL_CONNECT("Cannot detect an i2c DS2482-x00 on %s\n",in->name) ;
                    ret = -ENODEV ;
                }
#else /* OW_I2C */
                ret =  -ENOPROTOOPT ;
#endif /* OW_I2C */
                break ;
            case bus_ha7:
#if OW_HA7
                if ( HA7_detect(in) ) {
                    LEVEL_CONNECT("Cannot detect an HA7net server on %s\n",in->name) ;
                    ret = -ENODEV ;
                }
#else /* OW_HA7 */
                ret =  -ENOPROTOOPT ;
#endif /* OW_HA7 */
                break ;
            case bus_parallel:
#if OW_PARPORT
                if ( (ret=DS1410_detect(in)) ) {
                    LEVEL_DEFAULT("Cannot detect the DS1410E parallel adapter\n") ;
                }
#else /* OW_PARPORT */
                ret =  -ENOPROTOOPT ;
#endif /* OW_PARPORT */
                break ;
            case bus_usb:
#if OW_USB
                /* in->connin.usb.ds1420_address should be set to identify the
                    * adapter just in case it's disconnected. It's done in the
                * DS9490_next_both() if not set. */
                if ( (ret = DS9490_detect(in)) ) {
                    LEVEL_DEFAULT("Cannot open USB adapter\n") ;
                }
#else /* OW_USB */
                ret =  -ENOPROTOOPT ;
#endif /* OW_USB */
                break ;
            case bus_link:
                if ( (ret = LINK_detect(in)) ) {
                    LEVEL_CONNECT("Cannot open LINK adapter at %s\n",in->name) ;
                }
                break ;
            case bus_elink:
                if ( (ret = LINKE_detect(in)) ) {
                    LEVEL_CONNECT("Cannot open LINK-HUB-E adapter at %s\n",in->name) ;
                }
                break ;
            case bus_fake:
                Fake_detect(in) ; // never fails
                break ;
            default:
                break ;
        }
        if ( ret ) { /* flag that that the adapter initiation was unsuccessful */
            STAT_ADD1_BUS(BUS_detect_errors,in) ;
            BUS_close(in) ; /* can use adapter's close */
            BadAdapter_detect(in) ; /* Set to default null assignments */
        }
        in = in->next ;
    }

#ifndef __UCLIBC__
    if ( Global.want_background ) {
        switch ( Global.opt) {
            case opt_owfs:
                // handles PID from a callback
                break ;
            case opt_httpd:
            case opt_ftpd:
            case opt_server:
 #ifdef HAVE_DAEMON
                if(daemon(1, 0)) {
                    ERROR_DEFAULT("Cannot enter background mode, quitting.\n")
                    return 1 ;
                }
 #else /* HAVE_DAEMON */
                if(my_daemon(1, 0)) {
                    LEVEL_DEFAULT("Cannot enter background mode, quitting.\n")
                    return 1 ;
                }
 #endif /* HAVE_DAEMON */
                Global.now_background = 1;
                 // fall thought
            default:
                /* store the PID */
                PIDstart() ;
                break ;
        }
    } else { // not background
        if ( Global.opt != opt_owfs ) PIDstart() ;
    }
#endif /* __UCLIBC__ */

    /* Use first bus for http bus name */
    CONNINLOCK ;
        if ( indevice) Global.SimpleBusName = indevice->name ;
    CONNINUNLOCK ;
    // zeroconf/Bonjour look for new services
    if ( Global.autoserver ) OW_Browse() ;

    return 0 ;
}

/* All ow library closeup */
void LibClose( void ) {
    LEVEL_CALL("Starting Library cleanup\n");
    PIDstop() ;
#if OW_CACHE
    LEVEL_CALL("Closing Cache\n");
    Cache_Close() ;
#endif /* OW_CACHE */
    LEVEL_CALL("Closing input devices\n");
    FreeIn() ;
    LEVEL_CALL("Closing outout devices\n");
    FreeOut() ;
    if ( log_available ) closelog() ;
    DeviceDestroy() ;

#if OW_MT
    if ( pmattr ) {
        pthread_mutexattr_destroy(pmattr) ;
        pmattr = NULL ;
    }
#endif /* OW_MT */

    if ( Global.announce_name ) free(Global.announce_name) ;

    if ( Global.progname ) {
        free(Global.progname) ;
    }
#if OW_ZERO
    if ( Global.browse ) DNSServiceRefDeallocate( Global.browse ) ;
#endif /* OW_ZERO */
    LEVEL_CALL("Finished Library cleanup\n");
}

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

    sa.sa_handler = SIG_IGN;
    if(sigaction(SIGPIPE, &sa, NULL) == -1) {
        LEVEL_DEFAULT("Cannot set ignored signals")
        exit_handler(-1);
    }
}
