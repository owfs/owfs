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

    if ( (ret=getaddrinfo( out->host, out->service, &hint, &out->ai )) ) {
        fprintf(stderr,"GetAddrInfo error %s\n",gai_strerror(ret));
        return -1 ;
    }
    return 0 ;
}

int ServerListen( struct connection_out * out ) {
    int fd ;
    int on = 1 ;

    if ( out->ai == NULL ) {
        fprintf(stderr,"Server address not yet parsed\n");
        return -1 ;
    }

    if ( out->ai_ok == NULL ) out->ai_ok = out->ai ;
    do {
        fd = socket(
            out->ai_ok->ai_family,
            out->ai_ok->ai_socktype,
            out->ai_ok->ai_protocol
        ) ;
        if ( fd >= 0 ) {
            setsockopt(fd,SOL_SOCKET,SO_REUSEADDR,&on,sizeof(on)) ;
            if ( bind( fd, out->ai_ok->ai_addr, out->ai_ok->ai_addrlen )
               || listen(fd, 100)
            ) {
                close( fd ) ;
            } else {
                out->fd = fd ;
                return fd ;
            }
        }
    } while ( (out->ai_ok=out->ai_ok->ai_next) ) ;
    fprintf(stderr,"Socket problem errno=%d\n",errno) ;
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
    fprintf(stderr,"Socket problem errno=%d\n",errno) ;
    return -1 ;
}

/*
 Heap big magic!
 Doubly embedded function.
 Who says programming can't be fun.
 Loops through outdevices, starting a detached thread for each except the last
 Each loop spawn threads for accepting connections
 Uses my non-patented "pre-threaded technique"
*/
void ServerProcess( void (*HandlerRoutine)(int fd), void (*Exit)(int errcode) ) {
    struct connection_out * out = outdevice ;
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
    pthread_t thread ;
#ifndef __UCLIBC__
    pthread_attr_t attr ;

    pthread_attr_init(&attr) ;
    pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_DETACHED) ;
#endif
    /* Embedded function */
    void * ConnectionThread( void * v ) {
        struct connection_out * out2 = out ;
        pthread_t thread2 ;
        /* Doubly Embedded function */
        void * AcceptThread( void * v2 ) {
            int acceptfd ;
            (void) v2 ;
            //printf("ACCEPT thread=%ld waiting\n",pthread_self()) ;
            acceptfd = accept( out2->fd, NULL, NULL ) ;
            //printf("ACCEPT thread=%ld accepted fd=%d\n",pthread_self(),acceptfd) ;
            ACCEPTUNLOCK(out2)
            //printf("ACCEPT thread=%ld unlocked\n",pthread_self()) ;
            RunAccepted( acceptfd ) ;
            return NULL ;
        }
        (void) v ;
        ToListen( out2 ) ;
        for(;;) {
            ACCEPTLOCK(out2)
#ifdef __UCLIBC__
            if ( pthread_create( &thread2, NULL, AcceptThread, NULL ) ) Exit(1) ;
            pthread_detach(thread2);
#else
            if ( pthread_create( &thread2, &attr, AcceptThread, NULL ) ) Exit(1) ;
#endif
        }
    }
    while ( out->next ) {
#ifdef __UCLIBC__
        if ( pthread_create( &thread, NULL, ConnectionThread, NULL ) ) Exit(1) ;
        pthread_detach(thread);
#else
        if ( pthread_create( &thread, &attr, ConnectionThread, NULL ) ) Exit(1) ;
#endif
        out = out->next ;
    }
    ConnectionThread( NULL ) ;
#else /* OW_MT */
    ToListen( out ) ;
    for ( ;; ) RunAccepted( accept(outdevice->fd,NULL,NULL) ) ;
#endif /* OW_MT */
}
