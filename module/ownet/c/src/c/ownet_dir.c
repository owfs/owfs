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

static void dirlist_callback(void *v, const char *data_element)
{
	struct charblob *cb = v;
	CharblobAdd(data_element, strlen(data_element), cb);
}

int OWNET_dirlist(OWNET_HANDLE h, const char *onewire_path, char **return_string)
{
	struct charblob s_charblob;
	struct charblob *cb = &s_charblob;
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

	CharblobInit(cb);

	if (ServerDir(dirlist_callback, cb, rp) < 0) {
		CharblobClear(cb);
		return_value = -EINVAL;
	} else {
		return_string[0] = cb->blob;
		return_value = cb->used;
	}

	CONNIN_RUNLOCK;
	return return_value;
}

int OWNET_dirprocess(OWNET_HANDLE h,
					 const char *onewire_path, void (*dirfunc) (void *passed_on_value, const char *directory_element), void *passed_on_value)
{
	struct request_packet s_request_packet;
	struct request_packet *rp = &s_request_packet;
	memset(rp, 0, sizeof(struct request_packet));
	int return_value;

	CONNIN_RLOCK;
	rp->owserver = find_connection_in(h);
	if (rp->owserver == NULL) {
		CONNIN_RUNLOCK;
		return -EBADF;
	}

	rp->path = (onewire_path == NULL) ? "/" : onewire_path;
	return_value = ServerDir(dirfunc, passed_on_value, rp);

	CONNIN_RUNLOCK;
	return return_value;
}
