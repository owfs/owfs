/*
 * $Id$
 */

#include "owftpd.h"
#include <pwd.h>

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
	if (StateInfo.shutting_down) {
	  LEVEL_DEBUG("exit_handler: shutdown already in progress. signo=%d, self=%lu, main=%lu", signo, pthread_self(), main_threadid);
	}
	if (!StateInfo.shutting_down) {
		StateInfo.shutting_down = 1;

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
	StateInfo.shutting_down = 1;
#endif
	return;
}

int main(int argc, char *argv[])
{
	int c;
	int err, signo;
	struct ftp_listener_s ftp_listener;
	sigset_t myset;

	/* Set up owlib */
	LibSetup(opt_ftpd);

	/* grab our executable name */
	if (argc > 0) {
		Globals.progname = strdup(argv[0]);
	}

#if OW_MT
	main_threadid = pthread_self();
#endif
	
	/* check our command-line arguments */
	while ((c = getopt_long(argc, argv, OWLIB_OPT, owopts_long, NULL)) != -1) {
		switch (c) {
		case 'V':
			fprintf(stderr, "%s version:\n\t" VERSION "\n", argv[0]);
			break;
		}
		if ( BAD( owopt(c, optarg) ) ) {
			ow_exit(0);			/* rest of message */
		}
	}

	/* FTP on default port? */
	if (Outbound_Control.active == 0) {
		ARG_Server(NULL);	// default or ephemeral port
	}

	/* non-option args are adapters */
	while (optind < argc) {
		//printf("Adapter(%d): %s\n",optind,argv[optind]);
		ARG_Generic(argv[optind]);
		++optind;
	}

	set_exit_signal_handlers(exit_handler);
	set_signal_handlers(NULL);

	/* become a daemon if not told otherwise */
	if ( BAD(EnterBackground()) ) {
		ow_exit(1);
	}

	/* Set up adapters */
	if ( BAD(LibStart()) ) {
		ow_exit(1);
	}

#if OW_MT
	main_threadid = pthread_self();
	LEVEL_DEBUG("main_threadid = %lu", (unsigned long int) main_threadid);
#endif

	sigemptyset(&myset);
	sigaddset(&myset, SIGHUP);
	sigaddset(&myset, SIGINT);
	// SIGTERM is used to kill all outprocesses which hang in accept()
	pthread_sigmask(SIG_BLOCK, &myset, NULL);

	/* create our main listener */
	if (!ftp_listener_init(&ftp_listener)) {
		LEVEL_CONNECT("Problem initializing FTP listener");
		ow_exit(1);
	}

	/* start our listener */
	if (ftp_listener_start(&ftp_listener) == 0) {
		LEVEL_CONNECT("Problem starting FTP service");
		ow_exit(1);
	}

	sigemptyset(&myset);
	sigaddset(&myset, SIGHUP);
	sigaddset(&myset, SIGINT);
	sigaddset(&myset, SIGTERM);

	while (!StateInfo.shutting_down) {
		if ((err = sigwait(&myset, &signo)) == 0) {
			if (signo == SIGHUP) {
				LEVEL_DEBUG("owftpd: ignore signo=%d", signo);
				continue;
			}
			LEVEL_DEBUG("owftpd: break signo=%d", signo);
			break;
		} else {
			LEVEL_DEBUG("owftpd: sigwait error %d", err);
		}
	}

	StateInfo.shutting_down = 1;
	LEVEL_DEBUG("owftpd shutdown initiated");

	ftp_listener_stop(&ftp_listener);
	LEVEL_CONNECT("All connections finished, FTP server exiting");

	ow_exit(0);
	return 0;
}

