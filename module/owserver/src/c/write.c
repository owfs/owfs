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

/* Write, called from Handler with the following caveates: */
/* path is path, already parsed, and null terminated */
/* sm has been read, cm has been zeroed */
/* pn is configured */
/* data is what will be written, of sm->size length */
/* cm fully constructed */
/* cm.ret is also set to an error <0 or the written length */
void WriteHandler(struct handlerdata *hd, struct client_msg *cm, struct one_wire_query *owq)
{
	int ret;

	LEVEL_DEBUG("WriteHandler: hd->sm.payload=%d hd->sm.size=%d hd->sm.offset=%d OWQ_size=%d OWQ_offset=%d", hd->sm.payload, hd->sm.size, hd->sm.offset, OWQ_size(owq), OWQ_offset(owq));
	ret = FS_write_postparse(owq);

	//printf("Handler: WRITE done\n");
	if (ret < 0) {
		cm->size = 0;
		cm->ret = ret;
	} else {
		cm->size = ret;
		cm->ret = 0;
		cm->control_flags = OWQ_pn(owq).control_flags;
		if (hd->persistent)
			cm->control_flags |= PERSISTENT_MASK;
	}
}
