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

#include "owshell.h"


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
                    //ERROR_DATA("Network data read error\n") ;
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
                continue;
            }
            //ERROR_DATA("Selection error (network)\n") ;
            return -EINTR;
        } else { /* timed out */
            //LEVEL_CONNECT("TIMEOUT after %d bytes\n",n-nleft);
            return -EAGAIN;
        }
    }
    return(n - nleft); /* return >= 0 */
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
#if OW_CYGWIN
        in->connin.server.host = strdup("0.0.0.0") ;
#else
        in->connin.server.host = NULL ;
#endif
        in->connin.server.service = strdup(sname) ;
    }
    
    memset( &hint, 0, sizeof(struct addrinfo) ) ;
    hint.ai_socktype = SOCK_STREAM ;
#if OW_CYGWIN
    hint.ai_family = AF_INET ;
#else
    hint.ai_family = AF_UNSPEC ;
#endif

//printf("ClientAddr: [%s] [%s]\n", in->connin.server.host, in->connin.server.service);

    if ( (ret=getaddrinfo( in->connin.server.host, in->connin.server.service, &hint, &in->connin.server.ai )) ) {
        //LEVEL_CONNECT("GetAddrInfo error %s\n",gai_strerror(ret));
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
int ClientConnect( void ) {
    int fd ;
    struct addrinfo *ai ;

    if ( indevice->connin.server.ai == NULL ) {
        //LEVEL_DEBUG("Client address not yet parsed\n");
        return -1 ;
    }

    /* Can't change ai_ok without locking the in-device.
     * First try the last working address info, if it fails lock
     * the in-device and loop through the list until it works.
     * Not a perfect solution, but it should work at least.
     */
    ai = indevice->connin.server.ai_ok ;
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

    ai = indevice->connin.server.ai ;  // loop from first address info since it failed.
    do {
        fd = socket(
            ai->ai_family,
            ai->ai_socktype,
            ai->ai_protocol
            ) ;
        if ( fd >= 0 ) {
            if ( connect(fd, ai->ai_addr, ai->ai_addrlen) == 0 ) {
                indevice->connin.server.ai_ok = ai ;
                return fd ;
            }
            close( fd ) ;
        }
    } while ( (ai = ai->ai_next) ) ;
    indevice->connin.server.ai_ok = NULL ;

    //ERROR_CONNECT("ClientConnect: Socket problem\n") ;
    return -1 ;
}
