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


/* Dirall, called from Handler with the following caveats: */
/* path is path, already parsed, and null terminated */
/* sm has been read, cm has been zeroed */
/* pn is configured */
/* Dir, will return: */
/* cm fully constructed for error message or null marker (end of directory elements */
/* cm.ret is also set to an error or 0 */
struct dirallhandlerstruct {
	struct charblob *cb;
	const struct parsedname *pn;
};

void DirallHandlerCallback(void *v, const struct parsedname *pn2)
{
	struct dirallhandlerstruct *dhs = v;
	char retbuffer[PATH_MAX];
	char *path = (KnownBus(dhs->pn)
				  && BusIsServer(dhs->pn->in)) ? dhs->pn->
		path_busless : dhs->pn->path;

	/* make sure path has a "/" before the file is added */
	size_t _pathlen = strlen(path);
	strcpy(retbuffer, path);
	if ((_pathlen == 0) || (retbuffer[_pathlen - 1] != '/')) {
		retbuffer[_pathlen] = '/';
		retbuffer[++_pathlen] = '\0';
	}

	LEVEL_DEBUG("owserver dir path = %s\n", SAFESTRING(pn2->path));
	if (pn2->selected_device != NULL) {
		FS_DirName(&retbuffer[_pathlen], PATH_MAX - _pathlen - 1, pn2);
	} else if (NotRealDir(pn2)) {
		FS_dirname_type(&retbuffer[_pathlen], PATH_MAX - _pathlen - 1,
						pn2);
	} else {
		FS_dirname_state(&retbuffer[_pathlen], PATH_MAX - _pathlen - 1,
						 pn2);
	}

	CharblobAdd(retbuffer, strlen(retbuffer), dhs->cb);
}

void *DirallHandler(struct handlerdata *hd, struct client_msg *cm,
					const struct parsedname *pn)
{
	uint32_t flags = 0;
	struct charblob cb;
	struct dirallhandlerstruct dhs = { &cb, pn, };
	char *ret = NULL;

	(void) hd;

	CharblobInit(&cb);

	// Now generate the directory (using the embedded callback function above for each element
	LEVEL_DEBUG
		("OWSERVER Dir-All SpecifiedBus=%d pn->bus_nr=%d path = %s\n",
		 SpecifiedBus(pn), pn->bus_nr, SAFESTRING(pn->path));

	// Now generate the directory using the callback function above for each element
	cm->ret = FS_dir_remote(DirallHandlerCallback, &dhs, pn, &flags);

	if (cm->ret) {
		cm->size = cm->payload = 0;
	} else if ((ret = strdup(cb.blob))) {
		cm->payload = cb.used + 1;
		cm->size = cb.used;
	} else {
		cm->ret = -ENOMEM;
		cm->size = cm->payload = 0;
	}
    cm->offset = flags;         /* send the flags in the offset slot */
    CharblobClear(&cb);
	return ret;
}
