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
    int r ;
    
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

int ToClient( int fd, const enum msg_type type, int format, const char * data, int datasize, int more ) {
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
    cm.format = htonl(format) ;
    cm.more = htonl(more) ;
    
    return writev( fd, io, nio ) != size + sizeof(struct client_msg) ;
}

int Handler( int fd ) {
    char * path ;
    char * data ;
    char * returned = NULL ;
    int nreturned = 0 ;
    int err = 0 ;
    int ret = 0 ;
    int more = 0 ;
    struct server_msg sm ;
    char * msg = FromClientAlloc( fd, &sm ) ;
    struct parsedname pn ;
    
    if ( sm.size >= 0 ) {
        /* parse name? */
        switch( (enum msg_type) sm.type ) {
        case msg_nop:
            break ;
        default:
        
        }
        /* parse name? */
        switch( (enum msg_type) sm.type ) {
        case msg_nop:
        case msg_read:
        case msg_write:
        case msg_dir:
        case msg_get:
        case msg_put:
        case msg_attr:
        case msg_statf:
        }

    } else {
        err = sm.size ;
    }
    ret = ToClient( fd, 0, returned, err, more ) ;
    if ( returned ) free(returned) ;
    if ( msg ) free(msg) ;
    return ret ;
     
    
    
}