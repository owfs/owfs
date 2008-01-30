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

#include "owshell.h"

void DefaultOwserver(void)
{
#if OW_ZERO
	if (count_inbound_connections == 0) {
//      OW_Browse() ;
	}
#endif							/* OW_ZERO */
	if (count_inbound_connections == 0) {
		OW_ArgNet(OWSERVER_DEFAULT_PORT);
	}
}

int ClientAddr(char *sname)
{
	struct addrinfo hint;
	char *p;
	int ret;

	if (sname == NULL)
		return -1;
	if ((p = strrchr(sname, ':'))) {	/* : exists */
		p[0] = '\0';			/* Separate tokens in the string */
		owserver_connection->host = strdup(sname);
		owserver_connection->service = strdup(&p[1]);
		p[0] = ':';				/* restore name string */
	} else {
#if OW_CYGWIN
		owserver_connection->host = strdup("127.0.0.1");
#else
		owserver_connection->host = NULL;
#endif
		owserver_connection->service = strdup(sname);
	}

	memset(&hint, 0, sizeof(struct addrinfo));
	hint.ai_socktype = SOCK_STREAM;
#if OW_CYGWIN
	hint.ai_family = AF_INET;
#else
	hint.ai_family = AF_UNSPEC;
#endif

//printf("ClientAddr: [%s] [%s]\n", owserver_connection->host, owserver_connection->service);

	if ((ret = getaddrinfo(owserver_connection->host, owserver_connection->service, &hint, &owserver_connection->ai))) {
		//LEVEL_CONNECT("GetAddrInfo error %s\n",gai_strerror(ret));
		return -1;
	}
	return 0;
}

/* Usually called with BUS locked, to protect ai settings */
int ClientConnect(void)
{
	int file_descriptor;
	struct addrinfo *ai;

	if (owserver_connection->ai == NULL) {
		//LEVEL_DEBUG("Client address not yet parsed\n");
		return -1;
	}

	/* Can't change ai_ok without locking the in-device.
	 * First try the last working address info, if it fails lock
	 * the in-device and loop through the list until it works.
	 * Not a perfect solution, but it should work at least.
	 */
	ai = owserver_connection->ai_ok;
	if (ai) {
		file_descriptor = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
		if (file_descriptor >= 0) {
			if (connect(file_descriptor, ai->ai_addr, ai->ai_addrlen) == 0)
				return file_descriptor;
			close(file_descriptor);
		}
	}

	ai = owserver_connection->ai;	// loop from first address info since it failed.
	do {
		file_descriptor = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
		if (file_descriptor >= 0) {
			if (connect(file_descriptor, ai->ai_addr, ai->ai_addrlen) == 0) {
				owserver_connection->ai_ok = ai;
				return file_descriptor;
			}
			close(file_descriptor);
		}
	} while ((ai = ai->ai_next));
	owserver_connection->ai_ok = NULL;

	//ERROR_CONNECT("ClientConnect: Socket problem\n") ;
	return -1;
}
