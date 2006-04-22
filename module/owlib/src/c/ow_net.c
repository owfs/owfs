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
/* Much modification by Christian Magnusson especially for Valgrind and embedded */
/* non-threaded fixes by Jerry Scharf */

#include "owfs_config.h"
#include "ow.h"
#include "ow_counters.h"
#include "ow_connection.h"

/* structures */
struct AcceptThread_data {
    struct connection_out *o2;
    void (*HandlerRoutine)(int fd);
} ;

/* Prototypes */
static void RunAccepted( int rafd, void (*HandlerRoutine)(int fd) ) ;
static void * AcceptThread( void * v2 ) ;

/* Read "n" bytes from a descriptor. */
/* Stolen from Unix Network Programming by Stevens, Fenner, Rudoff p89 */
ssize_t readn(int fd, void *vptr, size_t n, const struct timeval * ptv ) {
    size_t	nleft;
    ssize_t	nread;
    char	*ptr;
    //printf("NetRead attempt %d bytes Time:(%ld,%ld)\n",n,ptv->tv_sec,ptv->tv_usec ) ;
    ptr = vptr;
    nleft = n;
    while (nleft > 0) {
        int         rc ;
        fd_set      readset;
        struct timeval tv = { ptv->tv_sec, ptv->tv_usec, } ;

        /* Initialize readset */
        FD_ZERO(&readset);
        FD_SET(fd, &readset);

        /* Read if it doesn't timeout first */
        rc = select( fd+1, &readset, NULL, NULL, &tv );
        if( rc > 0 ) {

            /* Is there something to read? */
            if( FD_ISSET( fd, &readset )==0 ) {
//                  STAT_ADD1_BUS(BUS_read_select_errors,pn->in);
                  return -EIO ; /* error */
            }
//                    update_max_delay(pn);
            if ( (nread = read(fd, ptr, nleft)) < 0) {
                if (errno == EINTR) {
                    errno = 0; // clear errno. We never use it anyway.
                    nread = 0; /* and call read() again */
                } else {
                    ERROR_DATA("Network data read error\n") ;
                    STAT_ADD1(NET_read_errors);
                    return(-1);
                }
            } else if (nread == 0) {
                break; /* EOF */
            }
            //{ int i ; for ( i=0 ; i<nread ; ++i ) printf("%.2X ",ptr[i]) ; printf("\n") ; }
            nleft -= nread ;
            ptr   += nread;
        } else if(rc < 0) { /* select error */
            if(errno == EINTR) {
                /* select() was interrupted, try again */
//                STAT_ADD1_BUS(BUS_read_interrupt_errors,pn->in);
                continue;
            }
            ERROR_DATA("Selection error (network)\n") ;
//            STAT_ADD1_BUS(BUS_read_select_errors,pn->in);
            return -EINTR;
        } else { /* timed out */
            LEVEL_CONNECT("TIMEOUT after %d bytes\n",n-nleft);
//            STAT_ADD1_BUS(BUS_read_timeout_errors,pn->in);
            return -EAGAIN;
        }
    }
    return(n - nleft); /* return >= 0 */
}

int ServerAddr(  struct connection_out * out ) {
    struct addrinfo hint ;
    int ret ;
    char * p ;

    if ( out->name == NULL ) return -1 ;
    if ( (p=strrchr(out->name,':')) ) { /* : exists */
        p[0] = '\0' ; /* Separate tokens in the string */
        out->host = strdup(out->name) ;
        out->service = strdup(&p[1]) ;
        p[0] = ';' ; /* Restore the string */
    } else {
        out->host = NULL ;
        out->service = strdup(out->name) ;
    }

    memset( &hint, 0, sizeof(struct addrinfo) ) ;
    hint.ai_flags = AI_PASSIVE ;
    hint.ai_socktype = SOCK_STREAM ;
    hint.ai_family = AF_UNSPEC ;

    //printf("ServerAddr: [%s] [%s]\n", out->host, out->service);

    if ( (ret=getaddrinfo( out->host, out->service, &hint, &out->ai )) ) {
        LEVEL_CONNECT("GetAddrInfo error %s\n",gai_strerror(ret));
        return -1 ;
    }
    return 0 ;
}

int ServerListen( struct connection_out * out ) {
    int fd ;
    int on = 1 ;
    int ret;

    if ( out->ai == NULL ) {
        LEVEL_CONNECT("Server address not yet parsed\n");
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
            ret = setsockopt(fd,SOL_SOCKET,SO_REUSEADDR,(char *)&on,sizeof(on))
                || bind( fd, out->ai_ok->ai_addr, out->ai_ok->ai_addrlen )
                || listen(fd, 10) ;
            if ( ret ) {
                LEVEL_CONNECT("ServerListen: Socket problem [%s]\n", strerror(errno));
                close( fd ) ;
            } else {
                out->fd = fd ;
                return fd ;
            }
        } else {
            LEVEL_CONNECT("ServerListen: socket() [%s]", strerror(errno));
        }
    } while ( (out->ai_ok=out->ai_ok->ai_next) ) ;
    LEVEL_CONNECT("ServerListen: Socket problem [%s]\n", strerror(errno)) ;
    return -1 ;
}

int ClientAddr(  char * sname, struct connection_in * in ) {
    struct addrinfo hint ;
    char * p ;
    int ret ;

    if ( sname == NULL ) return -1 ;
    if ( (p=strrchr(sname,':')) ) { /* : exists */
        p[0] = '\0' ; /* Separate tokens in the string */
        in->connin.server.host = strdup(sname) ;
        in->connin.server.service = strdup(&p[1]) ;
        p[0] = ':' ; /* restore name string */
    } else {
        in->connin.server.host = NULL ;
        in->connin.server.service = strdup(sname) ;
    }
    
    memset( &hint, 0, sizeof(struct addrinfo) ) ;
    hint.ai_socktype = SOCK_STREAM ;
    hint.ai_family = AF_UNSPEC ;

//printf("ClientAddr: [%s] [%s]\n", in->connin.server.host, in->connin.server.service);

    if ( (ret=getaddrinfo( in->connin.server.host, in->connin.server.service, &hint, &in->connin.server.ai )) ) {
        LEVEL_CONNECT("GetAddrInfo error %s\n",gai_strerror(ret));
        return -1 ;
    }
    return 0 ;
}

void FreeClientAddr(  struct connection_in * in ) {
    if ( in->connin.server.host ) {
        free(in->connin.server.host) ;
        in->connin.server.host = NULL ;
    }
    if ( in->connin.server.service ) {
        free(in->connin.server.service) ;
        in->connin.server.service = NULL ;
    }
    if ( in->connin.server.ai ) {
        freeaddrinfo( in->connin.server.ai ) ;
        in->connin.server.ai = NULL ;
    }
}

int ClientConnect( struct connection_in * in ) {
    int fd ;
    struct addrinfo *ai ;

    if(!in) {
      //printf("ClientConnect: in is NULL\n");
      return -1 ;
    }

    if ( in->connin.server.ai == NULL ) {
        LEVEL_CONNECT("Client address not yet parsed\n");
        return -1 ;
    }

    /* Can't change ai_ok without locking the in-device.
     * First try the last working address info, if it fails lock
     * the in-device and loop through the list until it works.
     * Not a perfect solution, but it should work at least.
     */
    INBUSLOCK(in);
    ai = in->connin.server.ai_ok ;
    if( ai ) {
        INBUSUNLOCK(in);
        fd = socket(
            ai->ai_family,
            ai->ai_socktype,
            ai->ai_protocol
            ) ;
        if ( fd >= 0 ) {
            if ( connect(fd, ai->ai_addr, ai->ai_addrlen) == 0 ) return fd ;
            close( fd ) ;
        }
        INBUSLOCK(in);
    }

    ai = in->connin.server.ai ;  // loop from first address info since it failed.
    do {
        fd = socket(
            ai->ai_family,
            ai->ai_socktype,
            ai->ai_protocol
            ) ;
        if ( fd >= 0 ) {
            if ( connect(fd, ai->ai_addr, ai->ai_addrlen) == 0 ) {
                in->connin.server.ai_ok = ai ;
                INBUSUNLOCK(in);
                return fd ;
            }
            close( fd ) ;
        }
    } while ( (ai = ai->ai_next) ) ;
    in->connin.server.ai_ok = NULL ;
    INBUSUNLOCK(in);

    STAT_ADD1(NET_connection_errors);
    //LEVEL_DEFAULT("ClientConnect: Socket problem %d [%s]\n", errno, strerror(errno)) ;
    return -1 ;
}


/*
 Loops through outdevices, starting a detached thread for each except the last
 Each loop spawn threads for accepting connections
 Uses my non-patented "pre-threaded technique"
 
 VALGRIND's mutexes doesn't handle locks/unlocks in different processes
 so I added this define just to temporary be able to search for memory
 leaks... The new accept() is not detached just to be able to wait
 for thread to end.

 OK OK -- bad practice.
 We'll put the locks in the same thread.
*/
void ServerProcess( void (*HandlerRoutine)(int fd), void (*Exit)(int errcode) ) {
    struct connection_out * out = outdevice ;
#ifdef OW_MT
    pthread_t thread ;
    pthread_attr_t attr ;
    pthread_attr_t *attr_p = NULL ;
#endif /* OW_MT */

    /* embedded function */
    void ToListen( struct connection_out * o ) {
        if ( ServerAddr( o ) || (ServerListen( o )<0) ) {
            LEVEL_DEFAULT("Cannot start server = %s\n",o->name)
            Exit(1);
        }
    }

#ifdef OW_MT
    /* Embedded function */
    void * ConnectionThread( void * v3 ) {
        struct connection_out * out2 = (struct connection_out *)v3 ;
        pthread_t thread2 ;

        ToListen( out2 ) ;
        for(;;) {
            struct AcceptThread_data * data ;
            ACCEPTLOCK(out2);
            //if ( pthread_create( &thread2, attr_p, AcceptThread, out2 ) ) Exit(1) ;
            /* Start a thread running AcceptThread(), all necessary parameters are passed
               using a struct AcceptThread_data */
            if ( (data=malloc(sizeof(struct AcceptThread_data))) == NULL ) Exit(1) ;
            data->o2 = out2;
            data->HandlerRoutine = HandlerRoutine;
            if ( pthread_create( &thread2, attr_p, AcceptThread, data ) ) Exit(1) ;
            if(!attr_p) pthread_detach(thread2);
#ifdef VALGRIND
            pthread_join(thread2, NULL);
#endif /* VALGRIND */
        }
        /* won't reach this usless we exit the loop above to shutdown
            * in a nice way */
        /* last connection_out wasn't a separate thread */
        if(out2->next) {
            pthread_exit((void *)0);
        }
        return NULL;
    }

    pthread_attr_init(&attr) ;
#ifndef VALGRIND
    /* For some reason we can't detach the threads when using
     * valgrind. Just create and call pthread_join() at once */
    pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_DETACHED) ;
#endif /* VALGRIND */

#ifndef __UCLIBC__
    /* Seem to be some bug in uClibc since it's not possible to
     * detach the new thread at once. */
    attr_p = &attr;
#endif /* __UCLIBC__ */

    while ( out->next ) {
        if ( pthread_create( &thread, attr_p, ConnectionThread, out) ) Exit(1) ;
        if(!attr_p) pthread_detach(thread);
        out = out->next ;
    }
    ConnectionThread( out ) ;
#else /* OW_MT */
    ToListen( out ) ;
    for ( ;; ) RunAccepted( accept(outdevice->fd,NULL,NULL),HandlerRoutine ) ;
#endif /* OW_MT */
}

static void RunAccepted( int rafd, void (*HandlerRoutine)(int fd) ) {
    if ( rafd>=0 ) {
        HandlerRoutine( rafd ) ;
        close( rafd ) ;
    } else {
        STAT_ADD1(NET_accept_errors);
        LEVEL_CONNECT("accept() error %d [%s]\n", errno, strerror(errno));
    }
}
static void * AcceptThread( void * v2 ) {
    struct AcceptThread_data * data = (struct AcceptThread_data *)v2;
    int acceptfd = accept( data->o2->fd, NULL, NULL ) ;
    ACCEPTUNLOCK(data->o2);
    RunAccepted( acceptfd, data->HandlerRoutine ) ;
    free(data);
#ifndef VALGRIND
    pthread_exit((void *)0);
#endif /* VALGRIND */
    return NULL;
}
