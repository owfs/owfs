/*
    OW_HTML -- OWFS used for the web
    OW -- One-Wire filesystem

    Written 2004 Paul H Alfille

 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

/* owserver -- responds to requests over a network socket, and processes them on the 1-wire bus/
         Basic idea: control the 1-wire bus and answer queries over a network socket
         Clients can be owperl, owfs, owhttpd, etc...
         Clients can be local or remote
                 Eventually will also allow bounce servers.

         syntax:
                 owserver
                 -u (usb)
                 -d /dev/ttyS1 (serial)
                 -p tcp port
                 e.g. 3001 or 10.183.180.101:3001 or /tmp/1wire
*/

#include "owserver.h"

pthread_mutex_t persistence_mutex ;

/* --- Prototypes ------------ */
static void SetupAntiloop(void);

int main(int argc, char **argv)
{
	int c;

	/* Set up owlib */
	LibSetup(program_type_server);
	Setup_Systemd() ; // systemd?
	Setup_Launchd() ; // launchd?

	/* grab our executable name */
	ArgCopy( argc, argv ) ;
	if ( strcasecmp( Globals.argv[0], "owexternal" ) == 0 ) {
		Globals.allow_external = 1 ; // only if program named "owexternal"
	}

	while ((c = getopt_long(argc, argv, OWLIB_OPT, owopts_long, NULL)) != -1) {
		switch (c) {
		case 'V':
			fprintf(stderr, "%s version:\n\t" VERSION "\n", argv[0]);
			break;
		default:
			break;
		}
		if ( BAD( owopt(c, optarg) ) ) {
			ow_exit(0);			/* rest of message */
		}
	}

	/* non-option arguments */
	while (optind < argc) {
		ARG_Generic(argv[optind]);
		++optind;
	}

	if (Outbound_Control.active == 0) {
		ARG_Server(NULL);		// make the default assignment or systemd
	}


	/* become a daemon if not told otherwise */
	if ( BAD(EnterBackground()) ) {
		ow_exit(1);
	}

	/* Set up adapters and systemd*/
	if ( BAD(LibStart(NULL)) ) {
		ow_exit(1);
	}


	set_exit_signal_handlers(exit_handler);
	set_signal_handlers(NULL);

	_MUTEX_INIT(persistence_mutex);

	/* Set up "Antiloop" -- a unique token */
	SetupAntiloop();

	/* Call up main processing routine -- waits for network queries */
	ServerProcess( Handler );
	LEVEL_DEBUG("ServerProcess done");

	_MUTEX_DESTROY(persistence_mutex);

	ow_exit(0);
	return 0;
}

static void SetupAntiloop(void)
{
	/* A structure of relatively unique items to hash together */
	struct {
		struct tms t;
		pid_t pid ;
		long int rand ;
	} data_struct ;

	/* clock ticks elapsed since startup */
	times( & (data_struct.t) );

	/* process ID */
	data_struct.pid = getpid() ;

	/* random number (seeded from current time) */
	srandom(time(0)) ;
	data_struct.rand = random() ;

	/* backup definition */
	memcpy( Globals.Token.uuid, (void *) &data_struct, 16 ) ;

	/* use MD5 has (gives the required 16 bytes) */
	md5( (void *) &data_struct, sizeof(data_struct) , Globals.Token.uuid ) ;
}
