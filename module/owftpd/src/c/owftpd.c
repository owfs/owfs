/*
 * owftpd -- an ftp server for owfs 1-wire "virtual" files.
 * No actual file contents are exposed.
 * see owfs.org
 * Paul Alfille -- GPLv2
 */

#include "owftpd.h"
#include <pwd.h>

int main(int argc, char *argv[])
{
	int c;
	int err, signo;
	struct ftp_listener_s ftp_listener;
	sigset_t myset;
	
	struct re_exec sre =
	{
		& ftp_listener, 
		ftp_listener_stop, 
	};

	/* Set up owlib */
	LibSetup(program_type_ftpd);
	Setup_Systemd() ; // systemd?
	Setup_Launchd() ; // launchd?

	/* grab our executable name */
	ArgCopy( argc, argv ) ;

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
		// except systemd
	}

	/* non-option args are adapters */
	while (optind < argc) {
		//printf("Adapter(%d): %s\n",optind,argv[optind]);
		ARG_Generic(argv[optind]);
		++optind;
	}

	/* become a daemon if not told otherwise */
	if ( BAD(EnterBackground()) ) {
		ow_exit(1);
	}

	sigemptyset(&myset);
	sigaddset(&myset, SIGHUP);
	sigaddset(&myset, SIGINT);
	// SIGTERM is used to kill all outprocesses which hang in accept()
	pthread_sigmask(SIG_BLOCK, &myset, NULL);

	/* Set up adapters and systemd */
	if ( BAD(LibStart(&sre)) ) {
		ow_exit(1);
	}

	set_exit_signal_handlers(exit_handler);
	set_signal_handlers(NULL);

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
		Announce_Systemd() ; // If in systemd mode announce we're ready.
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

