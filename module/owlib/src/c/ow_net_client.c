/*
    OWFS -- One-Wire filesystem
    OWHTTPD -- One-Wire Web Server
    Written 2003 Paul H Alfille
    email: paul.alfille@gmail.com
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
	struct port_in * pin = in->pown ;
	struct addrinfo hint;
	struct address_pair ap ;
	int ret;

LEVEL_DEBUG("Called with %s default=%s",SAFESTRING(sname),SAFESTRING(default_port));
	Parse_Address( sname, &ap ) ;

	switch ( ap.entries ) {
	case 0: // Complete default address
		pin->dev.tcp.host = NULL;
		pin->dev.tcp.service = owstrdup(default_port);
		break ;
	case 1: // single entry -- usually port unless a dotted quad
		switch ( ap.first.type ) {
		case address_none:
			pin->dev.tcp.host = NULL;
			pin->dev.tcp.service = owstrdup(default_port);
			break ;
		case address_dottedquad:
			// looks like an IP address
			pin->dev.tcp.host = owstrdup(ap.first.alpha);
			pin->dev.tcp.service = owstrdup(default_port);
			break ;
		case address_numeric:
			pin->dev.tcp.host = NULL;
			pin->dev.tcp.service = owstrdup(ap.first.alpha);
			break ;
		default:
			// assume it's a port if it's the SERVER
			if ( strcasecmp( default_port, DEFAULT_SERVER_PORT ) == 0 ) {
				pin->dev.tcp.host = NULL;
				pin->dev.tcp.service = owstrdup(ap.first.alpha);
			} else {
				pin->dev.tcp.host = owstrdup(ap.first.alpha);
				pin->dev.tcp.service = owstrdup(default_port);
			}
			break ;
		}
		break ;
	case 2:
	default: // address:port format -- unambiguous
		pin->dev.tcp.host = ( ap.first.type == address_none ) ? NULL : owstrdup(ap.first.alpha) ;
		pin->dev.tcp.service = ( ap.second.type == address_none ) ? owstrdup(default_port) : owstrdup(ap.second.alpha) ;
		break ;
	}
	Free_Address( &ap ) ;

	memset(&hint, 0, sizeof(struct addrinfo));
//    hint.ai_socktype = SOCK_STREAM | SOCK_CLOEXEC ;
    hint.ai_socktype = SOCK_STREAM ;

#if OW_CYGWIN
	hint.ai_family = AF_INET;
	if( pin->dev.tcp.host == NULL) {
		/* getaddrinfo doesn't work with host=NULL for cygwin */
		pin->dev.tcp.host = owstrdup("127.0.0.1");
	}
#else
	hint.ai_family = AF_UNSPEC;
#endif

	LEVEL_DEBUG("Called with [%s] IP address=[%s] port=[%s]", SAFESTRING(sname),SAFESTRING( pin->dev.tcp.host), SAFESTRING(pin->dev.tcp.service) );
	ret = getaddrinfo( pin->dev.tcp.host, pin->dev.tcp.service, &hint, &( pin->dev.tcp.ai) ) ;
	if ( ret != 0 ) {
		LEVEL_CONNECT("GETADDRINFO error %s", gai_strerror(ret));
		return gbBAD;
	}
	return gbGOOD;
}

void FreeClientAddr(struct connection_in *in)
{
	struct port_in * pin = in->pown ;
	SAFEFREE( pin->dev.tcp.host) ;
	SAFEFREE( pin->dev.tcp.service) ;
	if ( pin->dev.tcp.ai != NULL ) {
		freeaddrinfo( pin->dev.tcp.ai);
		pin->dev.tcp.ai = NULL;
	}
	pin->dev.tcp.ai_ok = NULL;
}

/* Usually called with BUS locked, to protect ai settings */
FILE_DESCRIPTOR_OR_ERROR ClientConnect(struct connection_in *in)
{
	struct port_in * pin = in->pown ;
	FILE_DESCRIPTOR_OR_ERROR file_descriptor;
	struct addrinfo *ai;

	if ( pin->dev.tcp.ai == NULL) {
		LEVEL_DEBUG("Client address not yet parsed");
		return FILE_DESCRIPTOR_BAD;
	}

	/* Can't change ai_ok without locking the in-device.
	 * First try the last working address info, if it fails lock
	 * the in-device and loop through the list until it works.
	 * Not a perfect solution, but it should work at least.
	 */
	ai = pin->dev.tcp.ai_ok;
	if (ai) {
		file_descriptor = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
		if ( FILE_DESCRIPTOR_VALID(file_descriptor) ) {
			if (connect(file_descriptor, ai->ai_addr, ai->ai_addrlen) == 0) {
				return file_descriptor;
			}
			close(file_descriptor);
		}
	}

	ai = pin->dev.tcp.ai;		// loop from first address info since it failed.
	do {
		file_descriptor = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
		if ( FILE_DESCRIPTOR_VALID(file_descriptor) ) {
			if (connect(file_descriptor, ai->ai_addr, ai->ai_addrlen) == 0) {
				pin->dev.tcp.ai_ok = ai;
				return file_descriptor;
			}
			close(file_descriptor);
		}
	} while ((ai = ai->ai_next));
	pin->dev.tcp.ai_ok = NULL;

	ERROR_CONNECT("Socket problem");
	STAT_ADD1(NET_connection_errors);
	return FILE_DESCRIPTOR_BAD;
}
