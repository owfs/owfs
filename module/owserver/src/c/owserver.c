/*
$Id$
    OW_HTML -- OWFS used for the web
    OW -- One-Wire filesystem

    Written 2004 Paul H Alfille

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

/* owserver -- responds to requests over a network socket, and processes them on the 1-wire bus/
         Basic idea: control the 1-wire bus and answer queries over a network socket
         Clients can be owperl, owfs, owhttpd, etc...
         Clients can be local or remote
                 Eventually will also allow bounce servers.

         syntax:
                 owserver
                 -u (usb)
                 -d /dev/ttyS1 (serial)
                 -p tcp port
                 e.g. 3001 or 10.183.180.101:3001 or /tmp/1wire
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

pthread_mutex_t accept_lock = PTHREAD_MUTEX_INITIALIZER;
#define ACCEPTLOCK      pthread_mutex_lock(&accept_lock);
#define ACCEPTUNLOCK    pthread_mutex_unlock(&accept_lock);


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

struct lsock {
    int fd ;
    struct sockaddr addr ;
    socklen_t len ;
};

/* Message preamble */
struct msg_in {
    int size ;
    int tempscale ;
    int operation ;
} ;

/* Message with small(minimum) data field */
struct msg_in_data {
    struct msg_in in ;
    unsigned char data[32] ;
} ;

/* Message preamble */
struct msg_out {
    int size ;
    int last ;
    int error ;
} ;

/* Message with small(minimum) data field */
struct msg_out_data {
    struct msg_out out ;
    unsigned char data[32] ;
} ;

static void ow_exit( int e ) ;
static void shutdown_sock(struct listen_sock sock) ;
static int get_listen_sock(struct listen_sock *sock) ;
static void http_loop( struct listen_sock *sock ) ;

void handle_sighup(int unused) {
    (void) unused ;
}

void handle_sigterm(int unused) {
    (void) unused ;
}

void handle_sigchild(int unused) {
    (void) unused ;
    signal(SIGCHLD, handle_sigchild);
}

int main(int argc, char *argv[]) {
    char c ;
    struct addrinfo hints ;
    struct addrinfo * ai ;
    char * port = NULL ;
    int listenfd ;
    int on=1 ;
#ifdef OW_MT
    pthread_t thread ;
#endif /* OW_MT */

    bzero( hints, sizeof(struct addrinfo) ) ;
    hints.ai_family = AF_UNSPEC ;
    hints.ai_socktype = SOCK_STREAM ;

    LibSetup() ;

    while ( (c=getopt_long(argc,argv,OWLIB_OPT,owopts_long,NULL)) != -1 ) {
        switch (c) {
        case 'h':
            fprintf(stderr,
            "Usage: %s ttyDevice -p tcpPort [options] \n"
            "   or: %s [options] -d ttyDevice -p tcpPort \n"
            "   or: %s [options] -u -p tcpPort \n"
            "    -p port   -- tcp port for listening (e.g. 3001)\n" ,
            argv[0],argv[0] ) ;
            break ;
        case 'V':
            fprintf(stderr,
            "%s version:\n\t" VERSION "\n",argv[0] ) ;
            break ;
        case 'p':
            port = optarg ;
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

    if ( port ) {
        fprintf(stderr, "No TCP port specified (-p)\n%s -h for help\n",argv[0]);
        ow_exit(1);
    }

    /*
    * Now we drop privledges and become a daemon.
    */
    if ( LibStart() ) ow_exit(1) ;

    if ( getaddrinfo(NULL,port,&hints,&ai) ) {
        syslog(LOG_WARNING,"Port (%s) address error %s",port,gai_strerror()) ;
        ow_exit(1);
    }

    if ( (listenfd=socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol)) == -1 ) {
        syslog(LOG_WARNING,"Could not create socket") ;
        ow_exit(1);
    }

    if ( setsockopt(listenfd, SOL_SOCKET,SO_REUSEADDR,&on,sizeof(on)) ) {
        syslog(LOG_WARNING,"Socket options error") ;
        ow_exit(1) ;
    }

    if ( bind(listenfd,ai->ai_addr,ai->ai_addrlen) ) {
        syslog(LOG_WARNING,"Could not bind socket") ;
        ow_exit(1);
    }

    if ( listen(listenfd,20) ) {
        syslog(LOG_WARNING,"Could not listen to socket") ;
        ow_exit(1) ;
    }

    freeaddrinfo(ai) ;

    signal(SIGCHLD, handle_sigchild);
    signal(SIGHUP, SIG_IGN);
    signal(SIGHUP, handle_sighup);
    signal(SIGTERM, handle_sigterm);

#ifdef OW_MT
    ACCEPTLOCK /* Start with lock of loop, thread will unlock after accept */
    while ( pthread_create(&thread,PTHREAD_CREATE_DETACHED,acceptor,(void *)listenfd) == 0 ) {
        ACCEPTLOCK
    }
    syslog(LOG_WARNING,"Could not create thread") ;
#else /* OW_MT */
    for(;;) acceptor((void *)listenfd) ;
#endif /* OW_MT */
}

void * acceptor( void * fd ) {
    struct sockaddr_storage sa ;
    socklen_t len = sizeof(struct sockaddr_storage) ;
    int acceptfd = accept((int)fd,&sa,&len);
    struct msg_in_data in ;
    unsigned char * pin = in.data ;
    struct msg_out_data out ;
    unsigned char * pout = out.data ;
    int lin, lout ;

#ifdef OW_MT
    ACCEPTUNLOCK /* Allow main loop to create next thread */
#endif /* OW_MT */
    if (acceptfd==-1) return NULL ;
    lin = readn( acceptfd, &in, sizeof(struct msg_in_data) ) ;
    if ( lin < sizeof( struct msg_in_data ) ) {
    } else {
        if ( ntohl(in.in.size)>sizeof(struct msg_in_data ) ) {
            if ( (pin=malloc( ntohl(in.in.size)-sizeof(struct msg_in))) ) {
                memcpy( pin, in.data, sizeof(in.data) ) ;

            }

    }
    close(acceptfd) ;
}

/* Read a message, return a pointer that must be free-ed */
/* Null return is an error -- bail */
static void * ReadMsg( int acceptfd ) {
    int size ;
    int
    void * msg = malloc( sizeof( struct msg_in_data ) ) ;
    if (msg == NULL) return NULL ;



/* Read "n" bytes from a descriptor. */
/* "borrowed" from Stevens */
int readn(int fd, void *vptr, size_t n) {
    size_t  nleft;
    ssize_t nread;
    char    *ptr;

    ptr = vptr;
    nleft = n;
    while (nleft > 0) {
        if ( (nread = read(fd, ptr, nleft)) < 0) {
            if (errno == EINTR)
                nread = 0;              /* and call read() again */
            else
                return(-1);
        } else if (nread == 0)
            break;                          /* EOF */

            nleft -= nread;
            ptr   += nread;
    }
    return(n - nleft);              /* return >= 0 */
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


