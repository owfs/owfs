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
 *   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
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

static void LoopCleanup(struct handlerdata *hd);
static enum toclient_state Ping_or_Send( enum toclient_state last_toclient, struct handlerdata * hd );
static GOOD_OR_BAD LoopSetup(struct handlerdata *hd) ;

static GOOD_OR_BAD LoopSetup(struct handlerdata *hd)
{
	_MUTEX_INIT(hd->to_client);
	if ( pipe(hd->ping_pipe) != 0 ) {
		ERROR_DEBUG("Cannot create pipe pair for keep-alive pulses") ;
		return gbBAD ;
	}
//	fcntl (hd->ping_pipe[fd_pipe_read], F_SETFD, FD_CLOEXEC); // for safe forking
//	fcntl (hd->ping_pipe[fd_pipe_write], F_SETFD, FD_CLOEXEC); // for safe forking
	hd->toclient = toclient_postping ;
	return gbGOOD ;
}

static void LoopCleanup(struct handlerdata *hd)
{
	_MUTEX_DESTROY(hd->to_client);
	if (hd->ping_pipe[fd_pipe_read] > -1 ) {
		close(hd->ping_pipe[fd_pipe_read]) ;
	}
	if (hd->ping_pipe[fd_pipe_write] > -1 ) {
		close(hd->ping_pipe[fd_pipe_write]) ;
	}
}

struct timeval tv_long  = { 1 , 000000 } ; // 1 second
struct timeval tv_short = { 0 , 500000 } ; // 1/2 second

static enum toclient_state Ping_or_Send( enum toclient_state last_toclient, struct handlerdata * hd )
{
	struct timeval tv ;
	fd_set read_set ;
	int select_value ;
	enum toclient_state next_toclient ;

	FD_ZERO( &read_set ) ;
	FD_SET( hd->ping_pipe[fd_pipe_read], &read_set ) ;

	switch ( last_toclient ) {
		case toclient_postmessage:
			tv = tv_short ;
			break ;
		case toclient_complete:
			return toclient_complete ;
		case toclient_postping:
			tv = tv_long ;
			break ;
	}

	select_value = select( hd->ping_pipe[fd_pipe_read]+1, &read_set, NULL, NULL, &tv ) ;

	TOCLIENTLOCK(hd);
	next_toclient = hd->toclient ;
	
	switch ( select_value ) {
		case 1: // message from other thread that we're done
			// some architectures (like MIPS) want this check inside the lock
			next_toclient = toclient_complete ;
			break ;
		case 0: // timeout -- time for ping?
			switch ( next_toclient ) {
				case toclient_complete:
					// crossed paths, we're really done
					break ;
				case toclient_postmessage:
					LEVEL_DEBUG("Ping forestalled by a directory element");
					hd->toclient = toclient_postping ;
					break ;
				case toclient_postping:
					LEVEL_DEBUG("Taking too long, send a keep-alive pulse");
					PingClient(hd);	// send the ping
					next_toclient = toclient_postping ;
					break ;
			}
			break ;
		default: // select error
			LEVEL_DEBUG("Select problem in keep-alive pulsing");
			switch ( next_toclient ) {
				case toclient_complete:
					// fortunately we're done
					break ;
				case toclient_postmessage:
				case toclient_postping:
					// not done. Send a ping and resync the process
					PingClient(hd);	// send the ping
					next_toclient = toclient_postping ;
					hd->toclient = toclient_postping ;
					break ;
			}
			break ;
	}

	TOCLIENTUNLOCK(hd);
	return next_toclient ;
}

void PingLoop(struct handlerdata *hd)
{
	enum toclient_state current_toclient = toclient_postping ;
	if ( GOOD( LoopSetup(hd) ) ) {
		pthread_t thread ;
		
		// Create DataHandler
		if (pthread_create(&thread, DEFAULT_THREAD_ATTR, DataHandler, hd) != 0) {
			LEVEL_DEBUG("OWSERVER:handler() can't create new thread");
			DataHandler(hd);		// do it without pings
		} else {
			// ping vs data loop
			do {
				current_toclient = Ping_or_Send( current_toclient, hd ) ;
			} while ( current_toclient != toclient_complete ) ;

			if (pthread_join(thread, NULL) != 0) {
				LEVEL_DEBUG("Error waiting for finish of data thread");
			}
		}
		LoopCleanup(hd);
	}
}
