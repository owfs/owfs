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
#include "ow_devices.h"
#include "ow_pid.h"

#if defined(__UCLIBC__)
#if (defined(__UCLIBC_HAS_MMU__) || defined(__ARCH_HAS_MMU__))
#define HAVE_DAEMON 1
#else							/* __UCLIBC_HAS_MMU__ */
#undef HAVE_DAEMON
#endif							/* __UCLIBC_HAS_MMU__ */
#endif							/* __UCLIBC__ */

#ifndef HAVE_DAEMON
#include <sys/resource.h>
#include <sys/wait.h>

static void catchchild(int sig)
{
	pid_t pid;
	int status;

	pid = wait4(-1, &status, WUNTRACED, 0);
}

/*
  This is a test to implement daemon()
*/
static int my_daemon(int nochdir, int noclose)
{
	struct sigaction act;
	int pid;
	int file_descriptor;

	signal(SIGCHLD, SIG_DFL);

#if defined(__UCLIBC__)
	pid = vfork();
#else							/* __UCLIBC__ */
	pid = fork();
#endif							/* __UCLIBC__ */
	switch (pid) {
	case -1:
		memset(&act, 0, sizeof(act));
		act.sa_handler = catchchild;
		act.sa_flags = SA_RESTART;
		sigaction(SIGCHLD, &act, NULL);
//printf("owlib: my_daemon: pid=%d fork error\n", getpid());
		LEVEL_CALL("Libsetup ok\n");

		return (-1);
	case 0:
		break;
	default:
		//signal(SIGCHLD, SIG_DFL);
//printf("owlib: my_daemon: pid=%d exit parent\n", getpid());
		_exit(0);
	}

	if (setsid() < 0) {
		perror("setsid:");
		return -1;
	}

	/* Make certain we are not a session leader, or else we
	 * might reacquire a controlling terminal */
#ifdef __UCLIBC__
	pid = vfork();
#else							/* __UCLIBC__ */
	pid = fork();
#endif							/* __UCLIBC__ */
	if (pid) {
		//printf("owlib: my_daemon: _exit() pid=%d\n", getpid());
		_exit(0);
	}

	memset(&act, 0, sizeof(act));
	act.sa_handler = catchchild;
	act.sa_flags = SA_RESTART;
	sigaction(SIGCHLD, &act, NULL);

	if (!nochdir) {
		chdir("/");
	}

	if (!noclose) {
		close(STDIN_FILENO);
		close(STDOUT_FILENO);
		close(STDERR_FILENO);
		if (dup(dup(open("/dev/null", O_APPEND))) == -1) {
			perror("dup:");
			return -1;
		}
	}
	return 0;
}
#endif							/* HAVE_DAEMON */

/* Start the owlib process -- actually only tests for backgrounding */
int EnterBackground(void)
{
	/* First call to pthread should be done after daemon() in uClibc, so
	 * I moved it here to avoid calling __pthread_initialize() */

	/* daemon() is called BEFORE initialization of USB adapter etc... Cygwin will fail to
	 * use the adapter after daemon otherwise. Some permissions are changed on the process
	 * (or process-group id) which libusb-win32 is depending on. */
	//printf("Enter Background\n") ;
	if (Globals.want_background) {
		switch (Globals.opt) {
		case opt_owfs:
			// handles PID from a callback
			break;
		case opt_httpd:
		case opt_ftpd:
		case opt_server:
			if (
#ifdef HAVE_DAEMON
				   daemon(1, 0)
#else							/* HAVE_DAEMON */
				   my_daemon(1, 0)
#endif							/* HAVE_DAEMON */
				) {
				LEVEL_DEFAULT("Cannot enter background mode, quitting.\n");
				return 1;
			} else {
				Globals.now_background = 1;
#ifdef __UCLIBC__
				/* Have to re-initialize pthread since the main-process is gone.
				 *
				 * This workaround will probably be fixed in uClibc-0.9.28
				 * Other uClibc developers have noticed similar problems which are
				 * trigged when pthread functions are used in shared libraries. */
				LockSetup();
#endif							/* __UCLIBC__ */
			}
		default:
			PIDstart();
			break;
		}
	} else {					// not background
		if (Globals.opt != opt_owfs) {
			PIDstart();
		}
	}
	//printf("Exit Background\n") ;

	return 0;
}
