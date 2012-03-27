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

#include <config.h>
#include "owfs_config.h"
#include "ow.h"

/* For thread ID to aid exitting */
int main_threadid_init = 0 ;
pthread_t main_threadid;

/* All ow library setup */
void LibSetup(enum enum_program_type program_type)
{
	Return_code_setup() ;
	
	/* Setup the multithreading synchronizing locks */
	LockSetup();

	Globals.program_type = program_type;

	/* special resort in case static data (devices and filetypes) not properly sorted */
	DeviceSort();

#if OW_CACHE
	Cache_Open();
#endif							/* OW_CACHE */

	StateInfo.start_time = NOW_TIME;
	SetLocalControlFlags() ; // reset by every option and other change.
	errno = 0;					/* set error level none */

}
