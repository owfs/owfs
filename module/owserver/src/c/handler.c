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
#define PERSISTENCELOCK    my_pthread_mutex_lock(   &persistence_mutex ) ;
#define PERSISTENCEUNLOCK  my_pthread_mutex_unlock( &persistence_mutex ) ;

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
	my_pthread_mutex_init(&hd.to_client, Mutex.pmattr);

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

		/* Sidetap handling */
		if (Sidebound_Control.active > 0) {
			struct connection_side *side;
			SetupSideMessage(&hd);
			for (side = Sidebound_Control.head; side != NULL; side = side->next) {
				FromClientSide(side, &hd);
			}
		}

		/* Do the real work */
		SingleHandler(&hd);

		/* Now see if we should reloop */
		if (loop_persistent == 0) {
			break;				/* easiest one */
		}

		/* Shorter wait */
		if (tcp_wait(file_descriptor, &tv_low)) {	// timed out
			/* test if below threshold for longer wait */

			PERSISTENCELOCK;

			/* store the test because the mutex locks the variable */
			loop_persistent = (persistent_connections < Globals.clients_persistent_low);

			PERSISTENCEUNLOCK;

			if (loop_persistent == 0) {
				break;			/* too many connections and we're slow */
			}

			/*  longer wait */
			if (tcp_wait(file_descriptor, &tv_high)) {
				break;
			}
		}

		LEVEL_DEBUG("OWSERVER tcp connection persistence -- reusing connection now.");
	}

	LEVEL_DEBUG("OWSERVER handler done");
	my_pthread_mutex_destroy(&hd.to_client);
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
unsigned long handler_count = 0 ;
struct client_msg ping_cm = {
	.version = 0 ,
	.payload = -1 ,
	.ret     = 0 ,
	.control_flags = 0 ,
	.size    = 0 ,
	.offset  = 0 ,
};

static void SingleHandler(struct handlerdata *hd)
{
	struct timeval now;			// timer calculation
	struct timeval delta = { Globals.timeout_network, 500000 };	// 1.5 seconds ping interval
	struct timeval result;		// timer calculation
	pthread_t thread;			// hanler thread id (not used)
	int loop = 1;				// ping loop flap
	struct timeval this_handler_start ;
	struct timeval this_handler_stop ;
	unsigned long this_handler_count = ++handler_count ;

	gettimeofday(&this_handler_start,0) ; // start for timing query handling
	timerclear(&hd->tv);
	LEVEL_DEBUG("START handler {%lu} %s",this_handler_count,hd->sp.path) ;

	gettimeofday(&(hd->tv), NULL);
#if OW_MT && defined(HAVE_SEM_TIMEDWAIT)
	sem_init(&(hd->complete_sem), 0, 0);
#endif
		
	//printf("OWSERVER pre-create\n");
	// PTHREAD_CREATE_DETACHED doesn't work for older uclibc... call pthread_detach() instead.

	if (Globals.pingcrazy) {	// extra pings
		TOCLIENTLOCK(hd);
		ToClient(hd->file_descriptor, &ping_cm, NULL);	// send the ping
		TOCLIENTUNLOCK(hd);
		LEVEL_DEBUG("Extra ping (pingcrazy mode)");
	}

	if (pthread_create(&thread, NULL, DataHandler, hd)) {
		LEVEL_DEBUG("OWSERVER:handler() can't create new thread");
		DataHandler(hd);		// do it without pings
		goto HandlerDone;
	}

	do {						// ping loop
#if OW_MT && defined(HAVE_SEM_TIMEDWAIT)
		int timeout = 100;  // max wait before testing hd->tv manually
		int rc, err = 0;
		struct timespec tspec_end;
		struct timeval tv_start, tv_end, tv_now, elapsed, left;
		
#ifdef USE_CLOCKGETTIME
		/* This require -lrt, and precision (nanoseconds) is a bit unneeded */
		if (clock_gettime(CLOCK_REALTIME, &tspec_end) == -1) {
			LEVEL_DEFAULT("clock_gettime failed");
		}
		LEVEL_DEBUG("clock_gettime returned time(NULL)=%ld %ld.%ld", time(NULL), tspec_end.tv_sec, tspec_end.tv_nsec);
#else
		/* micro seconds are good enough for this timeout */
		gettimeofday(&tv_start, NULL);
		tspec_end.tv_sec = tv_start.tv_sec;
		tspec_end.tv_nsec = tv_start.tv_usec*1000;
#endif
		tspec_end.tv_nsec += timeout*1000*1000;  // This is our end-time to wait until...
		if(tspec_end.tv_nsec >= 1000*1000*1000) {
			tspec_end.tv_nsec -= 1000*1000*1000;
			tspec_end.tv_sec += 1;
		}
		tv_end.tv_sec = tspec_end.tv_sec;
		tv_end.tv_usec = tspec_end.tv_nsec/1000;

		while(1) {
#if 0
			// This should be enough, but it doesn't work during debugging with gdb.
			while (((rc = sem_timedwait(&(hd->complete_sem), &tspec_end)) == -1) && ((err = errno) == EINTR)) {
				LEVEL_DEFAULT("restart sem_timedwait since EINTR");
				continue;       /* Restart if interrupted by handler */
			}
#else
			rc = sem_timedwait(&(hd->complete_sem), &tspec_end);
#endif

			if(rc < 0) {
				err = errno;
				gettimeofday(&tv_now, NULL);
				timersub(&tv_now, &tv_start, &elapsed);  // total waiting time...
				timersub(&tv_end, &tv_now,   &left);  // time left of "timeout"
			
				if(err == EINTR) {
					LEVEL_CALL("restart sem_timedwait since EINTR. elapsed=%ld.%03ld left=%ld.%03ld", elapsed.tv_sec, elapsed.tv_usec/1000, left.tv_sec, left.tv_usec/1000);
					continue;
				}
				if(err == ETIMEDOUT) {
					if(left.tv_sec >= 0) {
						/* Debugging the application with gdb will result into strange behavior from sem_timedwait.
						 * It might return too early with ETIMEDOUT even if there are more time left.
						 */
						LEVEL_CALL("Too early ETIMEDOUT from sem_timedwait elapsed=%ld.%03ld left=%ld.%03ld", elapsed.tv_sec, elapsed.tv_usec/1000, left.tv_sec, left.tv_usec/1000);
						//Too early ETIMEDOUT from sem_timedwait elapsed=0.022 left=179.977
						//sem_timedwait ok  time=0.077
						// Call sem_timedwait and wait again. tspec_end should be untouched and contain correct end-time.
						continue;
					} else {
						LEVEL_CALL("sem_timedwait timeout time=%ld.%03ld (timeout=%d ms)", elapsed.tv_sec, elapsed.tv_usec/1000, timeout);
					}
				} else {
					LEVEL_DEFAULT("sem_timedwait error %d rc=%d time=%ld.%03ld", err, elapsed.tv_sec, elapsed.tv_usec/1000);
				}
			} else {
				//LEVEL_DEFAULT("sem_timedwait ok  time=%ld.%03ld", elapsed.tv_sec, elapsed.tv_usec/1000);
			}
			break;
		}
	
#else
		
		// This will delay all persistant connections. (and result into cpu-usage)
		// Replace into a timed semaphore
#ifdef HAVE_NANOSLEEP
		struct timespec nano = { 0, 1000*1000 };	// .001 seconds (Note second element NANOsec)
		nanosleep(&nano, NULL);
#else							/* HAVE_NANOSLEEP */
		usleep((unsigned long) 1000);
#endif							/* HAVE_NANOSLEEP */

#endif

		TOCLIENTLOCK(hd);

		if (!timerisset(&(hd->tv))) {	// flag that the other thread is done
			LEVEL_DEBUG("UNPING handler {%lu} %s",this_handler_count,hd->sp.path) ;
			loop = 0;
		} else {				// check timing -- ping if expired
			gettimeofday(&now, NULL);	// current time
			timersub(&now, &delta, &result);	// less delay
			if (timercmp(&(hd->tv), &result, <) || Globals.pingcrazy) {	// test against last message time
				char *c = NULL;	// dummy argument
				LEVEL_DEBUG("PING handler {%lu} %s",this_handler_count,hd->sp.path) ;
				ToClient(hd->file_descriptor, &ping_cm, c);	// send the ping
				if (Sidebound_Control.active > 0) {
					struct connection_side *side;
					for (side = Sidebound_Control.head; side != NULL; side = side->next) {
						ToClientSide(side, &ping_cm, c, &(hd->sidem));
					}
				}
				gettimeofday(&(hd->tv), NULL);	// reset timer
			} else {
				LEVEL_DEBUG("NOPING handler {%lu} %s",this_handler_count,hd->sp.path) ;
			}
		}

		TOCLIENTUNLOCK(hd);

	} while (loop);

HandlerDone:
	gettimeofday(&this_handler_stop,0); // not for end of handling
	LEVEL_DEBUG("OWSERVER TIMING: Query %6lu Seconds %12.2f Path %s",
		this_handler_count,
		1.0*(this_handler_stop.tv_sec-this_handler_start.tv_sec)+.000001*(this_handler_stop.tv_usec-this_handler_start.tv_usec),
		hd->sp.path) ;
	LEVEL_DEBUG("STOP handler {%lu} %s",this_handler_count,hd->sp.path) ;
#if OW_MT && defined(HAVE_SEM_TIMEDWAIT)
	sem_destroy(&(hd->complete_sem));
#endif
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
//printf("OWSERVER handler done\n" ) ;
}

#endif							/* OW_MT */
