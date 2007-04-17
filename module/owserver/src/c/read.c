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
void *ReadHandler(struct handlerdata *hd, struct client_msg *cm,
				  struct one_wire_query * owq)
{
	char *retbuffer = NULL;
	ssize_t read_or_error;

	//printf("ReadHandler:\n");
	LEVEL_DEBUG
		("ReadHandler: From Client sm->payload=%d sm->size=%d sm->offset=%d\n",
		 hd->sm.payload, hd->sm.size, hd->sm.offset);

	if ((hd->sm.size <= 0) || (hd->sm.size > MAXBUFFERSIZE)) {
		cm->ret = -EMSGSIZE;
#ifdef VALGRIND
	} else if ((retbuffer = (char *) calloc(1, (size_t) hd->sm.size)) == NULL) {	// allocate return buffer
#else
	} else if ((retbuffer = (char *) malloc((size_t) hd->sm.size)) == NULL) {	// allocate return buffer
#endif
		cm->ret = -ENOBUFS;
    } else {
        OWQ_buffer(owq) = retbuffer ;
        read_or_error = FS_read_postparse(owq) ;
        printf("OWSERVER read on %s return = %d\n",PN(owq)->path,read_or_error) ;
        Debug_OWQ(owq) ;

        if (read_or_error <= 0) {
            free(retbuffer);
            retbuffer = NULL;
            cm->ret = read_or_error;
        } else {
			// make return size smaller (just large enough)
            cm->payload = read_or_error;
            cm->offset = hd->sm.offset;
            cm->size = read_or_error;
            cm->ret = read_or_error;
        }
	}
	LEVEL_DEBUG
		("ReadHandler: To Client cm->payload=%d cm->size=%d cm->offset=%d\n",
		 cm->payload, cm->size, cm->offset);
	LEVEL_DEBUG("ReadHandler: return size=%d [%*s]\n", cm->size, cm->size,
				retbuffer);
	return retbuffer;
}
