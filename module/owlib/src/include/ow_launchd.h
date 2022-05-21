/*
    OWFS -- One-Wire filesystem
    OWHTTPD -- One-Wire Web Server
    Written 2003 Paul H Alfille
	email: paul.alfille@gmail.com
	Released under the GPL
	See the header file: ow.h for full attribution
	1wire/iButton system from Dallas Semiconductor
*/

#ifndef OW_LAUNCHD_H
#define OW_LAUNCHD_H

#ifndef OWFS_CONFIG_H
#error Please make sure owfs_config.h is included *before* this header file
#endif


#ifdef HAVE_LAUNCH_ACTIVATE_SOCKET

// Header file from OSX
#include	<launch.h>

#endif /* HAVE_LAUNCH_ACTIVATE_SOCKET */

// Our function -- safe outside of OSX too
void Setup_Launchd( void ) ;

#endif							/* OW_LAUNCHD_H */
