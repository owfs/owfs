/*
$Id$
    OWFS -- One-Wire filesystem
    OWHTTPD -- One-Wire Web Server
    Written 2003 Paul H Alfille
	email: paul.alfille@gmail.com
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

/* ------- Globals ----------- */
pthread_mutex_t init_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t access_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t access_cond = PTHREAD_COND_INITIALIZER;

#define INITINIT       _MUTEX_INIT( init_mutex )
#define ACCESSINIT     _MUTEX_INIT( access_mutex )
#define INITLOCK       _MUTEX_LOCK(    init_mutex   )
#define INITUNLOCK     _MUTEX_UNLOCK(  init_mutex   )
#define ACCESSLOCK     _MUTEX_LOCK(  access_mutex   )
#define ACCESSUNLOCK   _MUTEX_UNLOCK(access_mutex   )
#define ACCESSWAIT     my_pthread_cond_wait(    &access_cond, &access_mutex )
#define ACCESSSIGNAL   my_pthread_cond_signal(  &access_cond )

int access_num = 0;

void API_setup(enum enum_program_type program_type)
{
	static int deja_vue = 0;
	// poor mans lock for the Lib Setup and Lock Setup
	if (++deja_vue == 1) {
		// first time through
		LibSetup(program_type);
		INITINIT ;
		ACCESSINIT ;
		StateInfo.owlib_state = lib_state_setup;
	}
}

void API_set_error_level(const char *params)
{
	if (params != NULL) {
		Globals.error_level = atoi(params);
		Globals.error_level_restore = Globals.error_level;
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


/* Swig ensures that API_Setup is called first, but still we check */
GOOD_OR_BAD API_init(const char *command_line)
{
	GOOD_OR_BAD return_code = gbGOOD;

	LEVEL_DEBUG("OWLIB started with <%s>",SAFESTRING(command_line));

	if (StateInfo.owlib_state == lib_state_pre) {
		LibSetup(Globals.program_type);	// use previous or default value
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
			goto init_exit ;
		}

		return_code = LibStart();
		if ( BAD(return_code) ) {
			goto init_exit ;
		}

		StateInfo.owlib_state = lib_state_started;
	}
	
init_exit:	
	LIB_WUNLOCK;
	LEVEL_DEBUG("OWLIB started with <%s>",SAFESTRING(command_line));
	return return_code;
}

void API_finish(void)
{
	LEVEL_DEBUG("OWLIB being stopped");
	
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
