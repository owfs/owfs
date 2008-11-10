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

/*
 * lower level routine for actually handling a request
 * deals with data (ping is handled higher)
 */
void *DataHandler(void *v)
{
	struct handlerdata *hd = v;
	char *retbuffer = NULL;
	struct client_msg cm;

#if OW_MT
	pthread_detach(pthread_self());
#endif

#if OW_CYGWIN
	/* Random generator seem to need initialization for each new thread
	 * If not, seed will be reset and rand() will return 0 the first call.
	 */
	{
		struct timeval tv;
		gettimeofday(&tv, NULL);
		srand((unsigned int) tv.tv_usec);
	}
#endif

	//printf("OWSERVER message type = %d\n",sm.type ) ;
	memset(&cm, 0, sizeof(struct client_msg));
	cm.version = MakeServerprotocol(OWSERVER_PROTOCOL_VERSION);
	cm.sg = hd->sm.sg;			// default flag return -- includes persistence state

	/* Pre-handling for special testing mode to exclude certain messsages */
	switch ((enum msg_classification) hd->sm.type) {
	case msg_dirall:
		if (Globals.no_dirall) {
			hd->sm.type = msg_error;
		}
		break;
	case msg_get:
		if (Globals.no_get) {
			hd->sm.type = msg_error;
		}
		break;
	default:
		break;
	}

	/* Now Check message types and only parse valid messages */
	switch ((enum msg_classification) hd->sm.type) {	// outer switch
	case msg_read:				// good message
	case msg_write:				// good message
	case msg_dir:				// good message
	case msg_presence:			// good message
	case msg_dirall:			// good message
	case msg_get:				// good message
		if (hd->sm.payload == 0) {	/* Bad query -- no data after header */
			cm.ret = -EBADMSG;
		} else {
			OWQ_allocate_struct_and_pointer(owq);
			struct parsedname *pn = PN(owq);

			/* Parse the path string and crete  query object */
			LEVEL_CALL("owserver: parse path=%s\n", hd->sp.path);
			cm.ret = FS_OWQ_create(hd->sp.path, NULL, hd->sm.size, hd->sm.offset, owq) ;
			if ( cm.ret != 0 ) {
				break;
			}

			/* Use client persistent settings (temp scale, display mode ...) */
			pn->sg = hd->sm.sg;
			/* Antilooping tags */
			pn->tokens = hd->sp.tokens;
			pn->tokenstring = hd->sp.tokenstring;
			//printf("Handler: sm.sg=%X pn.state=%X\n", sm.sg, pn.state);
			//printf("Scale=%s\n", TemperatureScaleName(SGTemperatureScale(sm.sg)));

			switch ((enum msg_classification) hd->sm.type) {
			case msg_presence:
				LEVEL_CALL("Presence message on %s bus number=%d\n", SAFESTRING(pn->path), pn->known_bus->index);
				// Basically, if we were able to ParsedName it's here!
				cm.size = cm.payload = 0;
				cm.ret = 0;		// good answer
				break;
			case msg_read:
				LEVEL_CALL("Read message\n");
				retbuffer = ReadHandler(hd, &cm, owq);
				LEVEL_DEBUG("Read message done value=%p\n", retbuffer);
				break;
			case msg_write:{
					LEVEL_CALL("Write message\n");
					if ((hd->sp.datasize <= 0)
						|| ((int) hd->sp.datasize < hd->sm.size)) {
						cm.ret = -EMSGSIZE;
					} else {
						/* set buffer (size already set) */
						OWQ_buffer(owq) = (ASCII *) hd->sp.data;
						WriteHandler(hd, &cm, owq);
					}
				}
				break;
			case msg_dir:
				LEVEL_CALL("Directory message (one at a time)\n");
				DirHandler(hd, &cm, pn);
				break;
			case msg_dirall:
				LEVEL_CALL("Directory message (all at once)\n");
				retbuffer = DirallHandler(hd, &cm, pn);
				break;
			case msg_get:
				if (IsDir(pn)) {
					LEVEL_CALL("Get -> Directory message (all at once)\n");
					retbuffer = DirallHandler(hd, &cm, pn);
				} else {
					LEVEL_CALL("Get -> Read message\n");
					retbuffer = ReadHandler(hd, &cm, owq);
				}
				break;
			default:			// never reached
				LEVEL_CALL("Error: unknown message %d\n", (int) hd->sm.type);
				break;
			}
			FS_OWQ_destroy(owq);
			LEVEL_DEBUG("RealHandler: FS_ParsedName_destroy done\n");
		}
		break;
	case msg_nop:				// "bad" message
		LEVEL_CALL("NOP message\n");
		cm.ret = 0;
		break;
	case msg_size:				// no longer used
	case msg_error:
	default:					// "bad" message
		cm.ret = -ENOMSG;
		LEVEL_CALL("No message\n");
		break;
	}
	LEVEL_DEBUG("RealHandler: cm.ret=%d\n", cm.ret);

	TOCLIENTLOCK(hd);
	if (cm.ret != -EIO) {
		ToClient(hd->file_descriptor, &cm, retbuffer);
		if (Sidebound_Control.active > 0) {
			struct connection_side *side;
			for (side = Sidebound_Control.head; side != NULL; side = side->next) {
				ToClientSide(side, &cm, retbuffer, &(hd->sidem));
			}
		}
	}
	timerclear(&(hd->tv));
	TOCLIENTUNLOCK(hd);
	if (retbuffer) {
		free(retbuffer);
	}
	LEVEL_DEBUG("RealHandler: done\n");
	return NULL;
}
