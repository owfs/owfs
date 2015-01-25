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

/* Code for monitoring the configuration files
 * using inotify
 * Linux uses this
 * kevent used in OSX and BSD
 * */
#ifdef WE_HAVE_INOTIFY

static int config_monitor_num_files = 0 ;

void Config_Monitor_Add( char * filename )
{
}

static void Config_Monitor_Block( void )
{
	// OS specific code
}

// Thread that waits for forfig change and then restarts the program
static void * Config_Monitor_Watchthread( void * v)
{
	(void) v ;
	DETATCH_THREAD ;
	// Blocking call unil a config change detected
	Config_Monitor_block() ;
	LEVEL_DEBUG("Configuration file change detected. Will restart %s",Globals.argv[0]);
	// Restart the program
	ReExecute() ;
}

static void Config_Monitor_Makethread( void )
{
	pthread_t thread ;
	if ( pthread_create( &thread, DEFAULT_THREAD_ATTR, Config_Monitor_Watchthread, NULL ) != 0 ) {
		LEVEL_DEBUG( "Could not create Configuration monitoring thread" ) ;
	}
}

void Config_Monitor_Watch( void )
{
	if ( config_monitor_num_files > 0 ) {
		Config_Monitor_Makethread() ;
	} else {
		LEVEL_DEBUG("No configuration files to monitor" ) ;
	}
}

#endif /* WE_HAVE_INOTIFY */
