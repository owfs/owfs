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
 * using kevent
 * used in OSX and BSD
 * Linux uses inotify instead
 * */
#ifdef WE_HAVE_KEVENT

static int config_monitor_num_files = 0 ; // keep track
static int kq ; // kqueue

void Config_Monitor_Add( const char * filename )
{
	FILE_DESCRIPTOR_OR_ERROR fd ;
	struct kevent ke ;
	if ( config_monitor_num_files == 0 ) {
		// first one
		kq = kqueue() ;
		if ( kq < 0 ) {
			LEVEL_DEBUG("Could not create a kevent queue (kqueue)" ) ;
			return ;
		}
	}
	fd = open( filename, O_EVTONLY ) ;
	if ( FILE_DESCRIPTOR_NOT_VALID( fd ) ) {
		LEVEL_DEBUG("Can't open %s for monitoring", filename ) ;
		return ;
	}
	EV_SET( &ke, fd, EVFILT_VNODE, EV_ADD, NOTE_DELETE|NOTE_WRITE|NOTE_EXTEND|NOTE_RENAME, 0, NULL ) ;
	if ( kevent( kq, &ke, 1, NULL, 0, NULL ) != 0 ) {
		LEVEL_DEBUG("Couldn't add %s to kqueue for monitoring",filename ) ;
	} else {
		++ config_monitor_num_files ;
		LEVEL_DEBUG("Added %s to kqueue", filename ) ;
	}
}

static void Config_Monitor_Block( void )
{
	// OS specific code
	struct kevent ke ;
	// kevent should block until an event
	while( kevent( kq, NULL, 0, &ke, 1, NULL ) < 1 ) {
		LEVEL_DEBUG("kevent loop (shouldn't happen!)" ) ;
	}
	// fall though -- event happened, time to resurrect
	LEVEL_DEBUG("Configuration file change -- time to resurrect");
}

// Thread that waits for forfig change and then restarts the program
static void * Config_Monitor_Watchthread( void * v)
{
	DETACH_THREAD ;
	// Blocking call until a config change detected
	Config_Monitor_Block() ;
	LEVEL_DEBUG("Configuration file change detected. Will restart %s",Globals.argv[0]);
	// Restart the program
	ReExecute(v) ;
	return v ;
}

static void Config_Monitor_Makethread( void * v )
{
	pthread_t thread ;
	if ( pthread_create( &thread, DEFAULT_THREAD_ATTR, Config_Monitor_Watchthread, v ) != 0 ) {
		LEVEL_DEBUG( "Could not create Configuration monitoring thread" ) ;
	}
}

void Config_Monitor_Watch( void * v)
{
	if ( config_monitor_num_files > 0 ) {
		Config_Monitor_Makethread() ;
	} else {
		LEVEL_DEBUG("No configuration files to monitor" ) ;
	}
}

#endif /* WE_HAVE_KEVENT */
