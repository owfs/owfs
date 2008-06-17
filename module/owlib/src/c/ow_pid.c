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
#include "ow_pid.h"

/* Globalss */
int pid_created = 0;			/* flag flag when file actually created */
char *pid_file = NULL;			/* filename where PID number stored */

void PIDstart(void)
{
/* store the PID */
	pid_t pid_num = getpid();

	if (pid_file) {
		FILE *pid = fopen(pid_file, "w+");
		if (pid == NULL) {
			ERROR_CONNECT("Cannot open PID file: %s\n", pid_file);
			free(pid_file);
			pid_file = NULL;
		} else {
			fprintf(pid, "%lu", (unsigned long int) pid_num);
			fclose(pid);
			pid_created = 1;
		}
	}
}

/* All ow library closeup */
void PIDstop(void)
{
	if (pid_created && pid_file) {
		if (unlink(pid_file)) {
			ERROR_CONNECT("Cannot remove PID file: %s\n", pid_file);
		}
		free(pid_file);
		pid_file = NULL;
	}
}
