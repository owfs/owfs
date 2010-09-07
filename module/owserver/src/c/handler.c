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

/* Counters for persistent connections */
int persistent_connections = 0;
int handler_count = 0 ;

#if OW_MT						// Handler for multithreaded approach -- with ping

static void SingleHandler(struct handlerdata *hd);

/*
 * Main routine for actually handling a request
 * deals with a connection
 */
void Handler(int file_descriptor)
{
	struct handlerdata hd;
	struct timeval tv_low = { Globals.timeout_persistent_low, 0, };
	struct timeval tv_high = { Globals.timeout_persistent_high, 0, };
	int persistent = 0;

	hd.file_descriptor = file_descriptor;
	_MUTEX_INIT(hd.to_client);

	timersub(&tv_high, &tv_low, &tv_high);	// just the delta

	while (FromClient(&hd) == 0) {
		// Was persistence requested?
		int loop_persistent = ((hd.sm.control_flags & PERSISTENT_MASK) != 0);

		/* Persistence suppression? */
		if (Globals.no_persistence) {
			loop_persistent = 0;
		}

		/* Persistence logic */
		if (loop_persistent) {	/* Requested persistence */
			LEVEL_DEBUG("Persistence requested");
			if (persistent) {	/* already had persistence granted */
				hd.persistent = 1;	/* so keep it */
			} else {			/* See if available */

				PERSISTENCELOCK;

				if (persistent_connections < Globals.clients_persistent_high) {	/* ok */
					++persistent_connections;	/* global count */
					persistent = 1;	/* connection toggle */
					hd.persistent = 1;	/* for responses */
				} else {
					loop_persistent = 0;	/* denied! */
					hd.persistent = 0;	/* for responses */
				}

				PERSISTENCEUNLOCK;

			}
		} else {				/* No persistence requested this time */
			hd.persistent = 0;	/* for responses */
		}

		/* now set the sg flag because it usually is copied back to the client */
		if (loop_persistent) {
			hd.sm.control_flags |= PERSISTENT_MASK;
		} else {
			hd.sm.control_flags &= ~PERSISTENT_MASK;
		}

		/* Do the real work */
		SingleHandler(&hd);

		/* Now see if we should reloop */
		if (loop_persistent == 0) {
			break;				/* easiest one */
		}

		/* Shorter wait */
		if ( BAD(tcp_wait(file_descriptor, &tv_low)) ) {	// timed out
			/* test if below threshold for longer wait */

			PERSISTENCELOCK;

			/* store the test because the mutex locks the variable */
			loop_persistent = (persistent_connections < Globals.clients_persistent_low);

			PERSISTENCEUNLOCK;

			if (loop_persistent == 0) {
				break;			/* too many connections and we're slow */
			}

			/*  longer wait */
			if ( BAD(tcp_wait(file_descriptor, &tv_high)) ) {
				break;
			}
		}

		LEVEL_DEBUG("OWSERVER tcp connection persistence -- reusing connection now.");
	}

	LEVEL_DEBUG("OWSERVER handler done");
	_MUTEX_DESTROY(hd.to_client);
	// restore the persistent count
	if (persistent) {

		PERSISTENCELOCK;

		--persistent_connections;

		PERSISTENCEUNLOCK;

	}
}

static void SingleHandler(struct handlerdata *hd)
{
	timerclear(&hd->tv);

	LEVEL_DEBUG("START handler %s",hd->sp.path) ;

	gettimeofday(&(hd->tv), NULL);
		
	if (Globals.pingcrazy) {	// extra pings
		PingClient(hd);	// send the ping
		LEVEL_DEBUG("Extra ping (pingcrazy mode)");
	}

	PingLoop( hd ) ;

	if (hd->sp.path) {
		owfree(hd->sp.path);
		hd->sp.path = NULL;
	}
}

#else							/* no OW_MT */
void Handler(int file_descriptor)
{
	struct handlerdata hd;
	hd.file_descriptor = file_descriptor;
	if (FromClient(&hd) == 0) {
		DataHandler(&hd);
	}
	if (hd.sp.path) {
		owfree(hd.sp.path);
		hd.sp.path = NULL;
	}
}

#endif							/* OW_MT */
