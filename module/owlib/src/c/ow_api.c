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
/* Uses two mutexes, one for indevices, showing of there are valid 1-wire adapters */
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
#else				/* OW_MT */
#define INITLOCK
#define INITUNLOCK
#define ACCESSLOCK
#define ACCESSUNLOCK
#define ACCESSWAIT
#define ACCESSSIGNAL
#endif				/* OW_MT */

int access_num = 0;

/* Returns 0 if ok, else 1 */
/* MUST BE PAIRED with OWLIB_can_init_end() */
int OWLIB_can_init_start(void)
{
    int ids;
#if OW_MT
#ifdef __UCLIBC__
    if (INITLOCK == EINVAL) {	/* Not initialized */
	pthread_mutex_init(&init_mutex, pmattr);
	pthread_mutex_init(&access_mutex, pmattr);
	INITLOCK;
    }
#else				/* UCLIBC */
    INITLOCK;
#endif				/* UCLIBC */
#endif				/* OW_MT */
    CONNINLOCK;
    ids = indevices;
    CONNINUNLOCK;
    return ids > 0;
}

void OWLIB_can_init_end(void)
{
    INITUNLOCK;
}

/* Returns 0 if ok, else 1 */
/* MUST BE PAIRED with OWLIB_can_access_end() */
int OWLIB_can_access_start(void)
{
    int ret;
    ACCESSLOCK;
    access_num++;
    ACCESSUNLOCK;
    INITLOCK;
    CONNINLOCK;
    ret = (indevices == 0);
    CONNINUNLOCK;
    INITUNLOCK;
    return ret;
}

void OWLIB_can_access_end(void)
{
    ACCESSLOCK;
    access_num--;
    ACCESSSIGNAL;
    ACCESSUNLOCK;
}

/* Returns 0 always */
/* MUST BE PAIRED with OWLIB_can_finish_end() */
int OWLIB_can_finish_start(void)
{
    ACCESSLOCK;
    while (access_num > 0)
	ACCESSWAIT;
    INITLOCK;
    return 0;			/* just for symetry */
}
void OWLIB_can_finish_end(void)
{
    INITUNLOCK;
    ACCESSUNLOCK;
}
