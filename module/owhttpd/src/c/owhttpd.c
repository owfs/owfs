/*
$Id$
    OW_HTML -- OWFS used for the web
    OW -- One-Wire filesystem

    Written 2003 Paul H Alfille

 * based on chttpd/1.0
 * main program. Somewhat adapted from dhttpd
 * Copyright (c) 0x7d0 <noop@nwonknu.org>
 * Some parts
 * Copyright (c) 0x7cd David A. Bartold
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

#include "owhttpd.h"			// httpd-specific

/*
 * Default port to listen too. If you aren't root, you'll need it to
 * be > 1024 unless you plan on using a config file
 */
#define DEFAULTPORT    80

static void Acceptor(int listenfd);

#if OW_MT
pthread_t main_threadid;
#define IS_MAINTHREAD (main_threadid == pthread_self())
#else
#define IS_MAINTHREAD 1
#endif

static void ow_exit(int e)
{
	LEVEL_DEBUG("ow_exit %d", e);
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
			("exit_handler: for signo=%d, errno %d, code %d, pid=%ld, self=%lu main=%lu",
			 signo, info->si_errno, info->si_code, (long int) info->si_pid, pthread_self(), main_threadid);
	} else {
		LEVEL_DEBUG("exit_handler: for signo=%d, self=%lu, main=%lu", signo, pthread_self(), main_threadid);
	}
	if (StateInfo.shutdown_in_progress) {
	  LEVEL_DEBUG("exit_handler: shutdown already in progress. signo=%d, self=%lu, main=%lu", signo, pthread_self(), main_threadid);
	}
	if (!StateInfo.shutdown_in_progress) {
		StateInfo.shutdown_in_progress = 1;

		if (info != NULL) {
			if (SI_FROMUSER(info)) {
				LEVEL_DEBUG("exit_handler: kill from user");
			}
			if (SI_FROMKERNEL(info)) {
				LEVEL_DEBUG("exit_handler: kill from kernel");
			}
		}
		if (!IS_MAINTHREAD) {
			LEVEL_DEBUG("exit_handler: kill mainthread %lu self=%lu signo=%d", main_threadid, pthread_self(), signo);
			pthread_kill(main_threadid, signo);
		} else {
			LEVEL_DEBUG("exit_handler: ignore signal, mainthread %lu self=%lu signo=%d", main_threadid, pthread_self(), signo);
		}
	}
#else
	(void) signo;
	(void) info;
	StateInfo.shutdown_in_progress = 1;
#endif
	return;
}

int main(int argc, char *argv[])
{
	int c;

	/* Set up owlib */
	LibSetup(opt_httpd);

	/* grab our executable name */
	if (argc > 0) {
		Globals.progname = owstrdup(argv[0]);
	}

	while ((c = getopt_long(argc, argv, OWLIB_OPT, owopts_long, NULL)) != -1) {
		switch (c) {
		case 'V':
			fprintf(stderr, "%s version:\n\t" VERSION "\n", argv[0]);
			break;
		default:
			break;
		}
		if (owopt(c, optarg)) {
			ow_exit(0);			/* rest of message */
		}
	}

	/* non-option arguments */
	while (optind < argc) {
		ARG_Generic(argv[optind]);
		++optind;
	}

	if (Outbound_Control.active == 0) {
		if (Globals.zero == zero_none) {
			LEVEL_DEFAULT("%s would be \"locked in\" so will quit.\nBonjour and Avahi not available.", argv[0]);
			ow_exit(1);
		} else {
			LEVEL_CONNECT("%s will use an ephemeral port", argv[0]) ;
		}
		ARG_Server(NULL);		// make an ephemeral assignment
	}

	set_exit_signal_handlers(exit_handler);
	set_signal_handlers(NULL);

	/* become a daemon if not told otherwise */
	if (EnterBackground()) {
		ow_exit(1);
	}

	/*
	 * Now we drop privledges and become a daemon.
	 */
	if (LibStart()) {
		ow_exit(1);
	}
#if OW_MT
	main_threadid = pthread_self();
#endif

	ServerProcess(Acceptor, ow_exit);
	LEVEL_DEBUG("ServerProcess done");
	ow_exit(0);
	return 0;
}

static void Acceptor(int listenfd)
{
	FILE *fp = fdopen(listenfd, "w+");
	if (fp) {
		handle_socket(fp);
		fflush(fp);
		fclose(fp);
	}
}
