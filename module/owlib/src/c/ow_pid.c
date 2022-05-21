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
#include "ow_pid.h"
#include "sd-daemon.h"

/* Globals */
int pid_created = 0;			/* flag flag when file actually created */
char *pid_file = NULL;			/* filename where PID number stored */

void PIDstart(void)
{
	/* store the PID */
	pid_t pid_num = getpid();

	if (pid_file) {
		FILE *pid = fopen(pid_file, "w+");
		if (pid == NULL) {
			ERROR_CONNECT("Cannot open PID file: %s", pid_file);
			owfree(pid_file);
			pid_file = NULL;
		} else {
			fprintf(pid, "%lu", (unsigned long int) pid_num);
			fclose(pid);
			pid_created = 1;
		}
	}
	
	/* Send the PID to systemd 
	 * -- will be a NOP if systed not available */
	sd_notifyf( 0, "MAINPID=%d",pid_num ) ;
}

/* At library closeup */
void PIDstop(void)
{
	if (pid_created && pid_file) {
		if (unlink(pid_file)) {
			ERROR_CONNECT("Cannot remove PID file: %s", pid_file);
		}
		SAFEFREE(pid_file);
	}
}
