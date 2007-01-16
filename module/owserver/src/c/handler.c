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
 * Main routine for actually handling a request
 * deals with a connection
 */
void Handler(int fd)
{
	struct handlerdata hd = {
		fd,
#if OW_MT
		PTHREAD_MUTEX_INITIALIZER,
#endif							/* OW_MT */
		{0, 0},
	};

#if OW_MT
	struct client_msg ping_cm;
	struct timeval now;			// timer calculation
	struct timeval delta = { Global.timeout_network, 500000 };	// 1.5 seconds ping interval
	struct timeval result;		// timer calculation
	pthread_t thread;			// hanler thread id (not used)
	int loop = 1;				// ping loop flap

	memset(&ping_cm, 0, sizeof(struct client_msg));
	ping_cm.payload = -1;		/* flag for delay message */
	gettimeofday(&(hd.tv), NULL);

	//printf("OWSERVER pre-create\n");
	// PTHREAD_CREATE_DETACHED doesn't work for older uclibc... call pthread_detach() instead.

	if (pthread_create(&thread, NULL, DataHandler, &hd)) {
		LEVEL_DEBUG("OWSERVER:handler() can't create new thread\n");
		DataHandler(&hd);		// do it without pings
		return;
	}

	do {						// ping loop
#ifdef HAVE_NANOSLEEP
		struct timespec nano = { 0, 100000000 };	// .1 seconds (Note second element NANOsec)
		nanosleep(&nano, NULL);
#else
		usleep((unsigned long) 100000);
#endif
		TOCLIENTLOCK(&hd);
		if (!timerisset(&(hd.tv))) {	// flag that the other thread is done
			loop = 0;
		} else {				// check timing -- ping if expired
			gettimeofday(&now, NULL);	// current time
			timersub(&now, &delta, &result);	// less delay
			if (timercmp(&(hd.tv), &result, <)) {	// test against last message time
				char *c = NULL;	// dummy argument
				ToClient(hd.fd, &ping_cm, c);	// send the ping
				//printf("OWSERVER ping\n") ;
				gettimeofday(&(hd.tv), NULL);	// reset timer
			}
		}
		TOCLIENTUNLOCK(&hd);
	} while (loop);
#else							/* OW_MT */
	DataHandler(&hd);
#endif							/* OW_MT */
	//printf("OWSERVER handler done\n" ) ;
}
