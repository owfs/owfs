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

/* ow_net holds the network utility routines. Many stolen unashamedly from Steven's Book */

#include "owfs_config.h"
#include "ow.h"
//#include <sys/uio.h>

/* Read "n" bytes from a descriptor. */
/* Stolen from Unix Network Programming by Stevens, Fenner, Rudoff p89 */
ssize_t readn(int fd, void *vptr, size_t n) {
    size_t	nleft;
    ssize_t	nread;
    char	*ptr;

    ptr = vptr;
    nleft = n;
    while (nleft > 0) {
        if ( (nread = read(fd, ptr, nleft)) < 0) {
            if (errno == EINTR)
                nread = 0; /* and call read() again */
            else
                return(-1);
        } else if (nread == 0)
            break; /* EOF */
        nleft -= nread ;
        ptr   += nread;
    }
    return(n - nleft); /* return >= 0 */
}

int ServerAddr(  struct connection_out * out ) {
    struct addrinfo hint ;
    int ret ;
    char * p ;

    if ( out->name == NULL ) return -1 ;
    if ( (p=strrchr(out->name,':')) ) { /* : exists */
        *p = '\0' ; /* Separate tokens in the string */
        out->host = strdup(out->name) ;
        out->service = strdup(&p[1]) ;
    } else {
        out->host = NULL ;
        out->service = strdup(out->name) ;
    }

    bzero( &hint, sizeof(struct addrinfo) ) ;
    hint.ai_flags = AI_PASSIVE ;
    hint.ai_socktype = SOCK_STREAM ;
    hint.ai_family = AF_UNSPEC ;

    //printf("ServerAddr: [%s] [%s]\n", out->host, out->service);

    if ( (ret=getaddrinfo( out->host, out->service, &hint, &out->ai )) ) {
        fprintf(stderr,"GetAddrInfo error %s\n",gai_strerror(ret));
        return -1 ;
    }
    return 0 ;
}

int ServerListen( struct connection_out * out ) {
    int fd ;
    int on = 1 ;
    int ret;

    if ( out->ai == NULL ) {
        fprintf(stderr,"Server address not yet parsed\n");
        return -1 ;
    }

    if ( out->ai_ok == NULL ) out->ai_ok = out->ai ;
    do {
      //printf("ServerListen: out->ai_ok = %p\n", out->ai_ok);
        fd = socket(
            out->ai_ok->ai_family,
            out->ai_ok->ai_socktype,
            out->ai_ok->ai_protocol
        ) ;
        if ( fd >= 0 ) {
            ret = setsockopt(fd,SOL_SOCKET,SO_REUSEADDR,&on,sizeof(on))
                || bind( fd, out->ai_ok->ai_addr, out->ai_ok->ai_addrlen )
                || listen(fd, 10) ;
            if ( ret ) {
                fprintf(stderr, "ServerListen: Socket problem [%s]\n", strerror(errno));
                close( fd ) ;
            } else {
                out->fd = fd ;
                return fd ;
            }
        } else {
            fprintf(stderr, "ServerListen: socket() errno=%d [%s]", errno, strerror(errno));
        }
    } while ( (out->ai_ok=out->ai_ok->ai_next) ) ;
    fprintf(stderr,"ServerListen: Socket problem errno=%d\n",errno) ;
    return -1 ;
}

int ClientAddr(  char * sname, struct connection_in * in ) {
    struct addrinfo hint ;
    char * p ;
    int ret ;

//printf("ClientAddr port=%s\n",sname);
    if ( sname == NULL ) return -1 ;
    if ( (p=strrchr(sname,':')) ) { /* : exists */
        *p = '\0' ; /* Separate tokens in the string */
        in->host = strdup(sname) ;
        in->service = strdup(&p[1]) ;
    } else {
        in->host = NULL ;
        in->service = strdup(sname) ;
    }

    bzero( &hint, sizeof(struct addrinfo) ) ;
    hint.ai_socktype = SOCK_STREAM ;
    hint.ai_family = AF_UNSPEC ;

//printf("ClientAddr: [%s] [%s]\n", in->host, in->service);

    if ( (ret=getaddrinfo( in->host, in->service, &hint, &in->ai )) ) {
//printf("GetAddrInfo ret=%d\n",ret);
        fprintf(stderr,"GetAddrInfo error %s\n",gai_strerror(ret));
        return -1 ;
    }
    return 0 ;
}

int ClientConnect( struct connection_in * in ) {
    int fd ;

    if ( in->ai == NULL ) {
        fprintf(stderr,"Client address not yet parsed\n");
        return -1 ;
    }

    if ( in->ai_ok == NULL ) in->ai_ok = in->ai ;
    do {
        fd = socket(
            in->ai_ok->ai_family,
            in->ai_ok->ai_socktype,
            in->ai_ok->ai_protocol
        ) ;
        if ( fd >= 0 ) {
            if ( connect(fd, in->ai_ok->ai_addr, in->ai_ok->ai_addrlen) == 0 ) {
                return fd ;
            }
            close( fd ) ;
        }
    } while ( (in->ai_ok=in->ai_ok->ai_next) ) ;
    fprintf(stderr,"ClientConnect: Socket problem errno=%d\n",errno) ;
    return -1 ;
}

/*
 Heap big magic!
 Doubly embedded function.
 Who says programming can't be fun.
 Loops through outdevices, starting a detached thread for each except the last
 Each loop spawn threads for accepting connections
 Uses my non-patented "pre-threaded technique"
 
 VALGRIND's mutexes doesn't handle locks/unlocks in different processes
 so I added this define just to temporary be able to search for memory
 leaks... The new accept() is not detached just to be able to wait
 for thread to end.
*/
//#define VALGRIND 1
void ServerProcess( void (*HandlerRoutine)(int fd), void (*Exit)(int errcode) ) {
    struct connection_out * out = outdevice ;
    struct connection_out * out_last = NULL;
    pthread_t thread ;
#ifndef __UCLIBC__
    pthread_attr_t attr ;
#endif
    /* embedded function */
    void ToListen( struct connection_out * o ) {
        if ( ServerAddr( o ) || (ServerListen( o )<0) ) {
            syslog(LOG_WARNING,"Cannot start server = %s\n",o->name);
            Exit(1);
        }
    }
    /* embedded function */
    void RunAccepted( int rafd ) {
        if ( rafd>=0 ) {
            HandlerRoutine( rafd ) ;
            close( rafd ) ;
        }
    }
#ifdef OW_MT
    /* Embedded function */
    void * ConnectionThread( void * v ) {
        struct connection_out * out2 = (struct connection_out *)v ;
        pthread_t thread2 ;
        /* Doubly Embedded function */
        void * AcceptThread( void * v2 ) {
            int acceptfd ;
	    struct connection_out *o2 = (struct connection_out *)v2;
            //printf("ACCEPT thread=%ld waiting\n",pthread_self()) ;
            acceptfd = accept( o2->fd, NULL, NULL ) ;
            //printf("ACCEPT thread=%ld accepted fd=%d\n",pthread_self(),acceptfd) ;
            ACCEPTUNLOCK(o2)
            //printf("ACCEPT thread=%ld unlocked\n",pthread_self()) ;
            RunAccepted( acceptfd ) ;
#ifndef VALGRIND
            pthread_exit((void *)0);
#else
	    return NULL;
#endif
        }
        ToListen( out2 ) ;
        for(;;) {
#ifdef VALGRIND
            void *v;
#endif
            ACCEPTLOCK(out2)
#ifdef __UCLIBC__
            if ( pthread_create( &thread2, NULL, AcceptThread, (void *)out2 ) ) Exit(1) ;
#ifndef VALGRIND
            pthread_detach(thread2);
#endif
#else
            if ( pthread_create( &thread2, &attr, AcceptThread, (void *)out2 ) ) Exit(1) ;
#endif
#ifdef VALGRIND
	    pthread_join(thread2, &v);
#endif
        }
        if(out == out_last) {
            /* last connection_out wasn't a separate thread */
            return NULL;
        } else {
            pthread_exit((void *)0);
        }
    }

#ifndef __UCLIBC__
    pthread_attr_init(&attr) ;
#ifndef VALGRIND
    pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_DETACHED) ;
#endif
#endif

    /* find the last outdevice to make sure embedded function
     * return currect value */
    out_last = outdevice;
    while ( out_last->next ) out_last = out_last->next ;

    while ( out->next ) {
#ifdef __UCLIBC__
        if ( pthread_create( &thread, NULL, ConnectionThread, out) ) Exit(1) ;
        pthread_detach(thread);
#else
        if ( pthread_create( &thread, &attr, ConnectionThread, out) ) Exit(1) ;
#endif
        out = out->next ;
    }
    ConnectionThread( out ) ;
#else /* OW_MT */
    ToListen( out ) ;
    for ( ;; ) RunAccepted( accept(outdevice->fd,NULL,NULL) ) ;
#endif /* OW_MT */
}
