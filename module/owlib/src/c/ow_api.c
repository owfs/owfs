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
/* Uses two mutexes, one for count_inbound_connections, showing of there are valid 1-wire adapters */
/* Second is paired with a condition variable to prevent "finish" when a "get" or "put" is in progress */
/* Thanks to Geo Carncross for the implementation */

#include <config.h>
#include "owfs_config.h"
#include "ow.h"

/* ------- Globals ----------- */
#if OW_MT
pthread_mutex_t init_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t access_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t access_cond = PTHREAD_COND_INITIALIZER;
#define INITLOCK       pthread_mutex_lock(    &init_mutex   )
#define INITUNLOCK     pthread_mutex_unlock(  &init_mutex   )
#define ACCESSLOCK     pthread_mutex_lock(  &access_mutex   )
#define ACCESSUNLOCK   pthread_mutex_unlock(&access_mutex   )
#define ACCESSWAIT     pthread_cond_wait(    &access_cond, &access_mutex )
#define ACCESSSIGNAL   pthread_cond_signal(  &access_cond )
#else							/* OW_MT */
#define INITLOCK
#define INITUNLOCK
#define ACCESSLOCK
#define ACCESSUNLOCK
#define ACCESSWAIT
#define ACCESSSIGNAL
#endif							/* OW_MT */

int access_num = 0;

/* Returns 0 if ok, else 1 */
/* MUST BE PAIRED with OWLIB_can_init_end() */
int OWLIB_can_init_start(void)
{
	int ids;
#if OW_MT
#ifdef __UCLIBC__
	if (INITLOCK == EINVAL) {	/* Not initialized */
		pthread_mutex_init(&init_mutex, Mutex.pmattr);
		pthread_mutex_init(&access_mutex, Mutex.pmattr);
		INITLOCK;
	}
#else							/* UCLIBC */
	INITLOCK;
#endif							/* UCLIBC */
#endif							/* OW_MT */
	CONNINLOCK;
	ids = count_inbound_connections;
	CONNINUNLOCK;
	return ids > 0;
}

void API_setup( enum opt_program opt )
{
    static int deja_vue = 0 ;
    // poor mans lock for the Lib Setup and Lock Setup
    if ( ++deja_vue == 1 ) {
        LibSetup(opt) ;
        StateInfo.owlib_state = lib_state_setup ;
    }
}

/* Swig ensures that API_LibSetup is called first, but still we check */
int API_init( const char * command_line )
{
    int return_code = 0 ;

    if ( StateInfo.owlib_state == lib_state_pre ) {
        LibSetup(Global.opt) ; // use previous or default value
        StateInfo.owlib_state = lib_state_setup ;
    }
    LIBWRITELOCK ;
    // stop if started
    if ( StateInfo.owlib_state == lib_state_started ) {
        LibStop() ;
        StateInfo.owlib_state = lib_state_setup ;
    }
        
    // now restart
    if ( StateInfo.owlib_state == lib_state_setup ) {
        return_code = owopt_packed( command_line ) ;
        if ( return_code != 0 ) {
            LIBWRITEUNLOCK ;
            return return_code ;
        }
        
        return_code = LibStart() ;
        if ( return_code != 0 ) {
            LIBWRITEUNLOCK ;
            return return_code ;
        }
            
        StateInfo.owlib_state = lib_state_started ;
    }
    LIBWRITEUNLOCK ;
    return return_code ;
}

void API_finish( void )
{
    if ( StateInfo.owlib_state == lib_state_pre ) {
        return ;
    }
    LIBWRITELOCK ;
    LibStop() ;
    StateInfo.owlib_state = lib_state_pre ;
    LIBWRITEUNLOCK ;
}

// called before read/write/dir operation -- tests setup state
// pair with API_access_stop
int API_access_start( void )
{
    if ( StateInfo.owlib_state == lib_state_pre ) {
        return -EACCES ;
    }
    LIBREADLOCK ;
    if ( StateInfo.owlib_state != lib_state_started ) {
        LIBREADUNLOCK ;
        return -EACCES ;
    }
    return 0 ;
}

// called after read/write/dir operation
// pair with API_access_start
void API_access_end( void )
{
    LIBREADUNLOCK ;
}

