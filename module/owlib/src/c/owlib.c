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
    start_time = time(NULL) ;

   /* Set up default adapter */
    BadAdapter_detect() ;
}

#ifdef __UCLIBC__

#include <sys/types.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/wait.h>

void catchchild() {
    pid_t pid;
    int status;

    /*signal(SIGCHLD, catchchild);*/ /* Unneeded */

    pid = wait4(-1, &status, WUNTRACED, 0);
#if 0
    if (WIFSTOPPED(status))
        sprintf(buf, "owlib: %d: Child %d stopped\n", getpid(), pid);
    else
        sprintf(buf, "owlib: %d: Child %d died\n", getpid(), pid);
#endif
}

/*
  This is a test to implement daemon() with uClibc and without MMU...
  I'm not sure it works correctly, so don't use it yet.
*/
int my_daemon(int nochdir, int noclose) {
    struct sigaction act;
    int pid;
    int fd;

    //printf("owlib: Warning, my_daemon() is used instead of daemon().\n");
    //printf("owlib: Run application with --foreground instead.\n");

    signal(SIGCHLD, SIG_DFL);

#if defined(__UCLIBC__) && !defined(__UCLIBC_HAS_MMU__)
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
#if defined(__UCLIBC__) && !defined(__UCLIBC_HAS_MMU__)
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
#else
    if(!noclose) {
        close(STDIN_FILENO);
        close(STDOUT_FILENO);
        close(STDERR_FILENO);
        if (dup(dup(open("/dev/null", O_APPEND)))==-1){
            perror("dup:");
            return -1;
        }
    }
#endif
    return 0;
}
#endif /* defined(__UCLIBC__) */

/* Start the owlib process -- actually only tests for backgrounding */
int LibStart( void ) {

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

/** Actually COM and Parallel */
int ComSetup( const char * busdev ) {
    struct stat s ;
    int ret = 0 ;

    if ( devport ) {
        fprintf(stderr,"1-wire port already set to %s, ignoring %s.\n",devport,busdev) ;
        return 0 ;
    }
    if ( (devport=strdup(busdev)) == NULL ) {
        ret = -ENOMEM ;
    } else if ( (ret=stat( devport, &s )) ) {
        syslog( LOG_ERR, "Cannot stat port: %s error=%s\n",devport,strerror(ret)) ;
        ret = -ret ;
    } else if ( ! S_ISCHR(s.st_mode) ) {
        syslog( LOG_INFO , "Not a character device: %s\n",devport) ;
        ret = -EBADF ;
    } else if ((devfd = open(devport, O_RDWR | O_NONBLOCK)) < 0) {
        ret = errno ;
        syslog( LOG_ERR,"Cannot open port: %s error=%s\n",devport,strerror(ret)) ;
        ret = -ret ;
    } else if ( major(s.st_rdev) == 99 ) { /* parport device */
#ifndef OW_PARPORT
        ret =  -ENOPROTOOPT ;
#else /* OW_PARPORT */
        if ( (ret=DS1410_detect()) ) {
            syslog(LOG_WARNING, "Cannot detect the DS1410E parallel adapter\n");
        }
#endif /* OW_PARPORT */
    } else if ( COM_open() ) { /* serial device */
        ret = -ENODEV ;
    } else if ( DS2480_detect() ) { /* Set up DS2480/LINK interface */
        syslog(LOG_WARNING,"Cannot detect DS2480 or LINK interface on %s.\n",devport) ;
        if ( DS9097_detect() ) {
            syslog(LOG_WARNING,"Cannot detect DS9097 (passive) interface on %s.\n",devport) ;
            ret = -ENODEV ;
        }
    }
    if (ret) BadAdapter_detect() ;
    syslog(LOG_INFO,"Interface type = %d on %s\n",Adapter,devport) ;
    return ret ;
}

int USBSetup( void ) {
    int ret ;
#ifdef OW_USB
    if ( devport ) {
        fprintf(stderr,"1-wire port already set to %s, ignoring USB.\n",devport) ;
        ret = -EADDRINUSE ;
    } else {
        ret = DS9490_detect() ;
    }
#else /* OW_USB */
    fprintf(stderr,"Cannot setup USB port properly. See the system log for details\n");
    ret = -ENOPROTOOPT ;
#endif /* OW_USB */
    if ( ret ) BadAdapter_detect() ;
    return ret ;
}

/* All ow library closeup */
void LibClose( void ) {
    if ( pid_file ) {
       if ( unlink( pid_file ) ) syslog(LOG_WARNING,"Cannot remove PID file: %s error=%s\n",pid_file,strerror(errno)) ;
       free( pid_file ) ;
       pid_file = NULL ;
    }
    COM_close() ;
    if ( devport ) {
        free(devport) ;
        devport = NULL ;
    }
#ifdef OW_USB
    DS9490_close() ;
#endif /* OW_USB */
#ifdef OW_CACHE
    Cache_Close() ;
#endif /* OW_CACHE */
    FreeAddr( &server ) ;
    FreeAddr( &client ) ;
    busmode = bus_unknown ;
    closelog() ;
}

struct s_timeout timeout = {1,DEFAULT_TIMEOUT,10*DEFAULT_TIMEOUT,5*DEFAULT_TIMEOUT,} ;
void Timeout( const char * c ) {
    timeout.vol = strtol( c,NULL,10 ) ;
    if ( errno || timeout.vol<1 ) timeout.vol = DEFAULT_TIMEOUT ;
    timeout.stable = 10*timeout.vol ;
    timeout.dir = 5*timeout.vol ;
}

/* Library presence function -- NOP */
void owlib_signature_function( void ) {
}
