/*
    OWFS -- One-Wire filesystem
    OWHTTPD -- One-Wire Web Server
    Written 2003 Paul H Alfille
    email: paul.alfille@gmail.com
    Released under the GPL
    See the header file: ow.h for full attribution
    1wire/iButton system from Dallas Semiconductor
*/

#include <config.h>
#include "owfs_config.h"
#include "ow.h"
#include "ow_connection.h"

/* Routines for handling a linked list of connections out */
/* typical connection out would be a network port for owserver or web access */

struct outbound_control Outbound_Control = {
	.active = 0,
	.next_index = 0,
	.head = NULL,
};

struct connection_out *NewOut(void)
{
	size_t len = sizeof(struct connection_out);
	struct connection_out *now = (struct connection_out *) owmalloc(len);
	if (now) {
		memset(now, 0, len);
		now->next = Outbound_Control.head;
		now->inet_type = inet_none ;
		Outbound_Control.head = now;
		now->index = Outbound_Control.next_index++;
		++Outbound_Control.active ;

		// Zero sref's -- done with struct memset
		//now->sref0 = 0 ;
		//now->sref1 = 0 ;
	} else {
		LEVEL_DEFAULT("Cannot allocate memory for server structure,");
	}
	return now;
}

void FreeOutAll(void)
{
	struct connection_out *next = Outbound_Control.head;
	struct connection_out *now;

	Outbound_Control.head = NULL;
	Outbound_Control.active = 0;
	while (next) {
		now = next;
		next = now->next;
		SAFEFREE(now->zero.name) ;
		SAFEFREE(now->zero.type) ;
		SAFEFREE(now->zero.domain) ;
		SAFEFREE(now->name) ;
		SAFEFREE(now->host) ;
		SAFEFREE(now->service) ;
		if (now->ai) {
			freeaddrinfo(now->ai);
			now->ai = NULL;
		}
		if ( FILE_DESCRIPTOR_VALID(now->file_descriptor) ) {
			shutdown(now->file_descriptor, SHUT_RDWR ) ;
			close(now->file_descriptor) ;
		}
#if OW_ZERO
		if (libdnssd != NULL) {
			if (now->sref0) {
				DNSServiceRefDeallocate(now->sref0);
			}
			if (now->sref1) {
				DNSServiceRefDeallocate(now->sref1);
			}
		}
#endif
		owfree(now);
	}
}
