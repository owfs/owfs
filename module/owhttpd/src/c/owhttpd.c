/*
$Id$
    OW_HTML -- OWFS used for the web
    OW -- One-Wire filesystem

    Written 2003 Paul H Alfille

 * based on chttpd/1.0
 * main program. Somewhat adapted from dhttpd
 * Copyright (c) 0x7d0 <noop@nwonknu.org>
 * Some parts
 * Copyright (c) 0x7cd David A. Bartold
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include "owfs_config.h"
#include "ow.h" // for libow
#include "owhttpd.h" // httpd-specific

/* ----- Globals ------------- */
#ifdef OW_MT
    pthread_mutex_t accept_mutex = PTHREAD_MUTEX_INITIALIZER ;
    #define ACCEPTLOCK       pthread_mutex_lock(  &accept_mutex) ;
    #define ACCEPTUNLOCK     pthread_mutex_unlock(&accept_mutex) ;
#else /* OW_MT */
    #define ACCEPTLOCK
    #define ACCEPTUNLOCK
#endif /* OW_MT */

#if 0
#ifdef OW_MT
#include <pthread.h>
sem_t accept_sem ;
#define MAX_THREADS 10
pthread_mutex_t thread_lock = PTHREAD_MUTEX_INITIALIZER;
struct mythread {
    pthread_t tid;
    int socket;
    unsigned char avail;
};
struct mythread threads[MAX_THREADS];
#define THREADLOCK      pthread_mutex_lock(&thread_lock);
#define THREADUNLOCK    pthread_mutex_unlock(&thread_lock);

#else /* OW_MT */
#define MAX_THREADS 1
struct mythread {
    int socket;
};
struct mythread threads[MAX_THREADS];
#define THREADLOCK
#define THREADUNLOCK
#endif /* OW_MT */
#endif /* 0 */

/*
 * Default port to listen too. If you aren't root, you'll need it to
 * be > 1024 unless you plan on using a config file
 */
#define DEFAULTPORT    80

struct listen_sock {
    int             socket;
    struct sockaddr_in sin;
};

//static struct listen_sock l_sock;
static void ow_exit( int e ) ;
static void shutdown_sock(struct listen_sock sock) ;
//static int get_listen_sock(struct listen_sock *sock) ;
//static void http_loop( struct listen_sock *sock ) ;
static void * ThreadedAccept( void * pv ) ;
static void * Acceptor( int listenfd ) ;

#if 0
void kill_threads(void) {
#ifdef OW_MT
    int i;
    THREADLOCK
        for (i = 0; i < MAX_THREADS; i++) {
            if(!threads[i].avail) pthread_cancel(threads[i].tid);
        }
    THREADUNLOCK
#endif
    shutdown_sock(l_sock);
    exit(0);
}
#endif /* 0 */

void handle_sighup(int unused) {
    (void) unused ;
//    kill_threads();
}

void handle_sigterm(int unused) {
    (void) unused ;
//    kill_threads();
}

void handle_sigchild(int unused) {
    (void) unused ;
    signal(SIGCHLD, handle_sigchild);
}

int main(int argc, char *argv[]) {
    char c ;
#ifdef OW_MT
    pthread_t thread ;
#ifdef __UCLIBC__
    pthread_mutexattr_t mattr;
    pthread_mutexattr_init(&mattr);
    pthread_mutexattr_settype(&mattr, PTHREAD_MUTEX_ADAPTIVE_NP);
    pthread_mutex_init(&accept_mutex, &mattr);
    pthread_mutexattr_destroy(&mattr);
#else /* __UCLIBC__ */
    pthread_attr_t attr ;

    pthread_attr_init(&attr) ;
    pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_DETACHED) ;
#endif /* __UCLIBC__ */
#endif /* OW_MT */

    LibSetup() ;
    progname = strdup(argv[0]) ;

    while ( (c=getopt_long(argc,argv,OWLIB_OPT,owopts_long,NULL)) != -1 ) {
        switch (c) {
        case 'h':
            fprintf(stderr,
            "Usage: %s ttyDevice -p tcpPort [options] \n"
            "   or: %s [options] -d ttyDevice -p tcpPort \n"
            "    -p port   -- tcp port for web serving process (e.g. 3001)\n" ,
            argv[0],argv[0] ) ;
            break ;
        case 'V':
            fprintf(stderr,
            "%s version:\n\t" VERSION "\n",argv[0] ) ;
            break ;
        }
        if ( owopt(c,optarg) ) ow_exit(0) ; /* rest of message */
    }

    /* non-option arguments */
    if ( optind == argc-1 ) {
        ComSetup(argv[optind]) ;
        ++optind ;
    }

    if ( portname==NULL ) {
        fprintf(stderr, "No TCP port specified (-p)\n%s -h for help\n",argv[0]);
        ow_exit(1);
    }

    if ( ServerAddr( portname, &server ) || (ServerListen( &server )<0) ) {
        fprintf(stderr, "socket problems: %s failed to start\n", progname);
        fprintf(stderr,"Cannot start server.\n");
        exit(1);
    }

    /*
     * Now we drop privledges and become a daemon.
     */
    if ( LibStart() ) ow_exit(1) ;

#if 0
#ifdef OW_MT
    {
      int i;
      for(i=0; i<MAX_THREADS; i++) threads[i].avail = 1;
      sem_init( &accept_sem, 0, MAX_THREADS ) ;
    }
#endif /* OW_MT */
#endif /* 0 */

    signal(SIGPIPE, SIG_IGN);
    signal(SIGCHLD, handle_sigchild);
    signal(SIGHUP, SIG_IGN);
    signal(SIGHUP, handle_sighup);
    signal(SIGTERM, handle_sigterm);

/*
    for (;;) {
        http_loop(&l_sock) ;
    }
    kill_threads();
*/
    for(;;) {
        ACCEPTLOCK
#ifdef OW_MT
#ifdef __UCLIBC__
        if ( pthread_create( &thread, NULL, ThreadedAccept, &server ) ) ow_exit(1) ;
	pthread_detach(thread);
#else /* __UCLIBC__ */
        if ( pthread_create( &thread, &attr, ThreadedAccept, &server ) ) ow_exit(1) ;
#endif /* __UCLIBC__ */
#else /* OW_MT */
        Acceptor( server.listenfd ) ;
#endif /* OW_MT */
    }
}

static void * ThreadedAccept( void * pv ) {
    return (void *) Acceptor( ((struct network_work *)pv)->listenfd ) ;
}

static void * Acceptor( int listenfd ) {
    int fd = accept( listenfd, NULL, NULL ) ;
    ACCEPTUNLOCK
    if ( fd>=0 ) {
        FILE * fp = fdopen(fd, "w+");
        if (fp) {
            handle_socket( fp ) ;
            fflush(fp);
        }
        close(fd);
    }
    return NULL ;
}

#if 0
static void * handle(void * ptr) {
    struct mythread *mt = ((struct mythread *)ptr);
    struct active_sock ch_sock;

#ifdef OW_MT
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
#endif /* OW_MT */

    ch_sock.socket = mt->socket;
    ch_sock.io = fdopen(ch_sock.socket, "w+");
    if (ch_sock.io) {
        handle_socket( ch_sock.io ) ;
        fflush(ch_sock.io);
    }
    close(ch_sock.socket);
#ifdef OW_MT
    THREADLOCK
        mt->avail = 1;
        mt->socket = 0;
    THREADUNLOCK
    sem_post(&accept_sem);
    pthread_exit(NULL);
#endif /* OW_MT */
    return NULL ;
}

#ifdef OW_MT
static int start_thread(struct mythread *mt) {
    sigset_t oldset;
    sigset_t newset;
    int res;

    /* Disallow signal reception in worker threads */
    sigfillset(&newset);
    pthread_sigmask(SIG_SETMASK, &newset, &oldset);
    res = pthread_create(&mt->tid, NULL, handle, mt);
    pthread_sigmask(SIG_SETMASK, &oldset, NULL);
    if (res != 0) {
        fprintf(stderr, "start_thread: error creating thread: %s\n", strerror(res));
        return -1;
    }
#ifdef DEBUG_SYSLOG
    /* uClibc-0.9.19 which WRT54G use, seem to have a buggy syslog() function.
       If syslogd is NOT started, syslog() hangs in the new thread (in ow_read.c)
       If I add the syslog below (after creating the new thread), it doesn't hang in ow_read.c?
       If syslogd is running the problem doesn't occour, but since WRT54G doesn't have any
         syslogd as default, it's better to remove syslog() when version <= 0.9.19.
    */
    //syslog(LOG_WARNING,"This solves the problem of hanging syslog() in the new thread???\n");
#endif /* DEBUG_SYSLOG */

    pthread_detach(mt->tid);
    return 0;
}
#endif /* OW_MT */

static void http_loop( struct listen_sock *sock ) {
    socklen_t size = sizeof((sock->sin));
    int n_sock = accept((sock->socket),(struct sockaddr*)&(sock->sin), &size);
    if (n_sock != -1) {
#ifdef OW_MT
        sem_wait(&accept_sem);
        if (multithreading) {
            int res, i;
            THREADLOCK
                for(i=0; i<MAX_THREADS; i++) if(threads[i].avail) break;
                threads[i].avail = 0;
                threads[i].socket = n_sock;
            THREADUNLOCK
            res = start_thread(&threads[i]);
            if (res == -1) {
                //shutdown(n_sock, SHUT_RDWR);
                close(n_sock);
                THREADLOCK
                    threads[i].avail = 1;
                THREADUNLOCK
                sem_post(&accept_sem);
            }
        } else {
            threads[0].avail = 0;
            threads[0].socket = n_sock;
            handle(&threads[0]);
        }
#else /* OW_MT */
        threads[0].socket = n_sock;
        handle(&threads[0]);
#endif /* OW_MT */
    } else {
        /* failed to accept */
        syslog(LOG_WARNING,"Failed to accept a socket\n");
    }
}
#endif /* 0 */

static void ow_exit( int e ) {
    LibClose() ;
    closelog() ;
    exit( e ) ;
}

