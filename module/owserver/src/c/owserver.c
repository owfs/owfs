/*
$Id$
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
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
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

/* --- Prototypes ------------ */
static void ow_exit(int e);
static void SetupAntiloop(void);

#if OW_MT
extern pthread_mutex_t persistence_mutex;
#endif

static void ow_exit(int e)
{
	LEVEL_DEBUG("ow_exit %d\n", e);
	if (IS_MAINTHREAD) {
		LibClose();
	}
#ifdef __UCLIBC__
	/* Process never die on WRT54G router with uClibc if exit() is used */
	_exit(e);
#else
	exit(e);
#endif
}

static void exit_handler(int signo, siginfo_t * info, void *context)
{
	(void) context;
#if OW_MT
	if (info) {
		LEVEL_DEBUG
			("exit_handler: for %d, errno %d, code %d, pid=%ld, self=%lu main=%lu\n",
			 signo, info->si_errno, info->si_code, (long int) info->si_pid, pthread_self(), main_threadid);
	} else {
		LEVEL_DEBUG("exit_handler: for %d, self=%lu, main=%lu\n", signo, pthread_self(), main_threadid);
	}
	if (!StateInfo.shutdown_in_progress) {
		StateInfo.shutdown_in_progress = 1;

		if (info != NULL) {
			if (SI_FROMUSER(info)) {
				LEVEL_DEBUG("exit_handler: kill from user\n");
			}
			if (SI_FROMKERNEL(info)) {
				LEVEL_DEBUG("exit_handler: kill from kernel\n");
			}
		}
		if (!IS_MAINTHREAD) {
			LEVEL_DEBUG("exit_handler: kill mainthread %lu self=%d signo=%d\n", main_threadid, pthread_self(), signo);
			pthread_kill(main_threadid, signo);
		}
	}
#else
	(void) signo;
	(void) info;
	StateInfo.shutdown_in_progress = 1;
#endif
	return;
}

int main(int argc, char **argv)
{
	int c;

	/* Set up owlib */
	LibSetup(opt_server);

	/* grab our executable name */
	if (argc > 0)
		Global.progname = strdup(argv[0]);

	while ((c = getopt_long(argc, argv, OWLIB_OPT, owopts_long, NULL)) != -1) {
		switch (c) {
		case 'V':
			fprintf(stderr, "%s version:\n\t" VERSION "\n", argv[0]);
			break;
		default:
			break;
		}
		if (owopt(c, optarg))
			ow_exit(0);			/* rest of message */
	}

	/* non-option arguments */
	while (optind < argc) {
		OW_ArgGeneric(argv[optind]);
		++optind;
	}

	if (count_outbound_connections == 0) {
		if (Global.announce_off) {
			LEVEL_DEFAULT("No TCP port specified (-p)\n%s -h for help\n", argv[0]);
			ow_exit(1);
		}
		OW_ArgServer(NULL);		// make the default assignment
	}

	set_signal_handlers(exit_handler);

	/*
	 * Now we drop privledges and become a daemon.
	 */
	if (LibStart())
		ow_exit(1);
#if OW_MT
	pthread_mutex_init(&persistence_mutex, Mutex.pmattr);

	main_threadid = pthread_self();
	LEVEL_DEBUG("main_threadid = %lu\n", (unsigned long int) main_threadid);
#endif

	/* Set up "Antiloop" -- a unique token */
	SetupAntiloop();

	ServerProcess(Handler, ow_exit);
	return 0;
}

static void SetupAntiloop(void)
{
	struct tms t;
	Global.Token.simple.pid = getpid();
	Global.Token.simple.clock = times(&t);
}
