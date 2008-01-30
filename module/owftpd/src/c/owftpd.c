/*
 * $Id$
 */

#include "owftpd.h"
#include <pwd.h>

/* put our executable name here where everybody can see it */
static void ow_exit(int e);

int main(int argc, char *argv[])
{
	int c;
	struct ftp_listener_s ftp_listener;
	sigset_t term_signal;
	int sig;

	/* Set up owlib */
	LibSetup(opt_ftpd);

	/* grab our executable name */
	if (argc > 0)
		Global.progname = strdup(argv[0]);

	/* check our command-line arguments */
	while ((c = getopt_long(argc, argv, OWLIB_OPT, owopts_long, NULL)) != -1) {
		switch (c) {
		case 'V':
			fprintf(stderr, "%s version:\n\t" VERSION "\n", argv[0]);
			break;
		}
		if (owopt(c, optarg))
			ow_exit(0);			/* rest of message */
	}

	/* FTP on default port? */
	if (count_outbound_connections == 0)
		OW_ArgServer("0.0.0.0:21");	// well known port

	/* non-option args are adapters */
	while (optind < argc) {
		//printf("Adapter(%d): %s\n",optind,argv[optind]);
		OW_ArgGeneric(argv[optind]);
		++optind;
	}

	/* Need at least 1 adapter */
	if (count_inbound_connections == 0 && !Global.autoserver) {
		LEVEL_DEFAULT("Need to specify at least one 1-wire adapter.\n");
		ow_exit(1);
	}

	/* Set up adapters */
	if (LibStart())
		ow_exit(1);

	/* avoid SIGPIPE on socket activity */
	signal(SIGPIPE, SIG_IGN);

	/* create our main listener */
	if (!ftp_listener_init(&ftp_listener)) {
		LEVEL_CONNECT("Problem initializing FTP listener\n");
		ow_exit(1);
	}

	/* start our listener */
	if (ftp_listener_start(&ftp_listener) == 0) {
		LEVEL_CONNECT("Problem starting FTP service\n");
		ow_exit(1);
	}

	/* wait for a SIGTERM and exit gracefully */
	sigemptyset(&term_signal);
	sigaddset(&term_signal, SIGTERM);
	sigaddset(&term_signal, SIGINT);
	pthread_sigmask(SIG_BLOCK, &term_signal, NULL);
	sigwait(&term_signal, &sig);
	if (sig == SIGTERM) {
		LEVEL_CONNECT("SIGTERM received, shutting down");
	} else {
		LEVEL_CONNECT("SIGINT received, shutting down");
	}
	ftp_listener_stop(&ftp_listener);
	LEVEL_CONNECT("All connections finished, FTP server exiting");
	ow_exit(0);
	return 0;
}

static void ow_exit(int e)
{
	LibClose();
	/* Process never die on WRT54G router with uClibc if exit() is used */
	_exit(e);
}
