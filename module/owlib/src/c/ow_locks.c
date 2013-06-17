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

/* locks are to handle multithreading */

#include <config.h>
#include "owfs_config.h"
#include "ow.h"
#include "ow_counters.h"
#include "ow_connection.h"

/* ------- Globals ----------- */

  #ifdef __UCLIBC__
    #if ((__UCLIBC_MAJOR__ << 16)+(__UCLIBC_MINOR__ << 8)+(__UCLIBC_SUBLEVEL__) < 0x00091D)
	/* If uClibc < 0.9.29, then re-initialize internal pthread-structs */
	extern char *__pthread_initial_thread_bos;
	void __pthread_initialize(void);
    #endif							/* UCLIBC_VERSION */
 #endif							/* __UCLIBC__ */

// Global structure holding mutexes
struct mutexes Mutex ;

/* Essentially sets up mutexes to protect global data/devices */
void LockSetup(void)
{
	/* global mutex attribute */
	_MUTEX_ATTR_INIT(Mutex.mattr);
  #ifdef __UCLIBC__
    #if ((__UCLIBC_MAJOR__ << 16)+(__UCLIBC_MINOR__ << 8)+(__UCLIBC_SUBLEVEL__) < 0x00091D)
	/* If uClibc < 0.9.29, then re-initialize internal pthread-structs
	 * pthread and mutexes doesn't work after daemon() is called and
	 *   the main-process is gone.
	 *
	 * This workaround will probably be fixed in uClibc-0.9.28
	 * Other uClibc developers have noticed similar problems which are
	 * trigged when pthread functions are used in shared libraries. */
	__pthread_initial_thread_bos = NULL;
	__pthread_initialize();

	_MUTEX_ATTR_SET(Mutex.mattr, PTHREAD_MUTEX_ADAPTIVE_NP);
    #else /* UCLIBC_VERSION */
	_MUTEX_ATTR_SET(Mutex.mattr, PTHREAD_MUTEX_DEFAULT);
    #endif							/* UCLIBC_VERSION */
	_MUTEX_INIT(Mutex.uclibc_mutex);
  #else /* __UCLIBC__ */
	_MUTEX_ATTR_SET(Mutex.mattr, PTHREAD_MUTEX_DEFAULT);   
  #endif							/* __UCLIBC__ */

	_MUTEX_INIT(Mutex.stat_mutex);
	_MUTEX_INIT(Mutex.controlflags_mutex);
	_MUTEX_INIT(Mutex.fstat_mutex);
	_MUTEX_INIT(Mutex.dir_mutex);
	_MUTEX_INIT(Mutex.typedir_mutex);
	_MUTEX_INIT(Mutex.externaldir_mutex);
	_MUTEX_INIT(Mutex.namefind_mutex);
	_MUTEX_INIT(Mutex.aliasfind_mutex);
	_MUTEX_INIT(Mutex.externalcount_mutex);
	_MUTEX_INIT(Mutex.timegm_mutex);
	_MUTEX_INIT(Mutex.detail_mutex);

	RWLOCK_INIT(Mutex.lib);
	RWLOCK_INIT(Mutex.cache);
	RWLOCK_INIT(Mutex.persistent_cache);
	RWLOCK_INIT(Inbound_Control.lock);
	RWLOCK_INIT(Inbound_Control.monitor_lock);
  #if OW_USB
	_MUTEX_INIT(Mutex.libusb_mutex);
  #endif							/* OW_USB */
}
