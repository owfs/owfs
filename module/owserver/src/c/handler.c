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


#if OW_MT						// Handler for multithreaded approach -- with ping

static void SingleHandler(struct handlerdata *hd);

pthread_mutex_t persistence_mutex = PTHREAD_MUTEX_INITIALIZER;
#define PERSISTENCELOCK    pthread_mutex_lock(   &persistence_mutex ) ;
#define PERSISTENCEUNLOCK  pthread_mutex_unlock( &persistence_mutex ) ;

/*
 * Main routine for actually handling a request
 * deals with a connection
 */
void Handler(int fd)
{
	struct handlerdata hd;
	struct timeval tv_low = { Global.timeout_persistent_low, 0, };
	struct timeval tv_high = { Global.timeout_persistent_high, 0, };
	int persistent = 0;

	hd.fd = fd;
	pthread_mutex_init(&hd.to_client, pmattr);

	timersub(&tv_high, &tv_low, &tv_high);	// just the delta

	while (FromClient(&hd) == 0) {
		int loop_persistent = ((hd.sm.sg & PERSISTENT_MASK) != 0);

		/* Persistence logic */
		if (loop_persistent) {	/* Requested persistence */
			if (persistent) {	/* already had persistence granted */
				hd.persistent = 1;	/* so keep it */
			} else {			/* See if available */

                PERSISTENCELOCK;

                if (persistent_connections < Global.clients_persistent_high) {	/* ok */
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
			hd.sm.sg |= PERSISTENT_MASK;
		} else {
			hd.sm.sg &= ~PERSISTENT_MASK;
		}

		/* Do the real work */
		SingleHandler(&hd);

		/* Now see if we should reloop */
		if (loop_persistent == 0)
			break;				/* easiest one */

		/* Shorter wait */
		if (tcp_wait(fd, &tv_low)) {	// timed out
			/* test if below threshold for longer wait */

            PERSISTENCELOCK;

            /* store the test because the mutex locks the variable */
			loop_persistent =
				(persistent_connections < Global.clients_persistent_low);

            PERSISTENCEUNLOCK;

            if (loop_persistent == 0)
				break;			/* too many connections and we're slow */

			/*  longer wait */
			if (tcp_wait(fd, &tv_high))
				break;
		}

		LEVEL_DEBUG
			("OWSERVER tcp connection persistence -- reusing connection now.\n");
	}

	LEVEL_DEBUG("OWSERVER handler done\n");
	pthread_mutex_destroy(&hd.to_client);
	// restore the persistent count
	if (persistent) {

        PERSISTENCELOCK;

        --persistent_connections;

        PERSISTENCEUNLOCK;

    }
}

/*
 * Routine for handling a single request
   returns 0 if ok, else non-zero for error
 */
static void SingleHandler(struct handlerdata *hd)
{
	struct client_msg ping_cm;
	struct timeval now;			// timer calculation
	struct timeval delta = { Global.timeout_network, 500000 };	// 1.5 seconds ping interval
	struct timeval result;		// timer calculation
	pthread_t thread;			// hanler thread id (not used)
	int loop = 1;				// ping loop flap

	timerclear(&hd->tv);

	memset(&ping_cm, 0, sizeof(struct client_msg));
	ping_cm.payload = -1;		/* flag for delay message */
	gettimeofday(&(hd->tv), NULL);

	//printf("OWSERVER pre-create\n");
	// PTHREAD_CREATE_DETACHED doesn't work for older uclibc... call pthread_detach() instead.

    if ( Global.pingcrazy ) { // extra pings
        TOCLIENTLOCK(hd);
            ToClient(hd->fd, &ping_cm, NULL);  // send the ping
        TOCLIENTUNLOCK(hd);
        LEVEL_DEBUG("Extra ping (pingcrazy mode)\n");
    }
	if (pthread_create(&thread, NULL, DataHandler, hd)) {
		LEVEL_DEBUG("OWSERVER:handler() can't create new thread\n");
		DataHandler(hd);		// do it without pings
		goto HandlerDone;
	}

	do {						// ping loop
#ifdef HAVE_NANOSLEEP
		struct timespec nano = { 0, 100000000 };	// .1 seconds (Note second element NANOsec)
		nanosleep(&nano, NULL);
#else /* HAVE_NANOSLEEP */
		usleep((unsigned long) 100000);
#endif /* HAVE_NANOSLEEP */
		if (!timerisset(&(hd->tv))) {	// flag that the other thread is done
			loop = 0;
		} else {				// check timing -- ping if expired
			gettimeofday(&now, NULL);	// current time
			timersub(&now, &delta, &result);	// less delay
			if (timercmp(&(hd->tv), &result, <) || Global.pingcrazy ) {	// test against last message time
				char *c = NULL;	// dummy argument
				ToClient(hd->fd, &ping_cm, c);	// send the ping
				//printf("OWSERVER ping\n") ;
				gettimeofday(&(hd->tv), NULL);	// reset timer
			}
		}
		TOCLIENTUNLOCK(hd);
	} while (loop);
  HandlerDone:
	if (hd->sp.path) {
		free(hd->sp.path);
		hd->sp.path = NULL;
	}
//printf("OWSERVER single handler done\n" ) ;
}

#else							/* no OW_MT */
void Handler(int fd)
{
	struct handlerdata hd;
	hd.fd = fd;
	if (FromClient(&hd) == 0) {
		DataHandler(&hd);
	}
	if (hd.sp.path) {
		free(hd.sp.path);
		hd.sp.path = NULL;
	}
//printf("OWSERVER handler done\n" ) ;
}

#endif							/* OW_MT */
