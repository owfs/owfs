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

/* Utility functions to copy arguements and restart the program */

/* Save the original command line args */
void ArgCopy( int argc, char * argv[] )
{
	Globals.argc = 0 ; // default bad value
	if ( argc > 0 ) {
		Globals.argv = owcalloc( argc+1, sizeof( char * ) ) ;
		if ( Globals.argv != NULL ) {
			int i ;
			Globals.argc = argc ;
			for ( i=0 ; i<argc ; ++i ) {
				Globals.argv[i] = owstrdup( argv[i] ) ;
			}
			Globals.argv[argc] = NULL ;
			Globals.argc = argc ;
		}
	} else {
		Globals.argv = owcalloc( 2, sizeof( char * ) ) ;
		if ( Globals.argv != NULL ) {
			Globals.argv[0] = owstrdup("Unknown_program") ;
			Globals.argv[1] = NULL ;
			Globals.argc = 1 ;
		}
	}
}

/* clean up command line args -- normal exit */
void ArgFree( void )
{
	if ( Globals.argc > 0 ) {
		int i ;
		for ( i=0 ; i<Globals.argc ; ++i ) {
			owfree( Globals.argv[i] ) ;
		}
		owfree( Globals.argv ) ;
		Globals.argv = NULL ;
		Globals.argc = 0 ;
	}
}

// Stop the loop that accepts connections
// and close sockets
static void StopProgram( void * v )
{
	/* Clean up everything */
	Globals.exitmode = exit_exec ;
	switch (Globals.program_type) {
		case program_type_ftpd:
			// owftpd uses it's own loop and stop
			// data and function pointer passed in argument
			{
				struct re_exec * sre = v ;
				(sre->func)(sre->data) ;
			}
			break ;
		default:
			// All the other routines use the standard loop
			// and standard stopper
			InterruptListening() ;
			break ;
	}
	
	// Close sockets
	FreeOutAll() ;
	
	// wait to settle down
	sleep( Globals.restart_seconds ) ;
}

static void RestartProgram( void * v )
{
	int argc = Globals.argc ;
	char * argv[argc+1] ;
	int i ;
	
	/* Copy arguements before cleaning up */
	for ( i=0 ; i <= argc ; ++i ) {
		argv[i] = NULL ;
		if ( Globals.argv[i] != NULL ) {
			argv[i] = strdup( Globals.argv[i] ) ;
		}		
	}
	
	// Close sockets
	StopProgram( v ) ;

	/* Execute fresh version */
	errno = 0 ;
	execvp( argv[0], argv ) ;
	
	Globals.exitmode = exit_normal ;
	fprintf(stderr,"Could not rerun %s. %s Exit\n",argv[0],strerror(errno));
	exit(0) ;
}

/* Restart program -- configuration file changed, presumably */
void ReExecute( void * v )
{
	LEVEL_CALL( "Restarting %s",Globals.argv[0] ) ;
	switch( Globals.inet_type ) {
		case inet_launchd:
			LEVEL_CALL("Will close %s and let the operating system (launchd) restart",Globals.argv[0] ) ;
			exit(0) ;
		case inet_systemd:
			LEVEL_CALL("Will close %s and let the operating system (systemd) restart",Globals.argv[0] ) ;
			exit(0) ;
		default:
			RestartProgram(v) ;
	}
}
		
