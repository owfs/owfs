/*
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
 *   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
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
int FromClient(struct handlerdata *hd)
{
	BYTE *msg;
	ssize_t trueload;
	size_t actual_read ;
	struct timeval tv = { Globals.timeout_server, 0, };

	/* Clear return structure */
	memset(&hd->sp, 0, sizeof(struct serverpackage));

	/* read header */
	tcp_read(hd->file_descriptor, (BYTE *) &hd->sm, sizeof(struct server_msg), &tv, &actual_read) ;
	if (actual_read != sizeof(struct server_msg)) {
		hd->sm.type = msg_error;
		return -EIO;
	}

	/* translate endian state */
	hd->sm.version = ntohl(hd->sm.version);
	hd->sm.payload = ntohl(hd->sm.payload);
	hd->sm.type = ntohl(hd->sm.type);
	hd->sm.control_flags = ntohl(hd->sm.control_flags);
	hd->sm.size = ntohl(hd->sm.size);
	hd->sm.offset = ntohl(hd->sm.offset);

	LEVEL_DEBUG("FromClient payload=%d size=%d type=%d sg=0x%X offset=%d", hd->sm.payload, hd->sm.size, hd->sm.type, hd->sm.control_flags, hd->sm.offset);

	/* figure out length of rest of message: payload plus tokens */
	trueload = hd->sm.payload;
	if (isServermessage(hd->sm.version)) {
		trueload += sizeof(struct antiloop) * Servertokens(hd->sm.version);
		LEVEL_DEBUG("FromClient (servermessage) payload=%d nrtokens=%d trueload=%d size=%d type=%d controlflags=0x%X offset=%d", hd->sm.payload, Servertokens(hd->sm.version), trueload, hd->sm.size, hd->sm.type, hd->sm.control_flags, hd->sm.offset);
	} else {
		LEVEL_DEBUG("FromClient (no servermessage) payload=%d size=%d type=%d controlflags=0x%X offset=%d", hd->sm.payload, hd->sm.size, hd->sm.type, hd->sm.control_flags, hd->sm.offset);
	}
	if (trueload == 0) {
		return 0;
	}

	/* valid size? */
	if ((hd->sm.payload < 0) || (trueload > MAX_OWSERVER_PROTOCOL_PAYLOAD_SIZE)) {
		hd->sm.type = msg_error;
		return -EMSGSIZE;
	}

	/* Can allocate space? */
	if ((msg = owmalloc(trueload+2)) == NULL) {	/* create a buffer */
		// Adds an extra byte for the path null
		hd->sm.type = msg_error;
		return -ENOMEM;
	}

	/* read in data */
	tcp_read(hd->file_descriptor, msg, trueload, &tv, &actual_read) ;
	if ((ssize_t)actual_read != trueload) {	/* read in the expected data */
		hd->sm.type = msg_error;
		goto BADDATA ;
	}

	/* New algorithm as of 2.9p4 -- no longer use terminating null as path length
	 * now use payload length and data size (for writes)
	 * */
	if (hd->sm.payload) {
		int pathlen = hd->sm.payload ;
		if ( hd->sm.type == msg_write ) {
			// only for write -- everything else has just path
			pathlen -= hd->sm.size ;
			if ( pathlen <= 0 ) {
				LEVEL_DEBUG("Data size mismatch") ;
				goto BADDATA ;
			}
			if ( msg[pathlen-1] == '\0' ) {
				// already null-terminated string
				// no reason to increase size, shift data, and add null
				hd->sp.data = & msg[pathlen];
			} else {
				// make room for null at end of path (needed for some clients)
				// we allocated extra space and will use memmove that allows overlapping
				memmove( &msg[pathlen+1], &msg[pathlen], hd->sm.size ) ;
				msg[pathlen] = '\0' ; // terminating null
				hd->sp.data = & msg[pathlen+1];
			}
			hd->sp.datasize = hd->sm.size;
		} else {
			// add null for the rest
			msg[hd->sm.payload] = '\0' ; // terminating null
			hd->sp.data = NULL;
			hd->sp.datasize = 0;
		}
		hd->sp.path = (char *) msg;
	} else {
		hd->sp.data = NULL;
		hd->sp.datasize = 0;
	}

	if (isServermessage(hd->sm.version)) {	/* make sure no loop */
		size_t i;
		BYTE *p = &msg[hd->sm.payload];	// end of normal buffer
		hd->sp.tokenstring = p;
		hd->sp.tokens = Servertokens(hd->sm.version);
		for (i = 0; i < hd->sp.tokens; ++i, p += sizeof(struct antiloop)) {
			if (memcmp(p, &(Globals.Token), sizeof(struct antiloop)) == 0) {
				hd->sm.type = msg_error;
				LEVEL_CALL("owserver loop suppression");
				goto BADDATA ;
			}
		}
	}
	return 0;
	
BADDATA:
	owfree(msg);
	return -EINVAL;
}
