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

/* locks are to handle multithreading */

#include <config.h>
#include "owfs_config.h"
#include "ow.h"
#include "ow_connection.h"

/* ------- Globalss ----------- */

#ifdef __UCLIBC__
#if ((__UCLIBC_MAJOR__ << 16)+(__UCLIBC_MINOR__ << 8)+(__UCLIBC_SUBLEVEL__) < 0x00091D)
/* If uClibc < 0.9.29, then re-initialize internal pthread-structs */
extern char *__pthread_initial_thread_bos;
void __pthread_initialize(void);
#endif							/* UCLIBC_VERSION */
#endif							/* __UCLIBC */

struct mutexes Mutex = {
#ifdef __UCLIBC__
/* vsnprintf() doesn't seem to be thread-safe in uClibc
   even if thread-support is enabled. */
	.uclibc_mutex = PTHREAD_MUTEX_INITIALIZER,
#endif							/* __UCLIBC__ */
/* mutex attribute -- needed for uClibc programming */
/* we create at start, and destroy at end */
	.pmattr = NULL,
};

/* Essentially sets up mutexes to protect global data/devices */
void LockSetup(void)
{
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

#endif							/* UCLIBC_VERSION */
	/* global mutex attribute */
	my_pthread_mutexattr_init(&Mutex.mattr);
	my_pthread_mutexattr_settype(&Mutex.mattr, PTHREAD_MUTEX_ADAPTIVE_NP);
	Mutex.pmattr = &Mutex.mattr;
#endif							/* __UCLIBC */

	my_rwlock_init(&Mutex.lib);
	my_rwlock_init(&Mutex.connin);
#ifdef __UCLIBC__
	my_pthread_mutex_init(&Mutex.uclibc_mutex, Mutex.pmattr);
#endif							/* UCLIBC */
}

void BUS_lock_in(struct connection_in *in)
{
	//printf("BUS_lock_in: in=%p\n", in);
	if (in) {
		my_pthread_mutex_lock(&(in->bus_mutex));
	}
}

void BUS_unlock_in(struct connection_in *in)
{
	//printf("BUS_unlock_in: in=%p\n", in);
	if (in) {
		my_pthread_mutex_unlock(&(in->bus_mutex));
	}
}
