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

GOOD_OR_BAD ClientAddr(char *sname, char * default_port, struct connection_in *in)
{
	struct addrinfo hint;
	struct address_pair ap ;
	int ret;

	Parse_Address( sname, &ap ) ;

	switch ( ap.entries ) {
	case 0: // Complete default address
		SOC(in)->dev.tcp.host = NULL;
		SOC(in)->dev.tcp.service = owstrdup(default_port);
		break ;
	case 1: // single entry -- usually port unless a dotted quad
		switch ( ap.first.type ) {
		case address_none:
			SOC(in)->dev.tcp.host = NULL;
			SOC(in)->dev.tcp.service = owstrdup(default_port);
			break ;
		case address_dottedquad:
			// looks like an IP address
			SOC(in)->dev.tcp.host = owstrdup(ap.first.alpha);
			SOC(in)->dev.tcp.service = owstrdup(default_port);
			break ;
		case address_numeric:
			SOC(in)->dev.tcp.host = NULL;
			SOC(in)->dev.tcp.service = owstrdup(ap.first.alpha);
			break ;
		default:
			// assume it's a port if it's the SERVER
			if ( strcasecmp( default_port, DEFAULT_SERVER_PORT ) == 0 ) {
				SOC(in)->dev.tcp.host = NULL;
				SOC(in)->dev.tcp.service = owstrdup(ap.first.alpha);
			} else {
				SOC(in)->dev.tcp.host = owstrdup(ap.first.alpha);
				SOC(in)->dev.tcp.service = owstrdup(default_port);
			}
			break ;
		}
		break ;
	case 2:
	default: // address:port format -- unambiguous
		SOC(in)->dev.tcp.host = ( ap.first.type == address_none ) ? NULL : owstrdup(ap.first.alpha) ;
		SOC(in)->dev.tcp.service = ( ap.second.type == address_none ) ? owstrdup(default_port) : owstrdup(ap.second.alpha) ;
		break ;
	}
	Free_Address( &ap ) ;

	memset(&hint, 0, sizeof(struct addrinfo));
	hint.ai_socktype = SOCK_STREAM;

#if OW_CYGWIN
	hint.ai_family = AF_INET;
	if( SOC(in)->dev.tcp.host == NULL) {
		/* getaddrinfo doesn't work with host=NULL for cygwin */
		SOC(in)->dev.tcp.host = owstrdup("127.0.0.1");
	}
#else
	hint.ai_family = AF_UNSPEC;
#endif

	LEVEL_DEBUG("IP address=[%s] port=[%s]", SAFESTRING( SOC(in)->dev.tcp.host), SOC(in)->dev.tcp.service);
	ret = getaddrinfo( SOC(in)->dev.tcp.host, SOC(in)->dev.tcp.service, &hint, &( SOC(in)->dev.tcp.ai) ) ;
	if ( ret != 0 ) {
		LEVEL_CONNECT("GETADDRINFO error %s", gai_strerror(ret));
		return gbBAD;
	}
	return gbGOOD;
}

void FreeClientAddr(struct connection_in *in)
{
	SAFEFREE( SOC(in)->dev.tcp.host) ;
	SAFEFREE( SOC(in)->dev.tcp.service) ;
	if ( SOC(in)->dev.tcp.ai != NULL ) {
		freeaddrinfo( SOC(in)->dev.tcp.ai);
		SOC(in)->dev.tcp.ai = NULL;
	}
	SOC(in)->dev.tcp.ai_ok = NULL;
}

/* Usually called with BUS locked, to protect ai settings */
FILE_DESCRIPTOR_OR_ERROR ClientConnect(struct connection_in *in)
{
	FILE_DESCRIPTOR_OR_ERROR file_descriptor;
	struct addrinfo *ai;

	if ( SOC(in)->dev.tcp.ai == NULL) {
		LEVEL_DEBUG("Client address not yet parsed");
		return FILE_DESCRIPTOR_BAD;
	}

	/* Can't change ai_ok without locking the in-device.
	 * First try the last working address info, if it fails lock
	 * the in-device and loop through the list until it works.
	 * Not a perfect solution, but it should work at least.
	 */
	ai = SOC(in)->dev.tcp.ai_ok;
	if (ai) {
		file_descriptor = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
		if ( FILE_DESCRIPTOR_VALID(file_descriptor) ) {
			fcntl (file_descriptor, F_SETFD, FD_CLOEXEC); // for safe forking
			if (connect(file_descriptor, ai->ai_addr, ai->ai_addrlen) == 0) {
				return file_descriptor;
			}
			close(file_descriptor);
		}
	}

	ai = SOC(in)->dev.tcp.ai;		// loop from first address info since it failed.
	do {
		file_descriptor = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
		if ( FILE_DESCRIPTOR_VALID(file_descriptor) ) {
			fcntl (file_descriptor, F_SETFD, FD_CLOEXEC); // for safe forking
			if (connect(file_descriptor, ai->ai_addr, ai->ai_addrlen) == 0) {
				SOC(in)->dev.tcp.ai_ok = ai;
				return file_descriptor;
			}
			close(file_descriptor);
		}
	} while ((ai = ai->ai_next));
	SOC(in)->dev.tcp.ai_ok = NULL;

	ERROR_CONNECT("Socket problem");
	STAT_ADD1(NET_connection_errors);
	return FILE_DESCRIPTOR_BAD;
}
