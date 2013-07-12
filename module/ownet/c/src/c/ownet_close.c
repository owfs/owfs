/*
$Id$
    OWFS -- One-Wire filesystem
    OWHTTPD -- One-Wire Web Server
    Written 2003 Paul H Alfille
    email: paul.alfille@gmail.com
    Released under the GPL
    See the header file: ow.h for full attribution
    1wire/iButton system from Dallas Semiconductor
*/

/* ow_server talks to the server, sending and recieving messages */
/* this is an alternative to direct bus communication */

#include "ownetapi.h"
#include "ow_server.h"

/* void OWNET_close( OWNET_HANDLE h)
   close a particular owserver connection
*/
void OWNET_close(OWNET_HANDLE h)
{
	CONNIN_WLOCK;
	FreeIn(find_connection_in(h));
	CONNIN_WUNLOCK;
}

/* void OWNET_closeall( void )
   close all owserver connections
*/
void OWNET_closeall(void)
{
	struct connection_in *target;

	CONNIN_WLOCK;
	// step through head_inbound_list linked list
	for (target = head_inbound_list; target != NULL; target = target->next) {
		FreeIn(target);
	}
	CONNIN_WUNLOCK;
}

/* void OWNET_finish( void )
   close all owserver connections and free all memory
*/
void OWNET_finish(void)
{
	CONNIN_WLOCK;
	FreeInAll();
	head_inbound_list = NULL;
	CONNIN_WUNLOCK;
}
