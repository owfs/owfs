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

OWNET_HANDLE OWNET_init(const char *owserver_tcp_address_and_port)
{
	OWNET_HANDLE handle;
	struct connection_in *slot_found;

	static int deja_vue = 0;
	// poor man's lock for the Lib Setup and Lock Setup
	if (++deja_vue == 1) {
		/* Setup the multithreading synchronizing locks */
		LockSetup();
	}

	CONNIN_WLOCK;

	slot_found = NewIn();
	// Could we create or reclaim a slot?
	if (slot_found == NULL) {
		handle = -ENOMEM;
	} else {

		if (owserver_tcp_address_and_port == NULL || owserver_tcp_address_and_port[0] == '\0') {
			slot_found->name = strdup("4304");
		} else {
			slot_found->name = strdup(owserver_tcp_address_and_port);
		}
		slot_found->busmode = bus_server;
		if (Server_detect(slot_found) == 0) {
			handle = slot_found->handle;
		} else {
			FreeIn(slot_found);
			handle = -EADDRNOTAVAIL;
		}
	}
	CONNIN_WUNLOCK;
	return handle;
}
