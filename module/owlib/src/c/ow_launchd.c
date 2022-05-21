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
#include "ow_launchd.h"

/* Test launchd existence
 * Sets up the connection_out as well
 * */

#ifdef HAVE_LAUNCH_ACTIVATE_SOCKET
static void Launchd_out( int * fds, size_t fd_count )
{
	size_t i ;
	for ( i=0 ; i < fd_count ; ++ i ) {
		struct connection_out *out = NewOut();
		if (out == NULL) {
			LEVEL_DEBUG("Memory error -- can't create a launchd structure");
			break ;
		}
		out->file_descriptor = fds[i] ;
		LEVEL_DEBUG("Found a launchd file descriptor at %d",out->file_descriptor);
		out->name = owstrdup("launchd");
		out->inet_type = inet_launchd ;
	}
}
#endif /* HAVE_LAUNCH_ACTIVATE_SOCKET */

void Setup_Launchd( void )
{
#ifdef HAVE_LAUNCH_ACTIVATE_SOCKET
	int * fds ;
	size_t fd_count ;

	switch ( launch_activate_socket( "Listeners", & fds , & fd_count ) ) {
		case 0:
			Launchd_out( fds, fd_count ) ;
			Globals.inet_type = inet_launchd ;
			free( fds ) ;
			break ;
		case ESRCH:
			LEVEL_DEBUG("Not started by the launchd daemon -- use command line for ports");
			break ;
		case ENOENT:
			LEVEL_CALL("No sockets specified in the launchd plist");
			break ;
		default:
			LEVEL_DEBUG("Launchd error");
			break ;
	}
#endif /* HAVE_LAUNCH_ACTIVATE_SOCKET */
}

