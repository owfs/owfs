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

#if OW_MT
  #ifdef __UCLIBC__
    #if ((__UCLIBC_MAJOR__ << 16)+(__UCLIBC_MINOR__ << 8)+(__UCLIBC_SUBLEVEL__) < 0x00091D)
	/* If uClibc < 0.9.29, then re-initialize internal pthread-structs */
	extern char *__pthread_initial_thread_bos;
	void __pthread_initialize(void);
    #endif							/* UCLIBC_VERSION */
 #endif							/* __UCLIBC__ */
#endif							/* OW_MT */

struct mutexes Mutex = {
#if OW_MT
	.stat_mutex = PTHREAD_MUTEX_INITIALIZER,
	.controlflags_mutex = PTHREAD_MUTEX_INITIALIZER,
	.fstat_mutex = PTHREAD_MUTEX_INITIALIZER,
	.dir_mutex = PTHREAD_MUTEX_INITIALIZER,
  #ifdef __UCLIBC__
/* vsnprintf() doesn't seem to be thread-safe in uClibc
   even if thread-support is enabled. */
	.uclibc_mutex = PTHREAD_MUTEX_INITIALIZER,
  #endif							/* __UCLIBC__ */
  #if OW_USB
	.libusb_mutex = PTHREAD_MUTEX_INITIALIZER,
  #endif							/* OW_USB */
/* mutex attribute -- needed for uClibc programming */
/* we create at start, and destroy at end */
	.pmattr = NULL,
#endif							/* OW_MT */
};

/* Essentially sets up mutexes to protect global data/devices */
void LockSetup(void)
{
#if OW_MT
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

	/* global mutex attribute */
	my_pthread_mutexattr_init(&Mutex.mattr);
	my_pthread_mutexattr_settype(&Mutex.mattr, PTHREAD_MUTEX_ADAPTIVE_NP);
	Mutex.pmattr = &Mutex.mattr;
    #endif							/* UCLIBC_VERSION */
  #endif							/* __UCLIBC__ */

	my_pthread_mutex_init(&Mutex.stat_mutex, Mutex.pmattr);
	my_pthread_mutex_init(&Mutex.controlflags_mutex, Mutex.pmattr);
	my_pthread_mutex_init(&Mutex.fstat_mutex, Mutex.pmattr);
	my_pthread_mutex_init(&Mutex.dir_mutex, Mutex.pmattr);

	my_rwlock_init(&Mutex.lib);
	my_rwlock_init(&Mutex.cache);
	my_rwlock_init(&Mutex.store);
	my_rwlock_init(&Inbound_Control.lock);
  #ifdef __UCLIBC__
	my_pthread_mutex_init(&Mutex.uclibc_mutex, Mutex.pmattr);
  #endif							/* __UCLIBC__ */
  #if OW_USB
	my_pthread_mutex_init(&Mutex.libusb_mutex, Mutex.pmattr);
  #endif							/* OW_USB */
	/* This will give us 10 concurrent Handler threads, and 10 in queue due to listen() */
	sem_init(&Mutex.accept_sem, 0, Globals.concurrent_connections);
#endif							/* OW_MT */
}
