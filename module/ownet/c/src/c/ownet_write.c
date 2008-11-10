/*
$Id$
    OWFS -- One-Wire filesystem
    OWHTTPD -- One-Wire Web Server
    Written 2003 Paul H Alfille
    email: palfille@earthlink.net
    Released under the GPL
    See the header file: ow.h for full attribution
    1wire/iButton system from Dallas Semiconductor
*/

/* ow_server talks to the server, sending and recieving messages */
/* this is an alternative to direct bus communication */

#include "ownetapi.h"
#include "ow_server.h"

int OWNET_put(OWNET_HANDLE h, const char *onewire_path, const char *value_string, size_t size)
{
	struct request_packet s_request_packet;
	struct request_packet *rp = &s_request_packet;
	int return_value;
	memset(rp, 0, sizeof(struct request_packet));

	CONNIN_RLOCK;
	rp->owserver = find_connection_in(h);
	if (rp->owserver == NULL) {
		CONNIN_RUNLOCK;
		return -EBADF;
	}

	rp->path = (onewire_path == NULL) ? "/" : onewire_path;
	rp->write_value = (const unsigned char *) value_string;
	rp->data_length = size;
	rp->data_offset = 0;

	return_value = ServerWrite(rp);

	CONNIN_RUNLOCK;
	return return_value;
}

int OWNET_lwrite(OWNET_HANDLE h, const char *onewire_path, const char *value_string, size_t size, off_t offset)
{
	struct request_packet s_request_packet;
	struct request_packet *rp = &s_request_packet;
	int return_value;
	memset(rp, 0, sizeof(struct request_packet));

	CONNIN_RLOCK;
	rp->owserver = find_connection_in(h);
	if (rp->owserver == NULL) {
		CONNIN_RUNLOCK;
		return -EBADF;
	}

	rp->path = (onewire_path == NULL) ? "/" : onewire_path;
	rp->write_value = (const unsigned char *) value_string;
	rp->data_length = size;
	rp->data_offset = offset;

	return_value = ServerWrite(rp);

	CONNIN_RUNLOCK;
	return return_value;
}
