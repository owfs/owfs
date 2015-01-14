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
#include "sd-daemon.h"

/* Test systemd existence
 * Sets up the connection_out as well
 * */

void Setup_Systemd( void )
{
	int fds = sd_listen_fds(0) ;
	int fd_count = 0 ;
	int i ;

	for ( i = 0 ; i < fds ; ++i ) {
		struct connection_out *out = NewOut();
		if (out == NULL) {
			break ;
		}
		out->file_descriptor = i + SD_LISTEN_FDS_START ;
		++ fd_count ;
		out->name = owstrdup("systemd");
		out->inet_type = inet_systemd ;
	}
	if ( fd_count > 0 ) {
		Globals.daemon_status = e_daemon_sd ;
	}
}

// Announce systemd is ready
void Announce_Systemd( void )
{
	if ( Globals.daemon_status == e_daemon_sd ) {
		sd_notify( 0,"READY=1" ) ;
	}
}
