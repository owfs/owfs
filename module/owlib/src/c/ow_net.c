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

#include <config.h>
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
static int ServerAddr(  struct connection_out * out ) ;
static int ServerListen( struct connection_out * out ) ;

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

static int ServerAddr(  struct connection_out * out ) {
    struct addrinfo hint ;
    int ret ;
    char * p ;

    if ( out->name == NULL ) return -1 ;
    if ( (p=strrchr(out->name,':')) ) { /* : exists */
        p[0] = '\0' ; /* Separate tokens in the string */
        out->host = strdup(out->name) ;
        out->service = strdup(&p[1]) ;
        p[0] = ':' ; /* Restore the string */
    } else {
        out->host = NULL ;
        out->service = strdup(out->name) ;
    }

    memset( &hint, 0, sizeof(struct addrinfo) ) ;
    hint.ai_flags = AI_PASSIVE ;
    hint.ai_socktype = SOCK_STREAM ;
#ifdef __FreeBSD__
    hint.ai_family = AF_INET ; // FreeBSD will bind IP6 preferentially
#else /* __FreeBSD__ */
    hint.ai_family = AF_UNSPEC ;
#endif /* __FreeBSD__ */

    //printf("ServerAddr: [%s] [%s]\n", out->host, out->service);

    if ( (ret=getaddrinfo( out->host, out->service, &hint, &out->ai )) ) {
        ERROR_CONNECT("GetAddrInfo error [%s]\n",SAFESTRING(out->name));
        return -1 ;
    }
    return 0 ;
}

static int ServerListen( struct connection_out * out ) {
    int fd ;
    int on = 1 ;
    int ret;

    if ( out->ai == NULL ) {
        LEVEL_CONNECT("Server address not yet parsed [%s]\n",SAFESTRING(out->name));
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
                ERROR_CONNECT("ServerListen: Socket problem [%s]\n", SAFESTRING(out->name));
                close( fd ) ;
            } else {
                out->fd = fd ;
                return fd ;
            }
        } else {
            ERROR_CONNECT("ServerListen: socket() [%s]", SAFESTRING(out->name));
        }
    } while ( (out->ai_ok=out->ai_ok->ai_next) ) ;
    ERROR_CONNECT("ServerListen: Socket problem [%s]\n", SAFESTRING(out->name)) ;
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

/* Usually called with BUS locked, to protect ai settings */
int ClientConnect( struct connection_in * in ) {
    int fd ;
    struct addrinfo *ai ;

    if ( in->connin.server.ai == NULL ) {
        LEVEL_CONNECT("Client address not yet parsed\n");
        return -1 ;
    }

    /* Can't change ai_ok without locking the in-device.
     * First try the last working address info, if it fails lock
     * the in-device and loop through the list until it works.
     * Not a perfect solution, but it should work at least.
     */
    ai = in->connin.server.ai_ok ;
    if( ai ) {
        fd = socket(
            ai->ai_family,
            ai->ai_socktype,
            ai->ai_protocol
            ) ;
        if ( fd >= 0 ) {
            if ( connect(fd, ai->ai_addr, ai->ai_addrlen) == 0 ) return fd ;
            close( fd ) ;
        }
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
                return fd ;
            }
            close( fd ) ;
        }
    } while ( (ai = ai->ai_next) ) ;
    in->connin.server.ai_ok = NULL ;

    ERROR_CONNECT("ClientConnect: Socket problem\n") ;
    STAT_ADD1(NET_connection_errors);
    return -1 ;
}

int ServerOutSetup( struct connection_out * out ) {
    return ServerAddr( out ) || (ServerListen( out )<0) ;
}

/*
 Loops through outdevices, starting a detached thread for each 
 Each loop spawn threads for accepting connections
 Uses my non-patented "pre-threaded technique"
 */

/* Server Process run for each out device, and each */
#if OW_MT
struct serverprocessstruct {
    struct connection_out * out ;
    void (*HandlerRoutine)(int fd) ;
    void (*Exit)(int errcode) ;
} ;

static void * ServerProcessAccept( void * vp ) {
    struct serverprocessstruct * sps = (struct serverprocessstruct *) vp ;
    pthread_t thread ;
    int acceptfd ;
    int ret ;
    
    pthread_detach( pthread_self() ) ;

badthread:
        //LEVEL_DEBUG("ACCEPT %s[%lu] start %d\n",SAFESTRING(sps->out->name),(unsigned long int)pthread_self(),sps->out->index) ;
    ACCEPTLOCK(sps->out) ;
    //LEVEL_DEBUG("ACCEPT %s[%lu] locked %d\n",SAFESTRING(sps->out->name),(unsigned long int)pthread_self(),sps->out->index) ;
    ret = pthread_create( &thread, NULL, ServerProcessAccept, vp ) ;
    //LEVEL_DEBUG("ACCEPT %s[%lu] create %d\n",SAFESTRING(sps->out->name),(unsigned long int)pthread_self(),sps->out->index) ;
    acceptfd = accept( sps->out->fd, NULL, NULL ) ;
    //LEVEL_DEBUG("ACCEPT %s[%lu] accept %d\n",SAFESTRING(sps->out->name),(unsigned long int)pthread_self(),sps->out->index) ;
    ACCEPTUNLOCK(sps->out) ;
    //LEVEL_DEBUG("ACCEPT %s[%lu] unlock %d\n",SAFESTRING(sps->out->name),(unsigned long int)pthread_self(),sps->out->index) ;
    
    if ( acceptfd < 0 ) {
        ERROR_CONNECT("Trouble with accept on listen socket %d [%s](%d)\n",sps->out->fd, sps->out->name, sps->out->index ) ;
    } else {
        (sps->HandlerRoutine)(acceptfd) ;
        close(acceptfd) ;
    }
    //LEVEL_DEBUG("ACCEPT  <%p> <%p>\n",sps,sps->out);
    //LEVEL_DEBUG("ACCEPT %s[%lu] handled %d\n",SAFESTRING(sps->out->name),(unsigned long int)pthread_self(),sps->out->index) ;
    /* only loop if another thread couldn't be created */
    if ( ret ) goto badthread ;
    return NULL ;
}
    
static void * ServerProcessOut( void * vp ) {
    struct serverprocessstruct * sps = (struct serverprocessstruct *) vp ;

    pthread_detach( pthread_self() ) ;

    if ( ServerOutSetup( sps->out ) ) {
        LEVEL_CONNECT("Cannot set up outdevice [%s](%d) -- will exit\n",SAFESTRING(sps->out->name),sps->out->index) ;
        (sps->Exit)(1) ;
    }

    ServerProcessAccept( vp ) ;
    return NULL ;
}

void ServerProcess( void (*HandlerRoutine)(int fd), void (*Exit)(int errcode) ) {
    struct serverprocessstruct * sps ;
    struct connection_out * out = outdevice ;
    int i ;

    if ( outdevices==0 ) {
        LEVEL_CALL("No output devices defined\n") ;
        Exit(1) ;
    }

    /* Create a memory buffer for each device */
    sps = (struct serverprocessstruct *) calloc(outdevices, sizeof(struct serverprocessstruct)) ;
    if ( sps==NULL ) {
        LEVEL_CALL("Cannot allocate space for out device structures\n") ;
        Exit(1) ;
    }

    /* Start the head of a thread chain for each outdevice */
    for ( i=0 ; i<outdevices ; ++i,out=out->next ) {
        pthread_t thread ;
        sps[i].out = out ;
        sps[i].HandlerRoutine = HandlerRoutine ;
        sps[i].Exit = Exit ;
        if ( pthread_create(&thread, NULL, ServerProcessOut, (void *)(&(sps[i])) ) ) {
            ERROR_CONNECT("Could not create a thread for %s\n",SAFESTRING(sps[i].out->name)) ;
            Exit(1) ;
        }
    }

    /* Wait for the end */
    pause() ;

    /* Cleanup that may never be reached */
    free(sps) ;
}

#else /* OW_MT */

void ServerProcess( void (*HandlerRoutine)(int fd), void (*Exit)(int errcode) ) {
    if ( outdevices==0 ) {
        LEVEL_CONNECT("Not output device (port) specified. Exiting.\n") ;
        Exit(1) ;
    } else if ( outdevices>1 ) {
        LEVEL_CONNECT("More than one output device specified (%d). Library compiled non-threaded. Exiting.\n",indevices) ;
        Exit(1) ;
    } else if ( ServerAddr( outdevice ) || (ServerListen( outdevice )<0) ) {
        LEVEL_CONNECT("Cannot set up outdevice [%s] -- will exit\n",SAFESTRING(outdevice->name)) ;
        Exit(1) ;
    } else {
        while (1) {
            int acceptfd=accept(outdevice->fd,NULL,NULL) ;
            if ( acceptfd < 0 ) {
                ERROR_CONNECT("Trouble with accept, will reloop\n") ;
            } else {
                HandlerRoutine(acceptfd) ;
                close(acceptfd) ;
            }
        }
    }
}
#endif /* OW_MT */
