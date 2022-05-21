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

void ow_exit(int exit_code)
{
	LEVEL_DEBUG("Exit code = %d", exit_code);
	if (IS_MAINTHREAD) {
		switch ( Globals.exitmode ) {
			case exit_normal:
				LibClose();
				break ;
			case exit_exec:
				sleep( 2 * Globals.restart_seconds ) ; // wait for RestartProgram() to do it's work
				break ;
			case exit_early:
				break ;
		}
	}

#ifdef __UCLIBC__
	/* Process never die on WRT54G router with uClibc if exit() is used */
	_exit(exit_code);
#else
	exit(exit_code);
#endif
}

void exit_handler(int signo, siginfo_t * info, void *context)
{
	(void) context;

	if (info) {
		LEVEL_DEBUG
			("Signal=%d, errno %d, code %d, pid=%ld, Threads: this=%lu main=%lu",
			 signo, info->si_errno, info->si_code, (long int) info->si_pid, pthread_self(), main_threadid);
	} else {
		LEVEL_DEBUG("Signal=%d, Threads: this=%lu, main=%lu", signo, pthread_self(), main_threadid);
	}
	if (StateInfo.shutting_down) {
	  LEVEL_DEBUG("exit_handler: shutdown already in progress. signo=%d, self=%lu, main=%lu", signo, pthread_self(), main_threadid);
	} else {
		StateInfo.shutting_down = 1;

		if (info != NULL) {
			if (SI_FROMUSER(info)) {
				LEVEL_DEBUG("Kill signal from user");
			}
			if (SI_FROMKERNEL(info)) {
				LEVEL_DEBUG("Kill signal from system");
			}
		}
		if (!IS_MAINTHREAD) {
			LEVEL_DEBUG("Kill from main thread: %lu this=%lu Signal=%d", main_threadid, pthread_self(), signo);
			pthread_kill(main_threadid, signo);
		} else {
			LEVEL_DEBUG("Ignore kill from this thread. main=%lu this=%lu Signal=%d", main_threadid, pthread_self(), signo);
		}
	}
}
