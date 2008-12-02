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
#ifdef HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif

#if OW_HA7
/* Multicast to discover HA7 servers */
/* Wait 10 seconds for responses */
/* returns number found (>=0) or <0 on error */
/* Taken from Embedded Data Systems' documentation */
/* http://www.talk1wire.com/?q=node/7 */

#pragma pack(push,1)
struct HA7_response {
	char signature[2] ;
	uint16_t command ;
	uint16_t port ;
	uint16_t sslport ;
	char serial_num[12] ; // MAC
	char dev_name[64] ;
} ;
#pragma pack(pop)

static int Test_HA7_response( struct HA7_response * ha7_response )
{
	LEVEL_DEBUG("From ha7_response: signature=%.2s, command=%X, port=%d, ssl=%d, MAC=%.12s, device=%s\n",
	ha7_response->signature,
	ntohs(ha7_response->command),
	ntohs(ha7_response->port),
	ntohs(ha7_response->sslport),
	ha7_response->serial_num,
	ha7_response->dev_name) ;
	
	if (memcmp("HA", ha7_response->signature, 2)) {
		LEVEL_CONNECT("HA7 response signature error\n");
		return 1;
	}
	if ( 0x8001 != ntohs(ha7_response->command) ) {
		LEVEL_CONNECT("HA7 response command error\n");
		return 1;
	}
	return 0 ;
}

static void Setup_HA7_hint( struct addrinfo * hint )
{
	memset(hint, 0, sizeof(struct addrinfo));
#ifdef AI_NUMERICSERV
	hint->ai_flags = AI_CANONNAME | AI_NUMERICHOST | AI_NUMERICSERV;
#else
	hint->ai_flags = AI_CANONNAME | AI_NUMERICHOST;
#endif
	hint->ai_family = AF_INET;
	hint->ai_socktype = SOCK_DGRAM; // udp
	hint->ai_protocol = 0;
}

static int Get_HA7_response( struct addrinfo *now, char * name )
{
	struct timeval tv = { 50, 0 };
	int file_descriptor;
	struct HA7_response ha7_response ;

	struct sockaddr_in from ;
	socklen_t fromlen = sizeof(struct sockaddr_in) ;
	int on = 1;

	if ((file_descriptor = socket(now->ai_family, now->ai_socktype, now->ai_protocol)) < 0) {
		return 1;
	}
	if (setsockopt(file_descriptor, SOL_SOCKET, SO_BROADCAST, &on, sizeof(on)) == -1) {
		ERROR_DEBUG("Cannot set socket option for broadcast.\n");
		return 1;
	}
	if (sendto(file_descriptor, "HA\000\001", 4, 0, now->ai_addr, now->ai_addrlen)
		!= 4) {
		ERROR_CONNECT("Trouble sending broadcast message\n");
		return 1;
	}

	/* now read */
	if ( udp_read(file_descriptor, &ha7_response, sizeof(struct HA7_response), &tv, &from, &fromlen) != sizeof(struct HA7_response) ) {
		LEVEL_CONNECT("HA7 response bad length\n");
		return 1;
	}

	if ( Test_HA7_response( &ha7_response ) ) {
		return 1 ;
	}

	UCLIBCLOCK ;
	snprintf(name,INET_ADDRSTRLEN+20,"%s:%d",inet_ntoa(from.sin_addr),ntohs(ha7_response.port));
	UCLIBCUNLOCK ;
	
	return 0 ;
}

int FS_FindHA7(void)
{
	struct addrinfo *ai;
	struct addrinfo hint;
	struct addrinfo *now;
	int ret = -ENOENT;
	int n ;

	Setup_HA7_hint( &hint ) ;
	if ((n = getaddrinfo("224.1.2.3", "4567", &hint, &ai))) {
		LEVEL_CONNECT("Couldn't set up HA7 broadcast message %s\n", gai_strerror(n));
		return -ENOENT;
	}

	for (now = ai; now; now = now->ai_next) {
		ASCII name[INET_ADDRSTRLEN+20]; // tcp quad + port
		struct connection_in *in;

		if ( Get_HA7_response( now, name ) ) {
			continue ;
		}
		
		in = NewIn(NULL) ;
		if (in == NULL) {
			continue;
		}

		in->name = strdup(name);
		in->busmode = bus_ha7net;

		LEVEL_CONNECT("HA7Net adapter discovered at s\n",in->name);
		ret = 0 ; // at least one good HA7
	}
	freeaddrinfo(ai);
	return ret ;
}
#endif							/* OW_HA7 */
