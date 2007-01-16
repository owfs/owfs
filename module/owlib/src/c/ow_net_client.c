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

/* ow_net holds the network utility routines. Many stolen unashamedly from Steven's Book */
/* Much modification by Christian Magnusson especially for Valgrind and embedded */
/* non-threaded fixes by Jerry Scharf */

#include <config.h>
#include "owfs_config.h"
#include "ow.h"
#include "ow_counters.h"
#include "ow_connection.h"

int ClientAddr(char *sname, struct connection_in *in)
{
	struct addrinfo hint;
	char *p;
	int ret;

	if (sname == NULL)
		return -1;
	if ((p = strrchr(sname, ':'))) {	/* : exists */
		p[0] = '\0';			/* Separate tokens in the string */
		in->connin.server.host = strdup(sname);
		in->connin.server.service = strdup(&p[1]);
		p[0] = ':';				/* restore name string */
	} else {
#if OW_CYGWIN
		in->connin.server.host = strdup("127.0.0.1");
#else
		in->connin.server.host = NULL;
#endif
		in->connin.server.service = strdup(sname);
	}

	memset(&hint, 0, sizeof(struct addrinfo));
	hint.ai_socktype = SOCK_STREAM;
#if OW_CYGWIN
	hint.ai_family = AF_INET;
#else
	hint.ai_family = AF_UNSPEC;
#endif

//printf("ClientAddr: [%s] [%s]\n", in->connin.server.host, in->connin.server.service);

	if ((ret =
		 getaddrinfo(in->connin.server.host, in->connin.server.service,
					 &hint, &in->connin.server.ai))) {
		LEVEL_CONNECT("GetAddrInfo error %s\n", gai_strerror(ret));
		return -1;
	}
	return 0;
}

void FreeClientAddr(struct connection_in *in)
{
	if (in->connin.server.host) {
		free(in->connin.server.host);
		in->connin.server.host = NULL;
	}
	if (in->connin.server.service) {
		free(in->connin.server.service);
		in->connin.server.service = NULL;
	}
	if (in->connin.server.ai) {
		freeaddrinfo(in->connin.server.ai);
		in->connin.server.ai = NULL;
	}
}

/* Usually called with BUS locked, to protect ai settings */
int ClientConnect(struct connection_in *in)
{
	int fd;
	struct addrinfo *ai;

	if (in->connin.server.ai == NULL) {
		LEVEL_DEBUG("Client address not yet parsed\n");
		return -1;
	}

	/* Can't change ai_ok without locking the in-device.
	 * First try the last working address info, if it fails lock
	 * the in-device and loop through the list until it works.
	 * Not a perfect solution, but it should work at least.
	 */
	ai = in->connin.server.ai_ok;
	if (ai) {
		fd = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
		if (fd >= 0) {
			if (connect(fd, ai->ai_addr, ai->ai_addrlen) == 0)
				return fd;
			close(fd);
		}
	}

	ai = in->connin.server.ai;	// loop from first address info since it failed.
	do {
		fd = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
		if (fd >= 0) {
			if (connect(fd, ai->ai_addr, ai->ai_addrlen) == 0) {
				in->connin.server.ai_ok = ai;
				return fd;
			}
			close(fd);
		}
	} while ((ai = ai->ai_next));
	in->connin.server.ai_ok = NULL;

	ERROR_CONNECT("ClientConnect: Socket problem\n");
	STAT_ADD1(NET_connection_errors);
	return -1;
}

