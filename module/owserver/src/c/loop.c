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

#if OW_MT						// Handler for multithreaded approach -- with ping

static void LoopCleanup(struct handlerdata *hd);
static enum toclient_state Ping_or_Send( enum toclient_state last_toclient, struct handlerdata * hd );

struct timeval tv_long  = { 1 , 000000 } ; // 1 second
struct timeval tv_short = { 0 , 500000 } ; // 1/2 second

GOOD_OR_BAD LoopSetup(struct handlerdata *hd)
{
	int fd[2] ;
	my_pthread_mutex_init(&hd->to_client, Mutex.pmattr);
	hd->read_file_descriptor = -1;
	hd->write_file_descriptor = -1 ;
	if ( pipe(fd) == 0 ) {
		LoopCleanup(hd) ;
		return gbBAD ;
	}
	hd->read_file_descriptor = fd[0];
	hd->write_file_descriptor = fd[1];
	hd->toclient = toclient_postping ;
	return gbGOOD ;
}

static void LoopCleanup(struct handlerdata *hd)
{
	my_pthread_mutex_destroy(&hd->to_client);
	if (hd->read_file_descriptor > -1 ) {
		close(hd->read_file_descriptor);
	}
	if (hd->write_file_descriptor > -1 ) {
		close(hd->write_file_descriptor);
	}
}

static enum toclient_state Ping_or_Send( enum toclient_state last_toclient, struct handlerdata * hd )
{
	struct timeval tv ;
	fd_set read_set ;
	int select_value ;
	enum toclient_state next_toclient ;

	FD_ZERO( &read_set ) ;
	FD_SET( hd->read_file_descriptor, &read_set ) ;

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
	
	select_value = select( hd->read_file_descriptor+1, &read_set, NULL, NULL, &tv ) ;

	// read pipe shows final data was sent
	if ( select_value == 1 ) {
		return toclient_complete ;
	}
	
	TOCLIENTLOCK(hd);
	next_toclient = hd->toclient ;
	
	// select error. just send a ping
	if ( select_value < 0 ) {
		switch ( next_toclient ) {
			case toclient_complete:
				break ;
			case toclient_postmessage:
			case toclient_postping:
				PingClient(hd);	// send the ping
				next_toclient = toclient_postping ;
				hd->toclient = toclient_postping ;
				break ;
		}
	} else { // timeout
		switch ( next_toclient ) {
			case toclient_complete:
				break ;
			case toclient_postmessage:
				hd->toclient = toclient_postping ;
				break ;
			case toclient_postping:
				PingClient(hd);	// send the ping
				next_toclient = toclient_postping ;
				break ;
		}
	}

	TOCLIENTUNLOCK(hd);
	return next_toclient ;
}

void PingLoop(struct handlerdata *hd)
{
	enum toclient_state current_toclient = toclient_postping ;
	
	if ( BAD( LoopSetup(hd) ) ) {
		LoopCleanup(hd) ;
		return gbBAD ;
	}

	do {
		current_toclient = Ping_or_Send( current_toclient, hd ) ;
	} while ( current_toclient != toclient_complete ) ;
	
	LoopCleanup(hd);
	return gbGOOD ;
}

#endif							/* OW_MT */
