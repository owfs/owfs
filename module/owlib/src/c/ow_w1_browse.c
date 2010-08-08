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

	if ( pthread_create(&thread_dispatch, NULL, W1_Dispatch, NULL ) != 0 ) {
		ERROR_DEBUG("Couldn't create netlink monitoring thread");
		return gbBAD ;
	}

	return gbGOOD ;
}

#endif /* OW_W1 && OW_MT */
