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

/* ow_server talks to the server, sending and recieving messages */
/* this is an alternative to direct bus communication */

#include "owfs_config.h"
#include "ow.h"
//#include <sys/uio.h>
#include <netinet/in.h>
#include <netdb.h> /* addrinfo */

static int ConnectFD( struct addrinfo * ai) ;
static int PortToFD(  char * port ) ;
static int FromServer( int fd, struct client_msg * cm, char * msg, int size ) ;
static void * FromServerAlloc( int fd, struct client_msg * cm ) ;
static int ToServer( int fd, struct server_msg * sm, const char * path, const char * data, int datasize ) ;
static int OpenServer( void ) ;

int Server_detect( void ) {
    int ret = OpenServer() ;
    if ( ret==0) busmode = bus_remote ;
    return ret ;
}

static int OpenServer( void ) {
    if ( servername ) {
        connectfd = PortToFD( servername ) ;
    }
    return (connectfd<0) ;
}

void CloseServer( void ) {
    if ( connectfd > -1 ) {
        close(connectfd) ;
        connectfd = -1 ;
    }
}

int ServerRead( const char * path, char * buf, const size_t size, const off_t offset ) {
    struct server_msg sm ;
    struct client_msg cm ;
    int ret ;

printf("ServerRead path=%s, size=%d, offset=%d\n",path,size,offset);
    sm.type = msg_read ;
    sm.size = size ;
    sm.sg =  SemiGlobal.int32;
    sm.offset = offset ;
    ret = ToServer( connectfd, &sm, path, NULL, 0) ;
    if (ret) return ret ;
    ret = FromServer( connectfd, &cm, buf, size ) ;
    if (ret) return ret ;
    if ( SemiGlobal.int32 != cm.format ) {
        CACHELOCK
            SemiGlobal.int32 = cm.format ;
        CACHEUNLOCK
    }
    
    return cm.ret ;
}

int ServerWrite( const char * path, const char * buf, const size_t size, const off_t offset ) {
    struct server_msg sm ;
    struct client_msg cm ;
    int ret ;

printf("ServerWrite path=%s, buf=%*s, size=%d, offset=%d\n",path,size,buf,size,offset);
    sm.type = msg_write ;
    sm.size = size ;
    sm.sg =  SemiGlobal.int32;
    sm.offset = offset ;
    ret = ToServer( connectfd, &sm, path, buf, size) ;
    if (ret) return ret ;
    ret = FromServer( connectfd, &cm, NULL, 0 ) ;
    if (ret) return ret ;
    return cm.ret ;
}

int ServerDir( void (* dirfunc)(const struct parsedname * const), const char * path, const struct parsedname * const pn ) {
    struct server_msg sm ;
    struct client_msg cm ;
    char * path2 ;
    int ret = 0 ;
    struct parsedname pn2 ;
    struct stateinfo si ;
    pn2.si = &si ;

printf("ServerDir path=%s\n",path);
    (void) pn ;
    sm.type = msg_dir ;
    sm.size = 0 ;
    sm.sg =  SemiGlobal.int32;
    sm.offset = 0 ;
    ret = ToServer( connectfd, &sm, path, NULL, 0) ;
    if (ret) return ret ;
    while( (path2=FromServerAlloc( connectfd, &cm))  ) {
        path2[cm.payload-1] = '\0' ; /* Ensure trailing null */
printf("ServerDir got:%s\n",path2);
	if ( (ret=FS_ParsedName( path2, &pn2 )) == 0 ) {
	    dirfunc(&pn2) ;
            FS_ParsedName_destroy( &pn2 ) ;
	}
        free(path2) ;
    }
    return ret ;
}

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

/* read from server, free return pointer if not Null */
static void * FromServerAlloc( int fd, struct client_msg * cm ) {
    char * msg ;

    if ( readn(fd, cm, sizeof(struct client_msg) ) != sizeof(struct client_msg) ) {
        cm->size = 0 ;
        cm->ret = -EIO ;
        return NULL ;
    }
    cm->payload = ntohl(cm->payload) ;
    cm->size = ntohl(cm->size) ;
    cm->ret = ntohl(cm->ret) ;
    cm->format = ntohl(cm->format) ;
    cm->offset = ntohl(cm->offset) ;

printf("FromServer payload=%d size=%d ret=%d format=%d offset=%d\n",cm->payload,cm->size,cm->ret,cm->format,cm->offset);
    if ( cm->payload == 0 ) return NULL ;
    if ( cm->payload > 65000 ) {
printf("FromServerAlloc payload too large\n");
        return NULL ;
    }
    
    if ( (msg=malloc(cm->payload)) ) {
        if ( readn(fd,msg,cm->payload) != cm->payload ) {
printf("FromServer couldn't read payload\n");
            cm->payload = 0 ;
            cm->ret = -EIO ;
            free(msg);
            msg = NULL ;
        }
printf("FromServer payload read ok\n");
    }
    return msg ;
}
/* Read from server -- return negative on error,
    return 0 or positive giving size of data element */
static int FromServer( int fd, struct client_msg * cm, char * msg, int size ) {
    int rtry ;
    int d ;

    if ( readn(fd, cm, sizeof(struct client_msg) ) != sizeof(struct client_msg) ) {
        cm->size = 0 ;
        cm->ret = -EIO ;
        return -1 ;
    }

    cm->payload = ntohl(cm->payload) ;
    cm->size = ntohl(cm->size) ;
    cm->ret = ntohl(cm->ret) ;
    cm->format = ntohl(cm->format) ;
    cm->offset = ntohl(cm->offset) ;

printf("FromServer payload=%d size=%d ret=%d format=%d offset=%d\n",cm->payload,cm->size,cm->ret,cm->format,cm->offset);
    if ( cm->payload == 0 ) return cm->payload ;

    d = cm->payload - size ;
    rtry = d<0 ? cm->payload : size ;
    if ( readn(fd, msg, rtry ) != rtry ) {
        cm->ret = -EIO ;
        return -1 ;
    }

    if ( d>0 ) {
        char extra[d] ;
        if ( readn(fd,extra,d) != d ) {
            cm->ret = -EIO ;
            return -1 ;
        }
        return size ;
    }
    return cm->payload ;
}

static int ToServer( int fd, struct server_msg * sm, const char * path, const char * data, int datasize ) {
    int nio = 1 ;
    int payload = 0 ;
    struct iovec io[] = {
        { sm, sizeof(struct server_msg), } ,
        { path, 0, } ,
        { data, datasize, } ,
    } ;
    if ( path ) {
        ++ nio ;
        io[1].iov_len = payload = strlen(path) + 1 ;
        if ( data && datasize ) {
            ++nio ;
            payload += datasize ;
        }
    }

printf("ToServer payload=%d size=%d type=%d tempscale=%X offset=%d\n",payload,sm->size,sm->type,sm->sg,sm->offset);

    sm->payload = htonl(payload)       ;
    sm->size    = htonl(sm->size)      ;
    sm->type    = htonl(sm->type)      ;
    sm->sg      = htonl(sm->sg) ;
    sm->offset  = htonl(sm->offset)    ;

    return writev( fd, io, nio ) != payload + sizeof(struct server_msg) ;
}

static int PortToFD(  char * sname ) {
    char * host ;
    char * serv ;
    struct addrinfo hint ;
    struct addrinfo * ai ;
    int matches = 0 ;
    int ret = -1;

printf("PortToFD port=%s\n",sname);
    if ( sname == NULL ) return -1 ;
    if ( (serv=strrchr(sname,':')) ) { /* : exists */
        *serv = '\0' ;
        ++serv ;
        host = sname ;
    } else {
        host = NULL ;
        serv = sname ;
    }

    bzero( &hint, sizeof(struct addrinfo) ) ;
    hint.ai_flags = AI_PASSIVE ;
    hint.ai_socktype = SOCK_STREAM ;
    hint.ai_family = AF_UNSPEC ;

    while( (ret=getaddrinfo( host, serv, &hint, &ai ))==0 ) {
printf("GetAddrInfo ret=%d\n",ret);
        ret = ConnectFD(ai) ;
        freeaddrinfo(ai) ;
	++matches ;
        break ;
    }
    
    return ret ;
}

static int ConnectFD( struct addrinfo * ai) {
    int fd ;

    if ( (fd=socket(ai->ai_family,ai->ai_socktype,ai->ai_protocol))<0 ) {
        fprintf(stderr,"Socket problem errno=%d\n",errno) ;
        return -1 ;
    }
    
    if ( connect(fd, ai->ai_addr, ai->ai_addrlen) ) { /* Arbitrary "backlog" parameter */
        fprintf(stderr,"Connect problem. errno=%d\n",errno) ;
        return -1 ;
    }
printf("ConnectFD happy\n");    
    return fd ;
}


