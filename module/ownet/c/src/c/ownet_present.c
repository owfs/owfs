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

int OWNET_present(OWNET_HANDLE h, const char *onewire_path)
{
	unsigned char buffer[MAX_READ_BUFFER_SIZE];
	int return_value;

	struct request_packet s_request_packet;
	struct request_packet *rp = &s_request_packet;
	memset(rp, 0, sizeof(struct request_packet));

	CONNIN_RLOCK;
	rp->owserver = find_connection_in(h);
	if (rp->owserver == NULL) {
		CONNIN_RUNLOCK;
		return -EBADF;
	}

	rp->path = (onewire_path == NULL) ? "/" : onewire_path;
	rp->read_value = buffer;
	rp->data_length = MAX_READ_BUFFER_SIZE;
	rp->data_offset = 0;

	return_value = ServerPresence(rp);
	/* 0 is ok, <0 not found */
	
	CONNIN_RUNLOCK;
	return return_value;
}

