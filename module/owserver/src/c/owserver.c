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

#include "owserver.h"

/* --- Prototypes ------------ */
int Handler( int fd ) ;
ssize_t readn(int fd, void *vptr, size_t n) ;
void * FromClientAlloc( int fd, struct server_msg * sm ) ;
int ToClient( int fd, int format, const char * data, int datasize, int ret ) ;
int Acceptor( int listenfd ) ;

/* ----- Globals ------------- */
#ifdef OW_MT
    pthread_mutex_t accept_mutex = PTHREAD_MUTEX_INITIALIZER ;
    #define ACCEPTLOCK       pthread_mutex_lock(  &accept_mutex) ;
    #define ACCEPTUNLOCK     pthread_mutex_unlock(&accept_mutex) ;
#else /* OW_MT */
    #define ACCEPTLOCK
    #define ACCEPTUNLOCK
#endif /* OW_MT */


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

/* read from client, free return pointer if not Null */
void * FromClientAlloc( int fd, struct server_msg * sm ) {
    char * msg ;

    if ( readn(fd, sm, sizeof(struct server_msg) ) != sizeof(struct server_msg) ) {
        sm->size = -1 ;
        return NULL ;
    }

    sm->size = ntohl(sm->size) ;
    sm->type = ntohl(sm->type) ;
    sm->tempscale = ntohl(sm->tempscale) ;

    if ( sm->size <= 0 ) return NULL ;

    if ( (msg=malloc(sm->size)) ) {
        if ( readn(fd,msg,sm->size) != sm->size ) {
            sm->size = -1 ;
            free(msg);
            msg = NULL ;
        }
    }
    return msg ;
}

int ToClient( int fd, int format, const char * data, int datasize, int ret ) {
    struct client_msg cm ;
    int nio = 1 ;
    struct iovec io[] = {
        { &cm, sizeof(struct client_msg), } ,
        { data, datasize, } ,
    } ;

    if ( data && datasize ) {
        ++nio ;
    }

    cm.size = htonl(datasize) ;
    cm.ret = htonl(ret) ;
    cm.format = htonl(format) ;

    return writev( fd, io, nio ) != cm.size + sizeof(struct client_msg) ;
}

int Handler( int fd ) {
    char * data = NULL ;
    int datasize ;
    int len ;
    char * returned = NULL ;
    int ret = 0 ;
    struct server_msg sm ;
    char * path = FromClientAlloc( fd, &sm ) ;
    struct parsedname pn ;
    struct stateinfo si ;
    void directory( const struct parsedname * const pn2 ) {
        char D[OW_FULLNAME_MAX] ;
        char * p ;
        FS_DirName( D, OW_FULLNAME_MAX, pn2 ) ;
        p = memchr(D,0,OW_FULLNAME_MAX) ;
        ToClient(fd,1,D,p?OW_FULLNAME_MAX:p-D,1) ;
    }
    /* error reading message */
    if ( sm.size < 0 ) {
        /* Nothing allocated */
        return ToClient( fd, 0, NULL, 0, -EIO ) ;
    }

    /* No payload -- only ok if msg_nop */
    if ( sm.size == 0 ) {
        /* Nothing allocated */
        return ToClient( fd, 0, NULL, 0, (enum msg_type)sm.type==msg_nop?0:-EBADMSG ) ;
    }

    /* Now parse path and locate memory */
    if ( memchr( path, 0, sm.size) == NULL ) { /* Bad string -- no trailing null */
        FS_ParsedName_destroy(&pn) ;
        if ( path ) free(path) ;
        return ToClient( fd, 0, NULL, 0, -EBADMSG ) ;
    }
    len = strlen( path ) + 1 ; /* include trailing null */

    /* Locate post-path buffer as data */
    datasize = sm.size - len ;
    if ( datasize ) data = &path[len] ;

    /* Parse the path string */
    pn.si = &si ;
    if ( (ret=FS_ParsedName( path , &pn )) ) {
        FS_ParsedName_destroy(&pn) ;
        if ( path ) free(path) ;
        return ToClient( fd, 0, NULL, 0, ret ) ;
    }

    /* parse name? */
    switch( (enum msg_type) sm.type ) {
    case msg_get:
        if ( pn.dev==NULL || pn.ft == NULL ) {
            sm.type = msg_dir ;
            len = OW_FULLNAME_MAX ;
            break ;
        }
        sm.type = msg_read ;
        /* fall through */
    case msg_read:
        len = FullFileLength(&pn) ;
        break ;
    case msg_put:
    case msg_write:
       len = 0 ;
       break ;
    case msg_dir:
        len = 0 ;
        break ;
    case msg_attr:
        len = 0 ;
        break ;
    case msg_statf:
        len = 0 ;
        break ;
    default:
        FS_ParsedName_destroy(&pn) ;
        if ( path ) free(path) ;
        return ToClient( fd, 0, NULL, 0, -ENOTSUP ) ;
    }

    if ( len && (returned = malloc(len))==NULL ) {
        FS_ParsedName_destroy(&pn) ;
        if ( path ) free(path) ;
        return ToClient( fd, 0, NULL, 0, -ENOBUFS ) ;
    }

    /* parse name? */
    switch( (enum msg_type) sm.type ) {
    case msg_read:
        ret = FS_read_postparse(path,returned,len,0,&pn) ;
        if ( ret<0 ) {
            free(returned);
            returned = NULL ;
        }
        len = ret ;
        break ;
    case msg_put:
    case msg_write:
        ret = FS_write_postparse(path,data,datasize,0,&pn) ;
        if ( ret<0 ) {
            free(returned);
            returned = NULL ;
        }
        break ;
    case msg_dir:
        FS_dir( directory, &pn ) ;
        ret = 0 ;
        break ;
    case msg_attr:
    case msg_statf:
    }
    FS_ParsedName_destroy(&pn) ;
    if (path) free(path) ;
    ret = ToClient( fd, 0, returned, len, ret ) ;
    if ( returned ) free(returned) ;
    return ret<0 ;
}

int Acceptor( int listenfd ) {
    int fd = accept( listenfd, NULL, NULL ) ;
    ACCEPTUNLOCK
    if ( fd<0 ) return -1 ;
    return Handler( fd ) ;
}

int main( int argc , char ** argv ) {
    int listenfd ;

    for(;;) {
        ACCEPTLOCK
        Acceptor(listenfd);
    }
}
