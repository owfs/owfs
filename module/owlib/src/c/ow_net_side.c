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

void SideAddr(struct connection_side *side)
{
	struct addrinfo hint;
	char *p;
	int ret;

	if (side->name == NULL || side->name[0] == '\0') {
		side->name = owstrdup("4305");	// arbitrary
	}
	if ((p = strrchr(side->name, ':'))) {	/* : exists */
		p[0] = '\0';			/* Separate tokens in the string */
		side->host = owstrdup(side->name);
		side->service = owstrdup(&p[1]);
		p[0] = ':';				/* restore name string */
	} else {
#if OW_CYGWIN
		side->host = owstrdup("127.0.0.1");
#else
		side->host = NULL;
#endif
		side->service = owstrdup(side->name);
	}

	memset(&hint, 0, sizeof(struct addrinfo));
	hint.ai_socktype = SOCK_STREAM;
#if OW_CYGWIN
	hint.ai_family = AF_INET;
#else
	hint.ai_family = AF_UNSPEC;
#endif

//printf("ClientAddr: [%s] [%s]\n", in->connin.tcp.host, in->connin.tcp.service);

	if ((ret = getaddrinfo(side->host, side->service, &hint, &side->ai))) {
		LEVEL_CONNECT("GetAddrInfo error %s\n", gai_strerror(ret));
		side->good_entry = 0;
	} else {
		side->good_entry = 1;
	}
}

void FreeSideAddr(struct connection_side *side)
{
	if (side->host) {
		owfree(side->host);
		side->host = NULL;
	}
	if (side->service) {
		owfree(side->service);
		side->service = NULL;
	}
	if (side->ai) {
		freeaddrinfo(side->ai);
		side->ai = NULL;
	}
}

/* Usually called with BUS locked, to protect ai settings */
int SideConnect(struct connection_side *side)
{
	int file_descriptor;
	struct addrinfo *ai;

	if (side->ai == NULL) {
		LEVEL_DEBUG("Sidetap address not yet parsed\n");
		return -1;
	}

	/* Can't change ai_ok without locking the in-device.
	 * First try the last working address info, if it fails lock
	 * the in-device and loop through the list until it works.
	 * Not a perfect solution, but it should work at least.
	 */
	ai = side->ai_ok;
	if (ai) {
		file_descriptor = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
		if (file_descriptor >= 0) {
			if (connect(file_descriptor, ai->ai_addr, ai->ai_addrlen) == 0) {
				return file_descriptor;
			}
			close(file_descriptor);
		}
	}

	ai = side->ai;				// loop from first address info since it failed.
	do {
		file_descriptor = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
		if (file_descriptor >= 0) {
			if (connect(file_descriptor, ai->ai_addr, ai->ai_addrlen) == 0) {
				side->ai_ok = ai;
				return file_descriptor;
			}
			close(file_descriptor);
		}
	} while ((ai = ai->ai_next));
	side->ai_ok = NULL;

	ERROR_CONNECT("SideConnect: Socket problem\n");
	STAT_ADD1(NET_connection_errors);
	return -1;
}
