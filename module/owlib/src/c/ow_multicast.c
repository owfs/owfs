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
#include <arpa/inet.h>

#if OW_HA7
/* Multicast to discover HA7 servers */
/* Wait 10 seconds for responses */
/* returns number found (>=0) or <0 on error */
# define HA7_response_len 84
int FS_FindHA7(void)
{
	struct addrinfo *ai;
	struct addrinfo hint;
	struct addrinfo *now;
	int ret = -1;
	int n;

	memset(&hint, 0, sizeof(struct addrinfo));
	hint.ai_flags = AI_CANONNAME;
	hint.ai_family = AF_INET;
	hint.ai_socktype = SOCK_DGRAM;
	hint.ai_protocol = 0;
	if ((n = getaddrinfo("224.1.2.3", "4567", &hint, &ai))) {
		LEVEL_CONNECT("Couldn't set up HA7 broadcast message %s\n",
					  gai_strerror(n));
		return -ENOENT;
	}

	for (now = ai; now; now = now->ai_next) {
		BYTE buffer[HA7_response_len];
		struct timeval tv = { 5, 0 };
		int file_descriptor;
		struct sockaddr_in from;
		ASCII name[64];
		struct connection_in *in;
		printf("Finding...\n");
		if ((file_descriptor =
			 socket(now->ai_family, now->ai_socktype,
					now->ai_protocol)) < 0)
			continue;
		printf("Finding file_descriptor=%d\n", file_descriptor);
		if (sendto(file_descriptor, "HA\000\001", 4, 0, now->ai_addr, now->ai_addrlen)
			!= 4) {
			ERROR_CONNECT("Trouble sending broadcast message\n");
			continue;
		}
		printf("sendto ok\n");
		if (ret < 0)
			ret = 0;			// possible read -- no default error

		/* now read */

		if ((n =
			 tcp_read(file_descriptor, buffer, HA7_response_len,
					  &tv)) != HA7_response_len) {
			LEVEL_CONNECT("HA7 response bad length\n");
			printf("HA7 response bad length %d\n", n);
			continue;
		}
		printf("Read in the corrent length\n");
		if (memcmp("HA\x80\x01", buffer, 4)) {
			LEVEL_CONNECT("HA7 response content error\n");
			continue;
		}
		printf("Read in with correct start chars\n");
		if ((in = NewIn(NULL)) == NULL)
			break;
		printf("Success!\n");
		++ret;
		inet_ntop(AF_INET, &(from.sin_addr), name, 64);
		snprintf(&name[64 - strlen(name)], 64 - strlen(name), ":%d",
				 (buffer[2] << 8) + buffer[3]);
		in->name = strdup(name);
		in->busmode = bus_ha7net;
	}
	freeaddrinfo(ai);
	return ret;
}
#endif							/* OW_HA7 */
