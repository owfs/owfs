/*
$Id$
    OWFS -- One-Wire filesystem
    OWHTTPD -- One-Wire Web Server
    Written 2003 Paul H Alfille
    email: palfille@earthlink.net
    Released under the GPL
    See the header file: ow.h for full attribution
    1wire/iButton system from Dallas Semiconductor
*/

#include <config.h>
#include "owfs_config.h"
#include "ow.h"
#include "ow_connection.h"
#include "ow_pid.h"
#include "ow_server.c"

void SetSignals(void);

// flag to initiate shutdown
int shutdown_in_progress = 0;

/* All ow library setup */
void LibSetup(void)
{
#if OW_ZERO
	OW_Load_dnssd_library();
#endif
    
#ifndef __UCLIBC__
	/* Setup the multithreading synchronizing locks */
	LockSetup();
#endif							/* __UCLIBC__ */

	start_time = time(NULL);
	errno = 0;					/* set error level none */
}

/* Start the owlib process -- actually only tests for backgrounding */
int LibStart(void)
{
	struct connection_in *in = head_inbound_list;
	int ret = 0;
	/* Initialize random number generator, make sure fake devices get the same
	 * id each time */
	srand(1);

#ifdef __UCLIBC__
	/* First call to pthread should be done after daemon() in uClibc, so
	 * I moved it here to avoid calling __pthread_initialize() */

#if OW_MT
	/* Have to re-initialize pthread since the main-process is gone.
	 *
	 * This workaround will probably be fixed in uClibc-0.9.28
	 * Other uClibc developers have noticed similar problems which are
	 * trigged when pthread functions are used in shared libraries. */
	__pthread_initial_thread_bos = NULL;
	__pthread_initialize();

	/* global mutex attribute */
	pthread_mutexattr_init(&mattr);
	pthread_mutexattr_settype(&mattr, PTHREAD_MUTEX_ADAPTIVE_NP);
	pmattr = &mattr;
#endif							/* OW_MT */

	/* Setup the multithreading synchronizing locks */
	LockSetup();
#endif							/* __UCLIBC__ */

	if (head_inbound_list == NULL && !ow_Global.autoserver) {
		LEVEL_DEFAULT
			("No device port/server specified (-d or -u or -s)\n%s -h for help\n",
			 SAFESTRING(Global.progname));
		return 1;
	}

	while (in) {
		switch (get_busmode(in)) {

		case bus_zero:
			if ((ret = Zero_detect(in))) {
				LEVEL_CONNECT("Cannot open server at %s\n", in->name);
			}
			break;

		case bus_server:
			if ((ret = Server_detect(in))) {
				LEVEL_CONNECT("Cannot open server at %s\n", in->name);
			}
			break;

		default:
			break;

		}

		if (ret) {				/* flag that that the adapter initiation was unsuccessful */
			BUS_close(in);		/* can use adapter's close */
		}
		in = in->next;
	}


	/* Use first bus for http bus name */
	CONNINLOCK;
	if (head_inbound_list)
		Global.SimpleBusName = head_inbound_list->name;
	CONNINUNLOCK;

	// zeroconf/Bonjour look for new services
	if (ow_Global.autoserver) {
#if OW_ZERO
		if (libdnssd == NULL) {
			fprintf(stderr,
					"Zeroconf/Bonjour is disabled since dnssd library isn't found.\n");
			exit(0);
		} else {
			OW_Browse();
		}
#else
		fprintf(stderr,
				"OWFS is compiled without Zeroconf/Bonjour support.\n");
		exit(0);
#endif
	}
	// Signal handlers
	SetSignals();

	return 0;
}

void SigHandler(int signo, siginfo_t * info, void *context)
{
	(void) context;
#if OW_MT
    if (info) {
    LEVEL_DEBUG
            ("Signal handler for %d, errno %d, code %d, pid=%ld, self=%lu\n",
             signo, info->si_errno, info->si_code, (long int) info->si_pid,
             pthread_self());
    } else {
        LEVEL_DEBUG("Signal handler for %d, self=%lu\n",
                    signo, pthread_self());
    }
#else /* OW_MT */
    if (info) {
    LEVEL_DEBUG
            ("Signal handler for %d, errno %d, code %d, pid=%ld\n",
             signo, info->si_errno, info->si_code, (long int) info->si_pid);
    } else {
        LEVEL_DEBUG("Signal handler for %d\n",
                    signo);
    }
#endif /* OW_MT */
    return;
}

void SetSignals(void)
{
	struct sigaction sa;

	memset(&sa, 0, sizeof(struct sigaction));
	sigemptyset(&(sa.sa_mask));

	sa.sa_flags = SA_SIGINFO;
	sa.sa_sigaction = SigHandler;
	if (sigaction(SIGHUP, &sa, NULL) == -1) {
		LEVEL_DEFAULT("Cannot handle SIGHUP\n");
		exit(0);
	}
	// more like a test to see if signals are handled correctly
	sa.sa_flags = SA_SIGINFO;
	sa.sa_sigaction = SigHandler;
	if (sigaction(SIGUSR1, &sa, NULL) == -1) {
		LEVEL_DEFAULT("Cannot handle SIGUSR1\n");
		exit(0);
	}

	sa.sa_flags = 0;
	sa.sa_handler = SIG_IGN;
	if (sigaction(SIGPIPE, &sa, NULL) == -1) {
		LEVEL_DEFAULT("Cannot ignore SIGPIPE\n");
		exit(0);
	}
}

/* All ow library closeup */
void LibClose(void)
{
	LEVEL_CALL("Starting Library cleanup\n");
	LEVEL_CALL("Closing input devices\n");
	OWNET_closeall();

#if OW_MT
	if (pmattr) {
		pthread_mutexattr_destroy(pmattr);
		pmattr = NULL;
	}
#endif							/* OW_MT */

	if (Global.announce_name)
		free(Global.announce_name);

	if (Global.progname)
		free(Global.progname);

#if OW_ZERO
	if (Global.browse && (libdnssd != NULL))
		DNSServiceRefDeallocate(Global.browse);

	OW_Free_dnssd_library();
#endif
	LEVEL_CALL("Finished Library cleanup\n");
	if (log_available)
		closelog();
}

/* Just close in/out devices and clear cache. Just enough to make it possible
   to call LibStart() again. This is called from swig/ow.i to when script
   wants to initialize a new server-connection. */
void LibStop(void)
{
	char *argv[1] = { NULL };
	LEVEL_CALL("Closing input devices\n");
	OWNET_closeall();


	/* Have to reset more internal variables, and this should be fixed
	 * by setting optind = 0 and call getopt()
         * (first_nonopt = last_nonopt = 1;)
	 */
	optind = 0;
	(void)getopt_long(1, argv, " ", NULL, NULL);

	optarg = NULL;
	optind = 1;
	opterr = 1;
	optopt = '?';
}

void set_signal_handlers(void (*exit_handler)
						  (int signo, siginfo_t * info, void *context))
{
	struct sigaction sa;

	memset(&sa, 0, sizeof(struct sigaction));
	sigemptyset(&(sa.sa_mask));

	sa.sa_flags = SA_SIGINFO;
	sa.sa_sigaction = exit_handler;
	if ((sigaction(SIGINT, &sa, NULL) == -1) ||
		(sigaction(SIGTERM, &sa, NULL) == -1)) {
		LEVEL_DEFAULT("Cannot set exit signal handlers\n");
		//exit_handler (-1, NULL, NULL);
		exit(1);
	}
}
