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

/* code for SWIG and owcapi safe access */
/* Uses two mutexes, one for inbound_connections, showing of there are valid 1-wire adapters */
/* Second is paired with a condition variable to prevent "finish" when a "get" or "put" is in progress */
/* Thanks to Geo Carncross for the implementation */

#include <config.h>
#include "owfs_config.h"
#include "ow.h"

/* ------- Globalss ----------- */
#if OW_MT
pthread_mutex_t init_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t access_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t access_cond = PTHREAD_COND_INITIALIZER;
#define INITLOCK       MUTEX_LOCK(    init_mutex   )
#define INITUNLOCK     MUTEX_UNLOCK(  init_mutex   )
#define ACCESSLOCK     MUTEX_LOCK(  access_mutex   )
#define ACCESSUNLOCK   MUTEX_UNLOCK(access_mutex   )
#define ACCESSWAIT     my_pthread_cond_wait(    &access_cond, &access_mutex )
#define ACCESSSIGNAL   my_pthread_cond_signal(  &access_cond )
#else							/* OW_MT */
#define INITLOCK		return_ok()
#define INITUNLOCK		return_ok()
#define ACCESSLOCK		return_ok()
#define ACCESSUNLOCK	return_ok()
#define ACCESSWAIT
#define ACCESSSIGNAL
#endif							/* OW_MT */

int access_num = 0;

void API_setup(enum opt_program opt)
{
	static int deja_vue = 0;
	// poor mans lock for the Lib Setup and Lock Setup
	if (++deja_vue == 1) {
		// first time through
		LibSetup(opt);
		MUTEX_INIT( init_mutex ) ;
		MUTEX_INIT( access_mutex ) ;
		StateInfo.owlib_state = lib_state_setup;
	}
}

void API_set_error_level(const char *params)
{
	if (params != NULL) {
		Globals.error_level = atoi(params);
	}
	return;
}

void API_set_error_print(const char *params)
{
	if (params != NULL) {
		Globals.error_print = atoi(params);
	}
	return;
}


/* Swig ensures that API_LibSetup is called first, but still we check */
GOOD_OR_BAD API_init(const char *command_line)
{
	GOOD_OR_BAD return_code = gbGOOD;

	if (StateInfo.owlib_state == lib_state_pre) {
		LibSetup(Globals.opt);	// use previous or default value
		StateInfo.owlib_state = lib_state_setup;
	}
	LIB_WLOCK;
	// stop if started
	if (StateInfo.owlib_state == lib_state_started) {
		LibStop();
		StateInfo.owlib_state = lib_state_setup;
	}
	// now restart
	if (StateInfo.owlib_state == lib_state_setup) {
		return_code = owopt_packed(command_line);
		if ( BAD(return_code) ) {
			LIB_WUNLOCK;
			return return_code;
		}

		LibStart();

		StateInfo.owlib_state = lib_state_started;
	}
	LIB_WUNLOCK;
	return return_code;
}

void API_finish(void)
{
	if (StateInfo.owlib_state == lib_state_pre) {
		return;
	}
	LIB_WLOCK;
	LibStop();
	StateInfo.owlib_state = lib_state_pre;
	LIB_WUNLOCK;
}

// called before read/write/dir operation -- tests setup state
// pair with API_access_stop
int API_access_start(void)
{
	if (StateInfo.owlib_state == lib_state_pre) {
		return -EACCES;
	}
	LIB_RLOCK;
	if (StateInfo.owlib_state != lib_state_started) {
		LIB_RUNLOCK;
		return -EACCES;
	}
	return 0;
}

// called after read/write/dir operation
// pair with API_access_start
void API_access_end(void)
{
	LIB_RUNLOCK;
}
