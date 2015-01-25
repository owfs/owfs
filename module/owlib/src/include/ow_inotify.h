/*
    OWFS -- One-Wire filesystem
    OWHTTPD -- One-Wire Web Server
    Written 2003 Paul H Alfille
	email: paul.alfille@gmail.com
	Released under the GPL
	See the header file: ow.h for full attribution
	1wire/iButton system from Dallas Semiconductor
*/

#ifndef OW_INOTIFY_H
#define OW_INOTIFY_H

#ifndef OWFS_CONFIG_H
#error Please make sure owfs_config.h is included *before* this header file
#endif

#ifdef WE_HAVE_INOTIFY
// kevent based monitor for configuration file changes
// usually linux systems

#include <sys/inotify.h>
void Config_Monitor_Add( const char * file ) ;
void Config_Monitor_Watch( void ) ;

#endif /* WE_HAVE_INOTIFY */

#endif							/* OW_INOTIFY_H */
