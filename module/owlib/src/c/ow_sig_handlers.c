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

void set_signal_handlers(void (*exit_handler)
						  (int signo, siginfo_t * info, void *context))
{
	struct sigaction sa;

	memset(&sa, 0, sizeof(struct sigaction));
	sigemptyset(&(sa.sa_mask));

	sa.sa_flags = SA_SIGINFO;
	sa.sa_sigaction = exit_handler;
	if ((sigaction(SIGINT, &sa, NULL) == -1) || (sigaction(SIGTERM, &sa, NULL) == -1)) {
		LEVEL_DEFAULT("Cannot set exit signal handlers\n");
		//exit_handler (-1, NULL, NULL);
		exit(1);
	}
}
