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

/* read from client, free return pointer if not Null */
int FromClient(int fd, struct server_msg *sm, struct serverpackage *sp)
{
	char *msg;
	ssize_t trueload;
	struct timeval tv = { Global.timeout_server, 0, };
	//printf("FromClient\n");

	/* Clear return structure */
	memset(sp, 0, sizeof(struct serverpackage));

	if (tcp_read(fd, sm, sizeof(struct server_msg), &tv) !=
		sizeof(struct server_msg)) {
		sm->type = msg_error;
		return -EIO;
	}

	sm->version = ntohl(sm->version);
	sm->payload = ntohl(sm->payload);
	sm->size = ntohl(sm->size);
	sm->type = ntohl(sm->type);
	sm->sg = ntohl(sm->sg);
	sm->offset = ntohl(sm->offset);
	LEVEL_DEBUG
		("FromClient payload=%d size=%d type=%d tempscale=%X offset=%d\n",
		 sm->payload, sm->size, sm->type, sm->sg, sm->offset);
	//printf("<%.4d|%.4d\n",sm->type,sm->payload);
	trueload = sm->payload;
	if (isServermessage(sm->version))
		trueload += sizeof(union antiloop) * Servertokens(sm->version);
	if (trueload == 0)
		return 0;

	/* valid size? */
	if ((sm->payload < 0) || (trueload > MAXBUFFERSIZE)) {
		sm->type = msg_error;
		return -EMSGSIZE;
	}

	/* Can allocate space? */
	if ((msg = (char *) malloc(trueload)) == NULL) {	/* create a buffer */
		sm->type = msg_error;
		return -ENOMEM;
	}

	/* read in data */
	if (tcp_read(fd, msg, trueload, &tv) != trueload) {	/* read in the expected data */
		sm->type = msg_error;
		free(msg);
		return -EIO;
	}

	/* path has null termination? */
	if (sm->payload) {
		int pathlen;
		if (memchr(msg, 0, (size_t) sm->payload) == NULL) {
			sm->type = msg_error;
			free(msg);
			return -EINVAL;
		}
		pathlen = strlen(msg) + 1;
		sp->data = (BYTE *) & msg[pathlen];
		sp->datasize = sm->payload - pathlen;
	} else {
		sp->data = NULL;
		sp->datasize = 0;
	}

	if (isServermessage(sm->version)) {	/* make sure no loop */
		size_t i;
		char *p = &msg[sm->payload];	// end of normal buffer
		sp->tokenstring = (BYTE *) p;
		sp->tokens = Servertokens(sm->version);
		for (i = 0; i < sp->tokens; ++i, p += sizeof(union antiloop)) {
			if (memcmp(p, &(Global.Token), sizeof(union antiloop)) == 0) {
				free(msg);
				sm->type = msg_error;
				LEVEL_DEBUG("owserver loop suppression\n");
				return -ELOOP;
			}
		}
	}
	sp->path = msg;
	return 0;
}
