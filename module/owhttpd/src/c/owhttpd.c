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

/*
 * Default port to listen too. If you aren't root, you'll need it to
 * be > 1024 unless you plan on using a config file
 */
#define DEFAULTPORT    80

struct listen_sock {
    int             socket;
    struct sockaddr_in sin;
};

static struct listen_sock l_sock;
static void ow_exit( int e ) ;
static void shutdown_sock(struct listen_sock sock) ;
static int get_listen_sock(struct listen_sock *sock) ;
static void http_loop( struct listen_sock *sock ) ;

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

void handle_sighup(int unused) {
    (void) unused ;
    kill_threads();
}

void handle_sigterm(int unused) {
    (void) unused ;
    kill_threads();
}

void handle_sigchild(int unused) {
    (void) unused ;
    signal(SIGCHLD, handle_sigchild);
}

int main(int argc, char *argv[]) {
    char c ;

    LibSetup() ;

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

    if ( devfd==-1 && devusb==0 ) {
        fprintf(stderr, "No device port specified (-d or -u)\n%s -h for help\n",argv[0]);
        ow_exit(1);
    }

    if ( portnum==-1 ) {
        fprintf(stderr, "No TCP port specified (-p)\n%s -h for help\n",argv[0]);
        ow_exit(1);
    }

    if (get_listen_sock(&l_sock) < 0) {
        fprintf(stderr, "socket problems: %s failed to start\n", argv[0]);
        ow_exit(1);
    }

    /*
     * Now we drop privledges and become a daemon.
     */
    if ( LibStart() ) ow_exit(1) ;

#ifdef OW_MT
    {
      int i;
      for(i=0; i<MAX_THREADS; i++) threads[i].avail = 1;
      sem_init( &accept_sem, 0, MAX_THREADS ) ;
    }
#endif

    signal(SIGCHLD, handle_sigchild);
    signal(SIGHUP, SIG_IGN);
    signal(SIGHUP, handle_sighup);
    signal(SIGTERM, handle_sigterm);

    for (;;) {
        http_loop(&l_sock) ;
    }
    kill_threads();
}

static void * handle(void * ptr) {
    struct mythread *mt = ((struct mythread *)ptr);
    struct active_sock ch_sock;

#ifdef OW_MT
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
#endif

    ch_sock.socket = mt->socket;
    ch_sock.io = fdopen(ch_sock.socket, "w+");
    if (ch_sock.io) {
        handle_socket( &ch_sock ) ;
        fflush(ch_sock.io);
        fclose(ch_sock.io);
        shutdown(ch_sock.socket, SHUT_RDWR);
    }
    close(ch_sock.socket);
#ifdef OW_MT
    THREADLOCK
        mt->avail = 1;
        mt->socket = 0;
    THREADUNLOCK
    sem_post(&accept_sem);
    pthread_exit(NULL);
#endif
    return NULL ;
}

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
    pthread_detach(mt->tid);
    return 0;
}

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

static void ow_exit( int e ) {
    LibClose() ;
    closelog() ;
    exit( e ) ;
}

/* -------------------------------------------------------------------
   ---------------- socket routines ----------------------------------
   ------------------------------------------------------------------- */
/*
 * get_listen_sock
 *   Start up the socket listening routines and return a filedescriptor
 *   for that socket
 */
static int get_listen_sock(struct listen_sock *sock) {
    int             status;
    int             one;

    status = 0;
    one = 1;
    memset(&(sock->sin), 0, sizeof(sock->sin));
    sock->sin.sin_family = AF_INET;
    sock->sin.sin_addr.s_addr = htonl(0);
    sock->sin.sin_port = htons(portnum);

    if ( ((sock->socket = socket(AF_INET, SOCK_STREAM, 0))==-1)
         || setsockopt(sock->socket, SOL_SOCKET, SO_REUSEADDR, (char *) &one, sizeof(one))
         || bind(sock->socket, (struct sockaddr *) &(sock->sin), sizeof((sock->sin)))
         || listen(sock->socket, 5)
       ) {
        shutdown(sock->socket, 2);
        sock->socket = -1;
    }
    return (sock->socket);
}

static void shutdown_sock(struct listen_sock sock) {
    if (sock.socket != -1)
        shutdown(sock.socket, 2);
}
