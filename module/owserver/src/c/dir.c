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
	const struct parsedname *pn;
};

static void DirHandlerCallback(void *v, const struct parsedname *pn2)
{
	struct dirhandlerstruct *dhs = (struct dirhandlerstruct *) v;
	char retbuffer[PATH_MAX];
#if 0
	char *path = (KnownBus(dhs->pn)
				  && (BusIsServer(dhs->pn->selected_connection))) ? dhs->pn->
		path_busless : dhs->pn->path;
#endif
char * path = dhs->pn->path ;      
	size_t _pathlen = strlen(path);

	LEVEL_DEBUG("owserver dir path = %s\n", SAFESTRING(pn2->path));

	/* make sure path ends in "/" before anything is tacked on */
	strcpy(retbuffer, path);
	if ((_pathlen == 0) || (retbuffer[_pathlen - 1] != '/')) {
        retbuffer[_pathlen] = '/';
		retbuffer[++_pathlen] = '\0';
	}
	
    if (pn2->selected_device != NULL) {
        FS_DirName(&retbuffer[_pathlen], PATH_MAX - _pathlen - 1, pn2);
	} else if (NotRealDir(pn2)) {
        FS_dirname_type(&retbuffer[_pathlen], PATH_MAX - _pathlen - 1, pn2);
	} else {
        FS_dirname_state(&retbuffer[_pathlen], PATH_MAX - _pathlen - 1, pn2);
	}
    dhs->cm->size = strlen(retbuffer);
	dhs->cm->payload = dhs->cm->size + 1;
	dhs->cm->ret = 0;

	TOCLIENTLOCK(dhs->hd);
	ToClient(dhs->hd->file_descriptor, dhs->cm, retbuffer);	// send this directory element
	gettimeofday(&(dhs->hd->tv), NULL);	// reset timer
	TOCLIENTUNLOCK(dhs->hd);
}

void DirHandler(struct handlerdata *hd, struct client_msg *cm,
				const struct parsedname *pn)
{
	uint32_t flags = 0;
	struct dirhandlerstruct dhs = { hd, cm, pn, };

	LEVEL_CALL("DirHandler: pn->path=%s\n", pn->path);

	// Settings for all directory elements
	cm->payload = strlen(pn->path) + 1 + OW_FULLNAME_MAX + 2;

	LEVEL_DEBUG("OWSERVER SpecifiedBus=%d path=%s\n",
				SpecifiedBus(pn),  SAFESTRING(pn->path));
    
    if ( hd->sm.payload >= PATH_MAX ) {
        cm->ret = -EMSGSIZE ;
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
