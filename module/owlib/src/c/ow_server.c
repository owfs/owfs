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

static int FromServer( int fd, struct client_msg * cm, char * msg, int size ) ;
static void * FromServerAlloc( int fd, struct client_msg * cm ) ;
static int ToServer( int fd, struct server_msg * sm, const char * path, const char * data, int datasize ) ;

int Server_detect( void ) {
    if ( servername == NULL ) return -1 ;
    if ( ClientAddr( servername, &client ) ) return -1 ;
    busmode = bus_remote ;
    return 0 ;
}

int ServerRead( const char * path, char * buf, const size_t size, const off_t offset ) {
    struct server_msg sm ;
    struct client_msg cm ;
    int connectfd = ClientConnect( &client ) ;
    int ret ;

    if ( connectfd < 0 ) return -EIO ;
//printf("ServerRead path=%s, size=%d, offset=%d\n",path,size,offset);
    sm.type = msg_read ;
    sm.size = size ;
    sm.sg =  SemiGlobal.int32;
    sm.offset = offset ;
    if ( ToServer( connectfd, &sm, path, NULL, 0) ) {
        ret = -EIO ;
    } else if ( FromServer( connectfd, &cm, buf, size ) < 0 ) {
        ret = -EIO ;
    } else {
        ret = cm.ret ;
    }
    close( connectfd ) ;
    return ret ;
}

int ServerWrite( const char * path, const char * buf, const size_t size, const off_t offset ) {
    struct server_msg sm ;
    struct client_msg cm ;
    int connectfd = ClientConnect( &client ) ;
    int ret ;

    if ( connectfd < 0 ) return -EIO ;
//printf("ServerWrite path=%s, buf=%*s, size=%d, offset=%d\n",path,size,buf,size,offset);
    sm.type = msg_write ;
    sm.size = size ;
    sm.sg =  SemiGlobal.int32;
    sm.offset = offset ;
    if ( ToServer( connectfd, &sm, path, buf, size) ) {
        ret = -EIO ;
    } else if ( FromServer( connectfd, &cm, NULL, 0 ) < 0 ) {
        ret = -EIO ;
    } else {
        ret = cm.ret ;
        if ( SemiGlobal.int32 != cm.sg ) {
            CACHELOCK
                SemiGlobal.int32 = cm.sg ;
            CACHEUNLOCK
        }
    }
    close( connectfd ) ;
    return ret ;
}

int ServerDir( void (* dirfunc)(const struct parsedname * const), const char * path, const struct parsedname * const pn ) {
    struct server_msg sm ;
    struct client_msg cm ;
    char * path2 ;
    int connectfd = ClientConnect( &client ) ;
    struct parsedname pn2 ;

    if ( connectfd < 0 ) return -EIO ;
    /* Make a copy (shallow) of pn to modify for directory entries */
    memcpy( &pn2, pn , sizeof( struct parsedname ) ) ; /*shallow copy */

//printf("ServerDir path=%s\n",path);
    (void) pn ;
    sm.type = msg_dir ;
    sm.size = 0 ;
    sm.sg = SemiGlobal.int32;
    sm.offset = 0 ;
    if ( ToServer( connectfd, &sm, path, NULL, 0) ) {
        cm.ret = -EIO ;
    } else {
        while( (path2=FromServerAlloc( connectfd, &cm))  ) {
            path2[cm.payload-1] = '\0' ; /* Ensure trailing null */
//printf("ServerDir got:%s\n",path2);
            if ( FS_ParsedName( path2, &pn2 ) ) {
                cm.ret = -EINVAL ;
                free(path2) ;
                break ;
            } else {
//printf("DIRFUNC\n");
                dirfunc(&pn2) ;
                FS_ParsedName_destroy( &pn2 ) ;
                free(path2) ;
            }
        }
    }
    close( connectfd ) ;
    return cm.ret ;
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
    cm->sg = ntohl(cm->sg) ;
    cm->offset = ntohl(cm->offset) ;

//printf("FromServer payload=%d size=%d ret=%d sg=%d offset=%d\n",cm->payload,cm->size,cm->ret,cm->sg,cm->offset);
//printf(">%.4d|%.4d\n",cm->ret,cm->payload);
    if ( cm->payload == 0 ) return NULL ;
    if ( cm->payload > 65000 ) {
//printf("FromServerAlloc payload too large\n");
        return NULL ;
    }

    if ( (msg=malloc(cm->payload)) ) {
        if ( readn(fd,msg,cm->payload) != cm->payload ) {
//printf("FromServer couldn't read payload\n");
            cm->payload = 0 ;
            cm->ret = -EIO ;
            free(msg);
            msg = NULL ;
        }
//printf("FromServer payload read ok\n");
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
        return -EIO ;
    }

    cm->payload = ntohl(cm->payload) ;
    cm->size = ntohl(cm->size) ;
    cm->ret = ntohl(cm->ret) ;
    cm->sg = ntohl(cm->sg) ;
    cm->offset = ntohl(cm->offset) ;

//printf("FromServer payload=%d size=%d ret=%d sg=%d offset=%d\n",cm->payload,cm->size,cm->ret,cm->sg,cm->offset);
//printf(">%.4d|%.4d\n",cm->ret,cm->payload);
    if ( cm->payload == 0 ) return cm->payload ;

    d = cm->payload - size ;
    rtry = d<0 ? cm->payload : size ;
    if ( readn(fd, msg, rtry ) != rtry ) {
        cm->ret = -EIO ;
        return -EIO ;
    }

    if ( d>0 ) {
        char extra[d] ;
        if ( readn(fd,extra,d) != d ) {
            cm->ret = -EIO ;
            return -EIO ;
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

//printf("ToServer payload=%d size=%d type=%d tempscale=%X offset=%d\n",payload,sm->size,sm->type,sm->sg,sm->offset);
//printf("<%.4d|%.4d\n",sm->type,payload);

    sm->payload = htonl(payload)       ;
    sm->size    = htonl(sm->size)      ;
    sm->type    = htonl(sm->type)      ;
    sm->sg      = htonl(sm->sg) ;
    sm->offset  = htonl(sm->offset)    ;

    return writev( fd, io, nio ) != payload + sizeof(struct server_msg) ;
}
