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

/* w1 browse for existing and new bus masters
 * w1 is different: it is a dynamic list of adapters
 * the scanning starts with "W1_Browse" in LibStart and continues in it's own thread 
 */

#include <config.h>
#include "owfs_config.h"
#include "ow_connection.h"

#if OW_W1 && OW_MT

#include "ow_w1.h"

GOOD_OR_BAD W1_Browse( void )
{
	pthread_t thread_dispatch ;

	++Inbound_Control.w1_entry_mark ;
	LEVEL_DEBUG("Calling for netlink w1 list");

	// Initial setup
	_MUTEX_INIT(Inbound_Control.w1_mutex);
	_MUTEX_INIT(Inbound_Control.w1_read_mutex);
	gettimeofday(&Inbound_Control.w1_last_read,NULL);
	++Inbound_Control.w1_last_read.tv_sec ;

	if ( FILE_DESCRIPTOR_NOT_VALID(w1_bind()) ) {
		ERROR_DEBUG("Netlink problem -- are you root?");
		return gbBAD ;
	}

	if ( pthread_create(&thread_dispatch, NULL, W1_Dispatch, NULL) != 0 ) {
		ERROR_DEBUG("Couldn't create netlink monitoring thread");
		return gbBAD ;
	}

	return gbGOOD ;
}

#endif /* OW_W1 && OW_MT */
