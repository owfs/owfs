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
	
    if (sname == NULL || sname[0] == '\0') {
		/* probably not a good idea to set localhost:DEFAULT_PORT
		 * The user have probably typed wrong address */
		in->connin.tcp.host = NULL;
		in->connin.tcp.service = owstrdup(DEFAULT_PORT);
	}
	if ((p = strrchr(sname, ':')) == NULL) {    /* : NOT exist */
		if (strchr(sname, '.')) {   //probably an address
			in->connin.tcp.host = owstrdup(sname);
			in->connin.tcp.service = owstrdup(DEFAULT_PORT);
		} else {                // assume a port
			in->connin.tcp.host = NULL;
			in->connin.tcp.service = owstrdup(sname);
		}
	} else {
		p[0] = '\0';            /* Separate tokens in the string */
		in->connin.tcp.host = owstrdup(sname);
		p[0] = ':';             /* restore name string */
		in->connin.tcp.service = owstrdup(&p[1]);
	}

	memset(&hint, 0, sizeof(struct addrinfo));
	hint.ai_socktype = SOCK_STREAM;
#if OW_CYGWIN
	hint.ai_family = AF_INET;
	if(in->connin.tcp.host == NULL) {
		/* getaddrinfo doesn't work with host=NULL for cygwin */
		in->connin.tcp.host = owstrdup("127.0.0.1");
	}
#else
	hint.ai_family = AF_UNSPEC;
#endif

	LEVEL_DEBUG("[%s] [%s]", SAFESTRING(in->connin.tcp.host), in->connin.tcp.service);

	if ((ret = getaddrinfo(in->connin.tcp.host, in->connin.tcp.service, &hint, &in->connin.tcp.ai))) {
		LEVEL_CONNECT("error %s", gai_strerror(ret));
		return -1;
	}
	return 0;
}

void FreeClientAddr(struct connection_in *in)
{
	if (in->connin.tcp.host) {
		owfree(in->connin.tcp.host);
		in->connin.tcp.host = NULL;
	}
	if (in->connin.tcp.service) {
		owfree(in->connin.tcp.service);
		in->connin.tcp.service = NULL;
	}
	if (in->connin.tcp.ai) {
		freeaddrinfo(in->connin.tcp.ai);
		in->connin.tcp.ai = NULL;
	}
}

/* Usually called with BUS locked, to protect ai settings */
int ClientConnect(struct connection_in *in)
{
	FILE_DESCRIPTOR_OR_ERROR file_descriptor;
	struct addrinfo *ai;

	if (in->connin.tcp.ai == NULL) {
		LEVEL_DEBUG("Client address not yet parsed");
		return -1;
	}

	/* Can't change ai_ok without locking the in-device.
	 * First try the last working address info, if it fails lock
	 * the in-device and loop through the list until it works.
	 * Not a perfect solution, but it should work at least.
	 */
	ai = in->connin.tcp.ai_ok;
	if (ai) {
		file_descriptor = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
		if ( FILE_DESCRIPTOR_VALID(file_descriptor) ) {
			if (connect(file_descriptor, ai->ai_addr, ai->ai_addrlen) == 0) {
				return file_descriptor;
			}
			close(file_descriptor);
		}
	}

	ai = in->connin.tcp.ai;		// loop from first address info since it failed.
	do {
		file_descriptor = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
		if ( FILE_DESCRIPTOR_VALID(file_descriptor) ) {
			if (connect(file_descriptor, ai->ai_addr, ai->ai_addrlen) == 0) {
				in->connin.tcp.ai_ok = ai;
				return file_descriptor;
			}
			close(file_descriptor);
		}
	} while ((ai = ai->ai_next));
	in->connin.tcp.ai_ok = NULL;

	ERROR_CONNECT("Socket problem");
	STAT_ADD1(NET_connection_errors);
	return -1;
}
