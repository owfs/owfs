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
/* The length of string is cm.payload */
/* If cm.payload is 0, then a NULL string is returned */
/* cm.ret is also set to an error <0 or the read length */
void *ReadHandler(struct handlerdata *hd, struct client_msg *cm, struct one_wire_query *owq)
{
	char *retbuffer = NULL;
	ssize_t read_or_error;

	LEVEL_DEBUG("ReadHandler:\n");
	if (hd == NULL || owq == NULL || cm == NULL) {
		LEVEL_DEBUG("ReadHandler: illegal null inputs hd==%p owq==%p cm==%p\n", hd, owq, cm);
		return NULL;			// only sane response for bad inputs
	}

	LEVEL_DEBUG("ReadHandler: From Client sm->payload=%d sm->size=%d sm->offset=%d\n", hd->sm.payload, hd->sm.size, hd->sm.offset);

	if (hd->sm.payload >= PATH_MAX) {
		cm->ret = -EMSGSIZE;
	} else if ((hd->sm.size <= 0) || (hd->sm.size > MAX_OWSERVER_PROTOCOL_PACKET_SIZE)) {
		cm->ret = -EMSGSIZE;
		LEVEL_DEBUG("ReadHandler: error hd->sm.size == %d\n", hd->sm.size);
#ifdef VALGRIND
	} else if ((retbuffer = (char *) calloc(1, (size_t) hd->sm.size + 1)) == NULL) {	// allocate return buffer
#else
	} else if ((retbuffer = (char *) malloc((size_t) hd->sm.size + 1)) == NULL) {	// allocate return buffer
#endif
		LEVEL_DEBUG("ReadHandler: can't allocate memory\n");
		cm->ret = -ENOBUFS;
	} else {
		struct parsedname *pn = PN(owq);
		char *path = "";
		if (pn) {
			if (pn->path)
				path = pn->path;
			LEVEL_DEBUG("ReadHandler: call FS_read_postparse on %s\n", path);
		} else {
			LEVEL_DEBUG("ReadHandler: call FS_read_postparse pn==NULL\n");
		}
		OWQ_buffer(owq) = retbuffer;
		read_or_error = FS_read_postparse(owq);
		if (pn) {
			if (pn->path)
				path = pn->path;
			LEVEL_DEBUG("ReadHandler: FS_read_postparse read on %s return = %d\n", path, read_or_error);
		} else {
			LEVEL_DEBUG("ReadHandler: FS_read_postparse pn==NULL return = %d\n", read_or_error);
		}

		Debug_OWQ(owq);

		if (read_or_error <= 0) {
			LEVEL_DEBUG("ReadHandler: FS_read_postparse error %d\n", read_or_error);
			free(retbuffer);
			retbuffer = NULL;
			cm->ret = read_or_error;
		} else {
			LEVEL_DEBUG("ReadHandler: FS_read_postparse ok size=%d\n", read_or_error);
			// make return size smaller (just large enough)
			cm->payload = read_or_error;
			cm->offset = hd->sm.offset;
			cm->size = read_or_error;
			cm->ret = read_or_error;
			retbuffer[cm->size] = '\0';	// end with null for debug output
		}
	}
	LEVEL_DEBUG("ReadHandler: To Client cm->payload=%d cm->size=%d cm->offset=%d\n", cm->payload, cm->size, cm->offset);
	if (cm->size > 0 && retbuffer)
		LEVEL_DEBUG("ReadHandler: return size=%d [%*s]\n", cm->size, cm->size, retbuffer);
	return retbuffer;
}
