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
	FILE_DESCRIPTOR_OR_ERROR file_descriptor;

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
		LEVEL_CALL("Libsetup ok");

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
GOOD_OR_BAD EnterBackground(void)
{
	/* First call to pthread should be done after daemon() in uClibc, so
	 * I moved it here to avoid calling __pthread_initialize() */

	/* daemon() is called BEFORE initialization of USB adapter etc... Cygwin will fail to
	 * use the adapter after daemon otherwise. Some permissions are changed on the process
	 * (or process-group id) which libusb-win32 is depending on. */
	//printf("Enter Background\n") ;
	switch (Globals.daemon_status) {
		case e_daemon_want_bg:
			switch (Globals.program_type) {
			case program_type_filesystem:
				// handles PID from a callback
				break;
			case program_type_httpd:
			case program_type_ftpd:
			case program_type_server:
			case program_type_external:
				if (
					// daemonize
					//   current directory left unchanged (not root)
					//   stdin, stdout and stderr sent to /dev/null
				#ifdef HAVE_DAEMON
					   daemon(1, 0)
				#else							/* HAVE_DAEMON */
					   my_daemon(1, 0)
				#endif							/* HAVE_DAEMON */
					) {
					LEVEL_DEFAULT("Cannot enter background mode, quitting.");
					return gbBAD;
				} else {
					Globals.daemon_status = e_daemon_bg ;
					LEVEL_DEFAULT("Entered background mode, quitting.");
				#ifdef __UCLIBC__
					/* Have to re-initialize pthread since the main-process is gone.
					 *
					 * This workaround will probably be fixed in uClibc-0.9.28
					 * Other uClibc developers have noticed similar problems which are
					 * trigged when pthread functions are used in shared libraries. */
					LockSetup();
				#endif							/* __UCLIBC__ */
				}
				PIDstart();
				break;
			default: // other type of program
				PIDstart();
				break;
			}
			break ;
		default: // not  want-background
			if (Globals.program_type != program_type_filesystem) {
				PIDstart();
			}
			break;
	}

	main_threadid = pthread_self();
	main_threadid_init = 1 ;
	LEVEL_DEBUG("main thread id = %lu", (unsigned long int) main_threadid);

	return gbGOOD;
}
