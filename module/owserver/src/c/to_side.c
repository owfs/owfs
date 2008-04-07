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

/* Send fully configured message back to client.
   data is optional and length depends on "payload"
   This is the Side message header added
 */
int ToClientSide(struct connection_side * side, struct client_msg *original_cm, char *data, struct side_msg * sidem )
{
    // note data should be (const char *) but iovec complains about const arguments 
    int nio = 2;
    int ret;
    struct client_msg s_cm ;
    struct client_msg * cm = &s_cm ;
    struct iovec io[] = {
        {sidem, sizeof(struct side_msg),},
        {cm, sizeof(struct client_msg),},
        {data, original_cm->payload,},
    };

    if ( ! side->good_entry ) {
        return 0 ;
    }
    // FromClientSide should have set file_descriptor, so don't start in the middle
    if ( side->file_descriptor < 0 ) {
        return 0 ;
    }
    LEVEL_DEBUG("ToSide \n");
    
    if (data && original_cm->payload > 0) {
        ++nio;
        //printf("ToClient <%*s>\n",original_cm->payload,data) ;
    }

    cm->version = htonl(original_cm->version);
    cm->payload = htonl(original_cm->payload);
    cm->size = htonl(original_cm->size);
    cm->offset = htonl(original_cm->offset);
    cm->ret = htonl(original_cm->ret);
    cm->sg = htonl(original_cm->sg);

    SIDELOCK(side) ;

    sidem->version |= MakeServermessage ;
    sidem->version = htonl(sidem->version);

    ret = writev(side->file_descriptor, io, nio) ;

    sidem->version = ntohl(sidem->version);
    sidem->version ^= MakeServermessage ;

    SIDEUNLOCK(side) ;
    
    return ( ret != (ssize_t) (io[0].iov_len+io[1].iov_len+io[2].iov_len) ) ;
}

int FromClientSide(struct connection_side * side, struct handlerdata *hd )
{
// note data should be (const char *) but iovec complains about const arguments
    int nio = 1;
    int ret;
    struct iovec io[] = {
        {&(hd->sidem), sizeof(struct side_msg),},
    };
    struct client_msg s_cm ;
    struct client_msg * cm = &s_cm ;

    if ( ! side->good_entry ) {
        return 0 ;
    }
    LEVEL_DEBUG("FromSide \n");

    memcpy( cm , &(hd->sm) , sizeof(hd->sm) ) ;

    SIDELOCK(side) ;

    // Connect if needed. If can't leave -1 flag for rest of transaction
    if ( side->file_descriptor < 0 ) {
        side->file_descriptor = SideConnect(side) ;
        if ( side->file_descriptor < 0 ) {
            SIDEUNLOCK(side) ;
            return -EIO ;
        }
    }

    hd->sidem.version = htonl(hd->sidem.version);

    ret = writev(side->file_descriptor, io, nio) != (ssize_t) (io[0].iov_len);

    hd->sidem.version = ntohl(hd->sidem.version);

    if (ret) {
        SIDEUNLOCK(side) ;
        return -EIO;
    }

    ret =  ToClient( side->file_descriptor, cm, hd->sp.path ) ;
    if (ret) {
        SIDEUNLOCK(side) ;
        return ret;
    }

    if ( hd->sp.tokens > 0 ) {
        io[0].iov_base = hd->sp.tokenstring ;
        io[0].iov_len = hd->sp.tokens ;
        if ( writev(side->file_descriptor, io, nio) != (ssize_t) (io[0].iov_len) ) {
            SIDEUNLOCK(side) ;
            return -EIO ;
        }
    }
    SIDEUNLOCK(side) ;
    return 0 ;
}

    /* read from client, free return pointer if not Null */
int SetupSideMessage( struct handlerdata *hd )
{
    socklen_t hostlength = sizeof(hd->sidem.host) ;
    socklen_t peerlength = sizeof(hd->sidem.peer) ;
    memset( &(hd->sidem), 0, sizeof(struct side_msg ) ) ;
    if ( getsockname( hd->file_descriptor, (struct sockaddr *)(hd->sidem.host), &hostlength) ) {
        ERROR_CALL("Can't get socket host address for Sidetap info\n");
        return -1 ;
    }
    if ( getpeername( hd->file_descriptor, (struct sockaddr *)(hd->sidem.peer), &peerlength) ) {
        ERROR_CALL("Can't get socket peer address for Sidetap info\n");
        return -1 ;
    }
    hd->sidem.version |= MakeServersidetap ;
    hd->sidem.version |= MakeServertokens(peerlength) ;
    return 0 ;
}









