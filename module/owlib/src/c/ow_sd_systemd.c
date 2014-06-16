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
#include "sd-demon.h"

/* Test systemd existence
 * Sets up the connection_out as well
 * */

void Setup_Systemd( void )
{
	Globals.systemd_fds = sd_listen_fds(0) ;
	if ( Globals.systemd_fds > 0 ) {
		Globals.daemon_status = e_daemon_sd ;
	}
}
