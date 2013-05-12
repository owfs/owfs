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
#include "ow_connection.h"

/* Just close in/out devices and clear cache. Just enough to make it possible
   to call LibStart() again. This is called from swig/ow.i to when script
   wants to initialize a new server-connection. */
void LibStop(void)
{
	char *argv[1] = { NULL };
	LEVEL_CALL("Clear Cache");
	Cache_Clear();
	LEVEL_CALL("Closing input devices");
	FreeInAll();
	LEVEL_CALL("Closing output devices");
	FreeOutAll();

	/* Have to reset more internal variables, and this should be fixed
	 * by setting optind = 0 and call getopt()
	 * (first_nonopt = last_nonopt = 1;)
	 */
	optind = 0;
	(void) getopt_long(1, argv, " ", NULL, NULL);

	optarg = NULL;
	optind = 1;
	opterr = 1;
	optopt = '?';
}
