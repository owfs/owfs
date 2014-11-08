/*
	OWFS -- One-Wire filesystem
	OWHTTPD -- One-Wire Web Server
	Written 2003 Paul H Alfille
	email: paul.alfille@gmail.com
	Released under the GPL
	See the header file: ow.h for full attribution
	1wire/iButton system from Dallas Semiconductor
*/

#ifndef OW_AVAHI_H
#define OW_AVAHI_H

#if OW_AVAHI

#include <avahi-client/client.h>
#include <avahi-client/publish.h>
#include <avahi-client/lookup.h>

#include <avahi-common/thread-watch.h>
#include <avahi-common/error.h>

struct connection_in ;
struct connection_out ;

/**********************************************/
/* Prototypes */
GOOD_OR_BAD OW_Avahi_Announce( struct connection_out *out ) ;
GOOD_OR_BAD OW_Avahi_Browse(struct connection_in * in) ;

#endif /* OW_AVAHI */

#endif 	/* OW_AVAHI_H */
