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

int ServerAddr(  char * sname, struct network_work * nw ) {
    struct addrinfo hint ;
    int ret ;
    char * p ;

//printf("ClientAddr port=%s\n",sname);
    if ( sname == NULL ) return -1 ;
    if ( (p=strrchr(sname,':')) ) { /* : exists */
        *p = '\0' ; /* Separate tokens in the string */
        nw->host = strdup(sname) ;
	nw->service = strdup(&p[1]) ;
    } else {
        nw->host = NULL ;
        nw->service = strdup(sname) ;
    }

    bzero( &hint, sizeof(struct addrinfo) ) ;
    hint.ai_flags = AI_PASSIVE ;
    hint.ai_socktype = SOCK_STREAM ;
    hint.ai_family = AF_UNSPEC ;

    if ( (ret=getaddrinfo( 
        nw->host, 
	nw->service, 
	&hint, 
	&nw->ai )) 
        ) {
//printf("GetAddrInfo ret=%d\n",ret);
        fprintf(stderr,"GetAddrInfo error %s\n",gai_strerror(ret));
        return -1 ;
    }
    return 0 ;
}

int ServerListen( struct network_work * nw ) {
    int fd ;
    int on = 1 ;
    
    if ( nw->ai == NULL ) {
        fprintf(stderr,"Server address not yet parsed\n");
	return -1 ;
    }
    
    if ( nw->ai_ok == NULL ) nw->ai_ok = nw->ai ;
    do {
        fd = socket(
	    nw->ai_ok->ai_family,
	    nw->ai_ok->ai_socktype,
	    nw->ai_ok->ai_protocol
	    ) ;
	if ( fd >= 0 ) {
	    setsockopt(fd,SOL_SOCKET,SO_REUSEADDR,&on,sizeof(on)) ;
	    if ( bind( fd, nw->ai_ok->ai_addr, nw->ai_ok->ai_addrlen )
	        || listen(fd, 100)
		) {
		close( fd ) ;
	    } else {
	        nw->listenfd = fd ;
		return fd ;
	    }
	}
    } while ( (nw->ai_ok=nw->ai_ok->ai_next) ) ;
    fprintf(stderr,"Socket problem errno=%d\n",errno) ;
    return -1 ;
}

int ClientAddr(  char * sname, struct network_work * nw ) {
    struct addrinfo hint ;
    char * p ;
    int ret ;

//printf("ClientAddr port=%s\n",sname);
    if ( sname == NULL ) return -1 ;
    if ( (p=strrchr(sname,':')) ) { /* : exists */
        *p = '\0' ; /* Separate tokens in the string */
        nw->host = strdup(sname) ;
	nw->service = strdup(&p[1]) ;
    } else {
        nw->host = NULL ;
        nw->service = strdup(sname) ;
    }

    bzero( &hint, sizeof(struct addrinfo) ) ;
    hint.ai_socktype = SOCK_STREAM ;
    hint.ai_family = AF_UNSPEC ;

    if ( (ret=getaddrinfo( nw->host, nw->service, &hint, &nw->ai )) ) {
//printf("GetAddrInfo ret=%d\n",ret);
        fprintf(stderr,"GetAddrInfo error %s\n",gai_strerror(ret));
        return -1 ;
    }
    return 0 ;
}

int ClientConnect( struct network_work * nw ) {
    int fd ;

    if ( nw->ai == NULL ) {
        fprintf(stderr,"Client address not yet parsed\n");
	return -1 ;
    }
    
    if ( nw->ai_ok == NULL ) nw->ai_ok = nw->ai ;
    do {
        fd = socket(
	    nw->ai_ok->ai_family,
	    nw->ai_ok->ai_socktype,
	    nw->ai_ok->ai_protocol
	    ) ;
	if ( fd >= 0 ) {
	    if ( connect(fd, nw->ai_ok->ai_addr, nw->ai_ok->ai_addrlen) == 0 ) { 
		return fd ;
	    }
	    close( fd ) ;
	}
    } while ( (nw->ai_ok=nw->ai_ok->ai_next) ) ;
    fprintf(stderr,"Socket problem errno=%d\n",errno) ;
    return -1 ;
}

void FreeAddr( struct network_work * nw ) {
    if ( nw->host ) {
        free(nw->host) ;
	nw->host = NULL ; 
    }
    if ( nw->service ) {
        free(nw->service) ;
	nw->service = NULL ;
    }
    if ( nw->ai ) {
        freeaddrinfo(nw->ai) ;
	nw->ai = NULL ;
    }
}
