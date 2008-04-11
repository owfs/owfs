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

/* Dir, called from Handler with the following caveats: */
/* path is path, already parsed, and null terminated */
/* sm has been read, cm has been zeroed */
/* pn is configured */
/* Dir, will return: */
/* cm fully constructed for error message or null marker (end of directory elements */
/* cm.ret is also set to an error or 0 */
struct dirhandlerstruct {
	struct handlerdata *hd;
	struct client_msg *cm;
};

static void DirHandlerCallback(void *v, const struct parsedname *pn_entry)
{
	struct dirhandlerstruct *dhs = (struct dirhandlerstruct *) v;
	char *path = pn_entry->path;

    LEVEL_DEBUG("owserver Calling dir=%s\n", SAFESTRING(path));

	dhs->cm->size = strlen(path);
	dhs->cm->payload = dhs->cm->size + 1;
	dhs->cm->ret = 0;

	TOCLIENTLOCK(dhs->hd);
	ToClient(dhs->hd->file_descriptor, dhs->cm, path);	// send this directory element
    if ( count_sidebound_connections > 0 ) {
        struct connection_side * side ;
        for ( side=head_sidebound_list ; side!=NULL ; side = side->next ) {
            ToClientSide(side, dhs->cm, path, &(dhs->hd->sidem) );
        }
    }
    gettimeofday(&(dhs->hd->tv), NULL);	// reset timer
	TOCLIENTUNLOCK(dhs->hd);
}

void DirHandler(struct handlerdata *hd, struct client_msg *cm, const struct parsedname *pn)
{
	uint32_t flags = 0;
	struct dirhandlerstruct dhs = { hd, cm, };

	LEVEL_CALL("DirHandler: pn->path=%s\n", pn->path);

	// Settings for all directory elements
	cm->payload = strlen(pn->path) + 1 + OW_FULLNAME_MAX + 2;

	LEVEL_DEBUG("OWSERVER SpecifiedBus=%d path=%s\n", SpecifiedBus(pn), SAFESTRING(pn->path));

	if (hd->sm.payload >= PATH_MAX) {
		cm->ret = -EMSGSIZE;
	} else {
		// Now generate the directory using the callback function above for each element
		cm->ret = FS_dir_remote(DirHandlerCallback, &dhs, pn, &flags);
	}

	// Finished -- send some flags and set up for a null element to tell client we're done
	cm->offset = flags;			/* send the flags in the offset slot */
	/* Now null entry to show end of directory listing */
	cm->payload = cm->size = 0;
	//printf("DirHandler: DIR done ret=%d flags=%d\n", cm->ret, flags);
}
