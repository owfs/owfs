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

/* Read, called from Handler with the following caveates: */
/* path is path, already parsed, and null terminated */
/* sm has been read, cm has been zeroed */
/* pn is configured */
/* Read, will return: */
/* cm fully constructed */
/* a malloc'ed string, that must be free'd by Handler */
/* The length of string in cm.payload */
/* If cm.payload is 0, then a NULL string is returned */
/* cm.ret is also set to an error <0 or the read length */
void *ReadHandler(struct server_msg *sm, struct client_msg *cm,
				  const struct parsedname *pn)
{
	char *retbuffer = NULL;
	ssize_t ret;

	//printf("ReadHandler:\n");
	LEVEL_DEBUG("ReadHandler: cm->payload=%d cm->size=%d cm->offset=%d\n",
				cm->payload, cm->size, cm->offset);
	LEVEL_DEBUG("ReadHandler: sm->payload=%d sm->size=%d sm->offset=%d\n",
				sm->payload, sm->size, sm->offset);
	cm->size = 0;
	cm->payload = 0;
	cm->offset = 0;

	if ((sm->size <= 0) || (sm->size > MAXBUFFERSIZE)) {
		cm->ret = -EMSGSIZE;
#ifdef VALGRIND
	} else if ((retbuffer = (char *) calloc(1, (size_t) sm->size)) == NULL) {	// allocate return buffer
#else
	} else if ((retbuffer = (char *) malloc((size_t) sm->size)) == NULL) {	// allocate return buffer
#endif
		cm->ret = -ENOBUFS;
	} else
		if ((ret =
			 FS_read_postparse(retbuffer, (size_t) sm->size,
							   (off_t) sm->offset, pn)) <= 0) {
		free(retbuffer);
		retbuffer = NULL;
		cm->ret = ret;
	} else {
		cm->payload = sm->size;
		cm->offset = sm->offset;
		cm->size = ret;
		cm->ret = ret;
	}
	LEVEL_DEBUG("ReadHandler: return size=%d [%*s]\n", sm->size, sm->size,
				retbuffer);
	return retbuffer;
}
