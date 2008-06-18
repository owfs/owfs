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
int ToClientSide(struct connection_side *side, struct client_msg *original_cm, char *data, struct side_msg *sidem)
{
	// note data should be (const char *) but iovec complains about const arguments 
	int nio = 2;
	int ret;
	struct client_msg s_cm;
	struct client_msg *cm = &s_cm;
	struct iovec io[] = {
		{sidem, sizeof(struct side_msg),},
		{cm, sizeof(struct client_msg),},
		{data, original_cm->payload,},
	};

	if (!side->good_entry) {
		return 0;
	}
	// FromClientSide should have set file_descriptor, so don't start in the middle
	if (side->file_descriptor < 0) {
		return 0;
	}
	LEVEL_DEBUG("ToClient data sent to Sidetap \n");

	if (data && original_cm->payload > 0) {
		++nio;
		//printf("ToClient <%*s>\n",original_cm->payload,data) ;
	}

	cm->version = htonl(original_cm->version);
	cm->payload = htonl(original_cm->payload);
	cm->ret = htonl(original_cm->ret);
	cm->sg = htonl(original_cm->sg);
	cm->size = htonl(original_cm->size);
	cm->offset = htonl(original_cm->offset);

	SIDELOCK(side);

	sidem->version |= MakeServermessage;
	sidem->version = htonl(sidem->version);

	ret = writev(side->file_descriptor, io, nio);

	sidem->version = ntohl(sidem->version);
	sidem->version ^= MakeServermessage;

	SIDEUNLOCK(side);

	return (ret != (ssize_t) (io[0].iov_len + io[1].iov_len + io[2].iov_len));
}

int FromClientSide(struct connection_side *side, struct handlerdata *hd)
{
// note data should be (const char *) but iovec complains about const arguments
	int nio = 2;
	int ret;
	ssize_t trueload = hd->sm.payload + sizeof(union antiloop) * Servertokens(hd->sm.version);
	struct server_msg s_sm;
	struct server_msg *sm = &s_sm;
	struct iovec io[] = {
		{&(hd->sidem), sizeof(struct side_msg),},
		{sm, sizeof(struct server_msg),},
		{hd->sp.path, trueload,},
	};

	if (!side->good_entry) {
		return 0;
	}
	LEVEL_DEBUG("FromClient data sent to Sidetap \n");

	SIDELOCK(side);

	// Connect if needed. If can't leave -1 flag for rest of transaction
	if (side->file_descriptor < 0) {
		side->file_descriptor = SideConnect(side);
		if (side->file_descriptor < 0) {
			SIDEUNLOCK(side);
			return -EIO;
		}
	}

	hd->sidem.version = htonl(hd->sidem.version);

	sm->version = htonl(hd->sm.version);
	sm->payload = htonl(hd->sm.payload);
	sm->type = htonl(hd->sm.type);
	sm->sg = htonl(hd->sm.sg);
	sm->size = htonl(hd->sm.size);
	sm->offset = htonl(hd->sm.offset);

	if (trueload > 0) {
		++nio;
	}

	ret = writev(side->file_descriptor, io, nio);

	hd->sidem.version = ntohl(hd->sidem.version);

	SIDEUNLOCK(side);

	return (ret != (ssize_t) (io[0].iov_len + io[1].iov_len + io[2].iov_len));
}

	/* read from client, free return pointer if not Null */
int SetupSideMessage(struct handlerdata *hd)
{
	socklen_t hostlength = sizeof(union address);
	socklen_t peerlength = sizeof(union address);
	memset(&(hd->sidem), 0, sizeof(struct side_msg));
	if (getsockname(hd->file_descriptor, &(hd->sidem.host.sock), &hostlength)) {
		ERROR_CALL("Can't get socket host address for Sidetap info\n");
		return -1;
	}
	if (getpeername(hd->file_descriptor, &(hd->sidem.peer.sock), &peerlength)) {
		ERROR_CALL("Can't get socket peer address for Sidetap info\n");
		return -1;
	}
#if 0
	{
		char host[50];
		char serv[50];
		printf("Peer getnameinfo=%d\n", getnameinfo(&(hd->sidem.peer.sock), peerlength, host, 50, serv, 50, 0));
		printf("\tAddress=<%s>:<%s>\n", host, serv);
	}
	{
		char host[50];
		char serv[50];
		printf("Host getnameinfo=%d\n", getnameinfo(&(hd->sidem.host.sock), hostlength, host, 50, serv, 50, 0));
		printf("\tAddress=<%s>:<%s>\n", host, serv);
	}
#endif
	hd->sidem.version |= MakeServersidetap;
	hd->sidem.version |= MakeServertokens(peerlength);
	return 0;
}
