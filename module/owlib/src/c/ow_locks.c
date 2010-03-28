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

// dynamically created access control for a 1-wire device
// used to negotiate between different threads (queries)
struct devlock {
#if OW_MT
	pthread_mutex_t lock;
#endif							/* OW_MT */
	BYTE sn[SERIAL_NUMBER_SIZE];
	UINT users;
};


/*
We keep a finite number of devlocks, organized in a tree for faster searching.
The have a mutex and a counter to negotiate between threads and total lifetime.
*/
#if OW_MT
	#define DEVTREE_LOCK(pn)           my_pthread_mutex_lock( &(((pn)->selected_connection)->dev_mutex) )
	#define DEVTREE_UNLOCK(pn)         my_pthread_mutex_unlock( &(((pn)->selected_connection)->dev_mutex) )
#endif							/* OW_MT */

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

/* Bad bad C library */
/* implementation of tfind, tsearch returns an opaque structure */
/* you have to know that the first element is a pointer to your data */

/* This is a tree element for the devlocks */
struct dev_opaque {
	struct devlock *key;
	void *other;
};

#if OW_MT
	/* compilation error in gcc version 4.0.0 20050519 if dev_compare
	 * is defined as an embedded function
	 */
	/* Use the serial numbers to find the right devlock */
	static int dev_compare(const void *a, const void *b)
	{
		return memcmp(&((const struct devlock *) a)->sn, &((const struct devlock *) b)->sn, 8);
	}
#endif


/* Grabs a device lock, either one already matching, or creates one */
/* called per-adapter */
/* The device locks (devlock) are kept in a tree */
ZERO_OR_ERROR LockGet(struct parsedname *pn)
{
#if OW_MT
	struct devlock *local_devicelock;
	struct devlock *tree_devicelock;
	struct dev_opaque *opaque;

	//printf("LockGet() pn->path=%s\n", pn->path);
	if (pn->selected_device == DeviceSimultaneous) {
		/* Shouldn't call LockGet() on DeviceSimultaneous. No sn exists */
		return 0;
	}

	/* pn->selected_connection is null when "cat /tmp/1wire/system/adapter/pulldownslewrate.0"
	 * and you have owfs -s & owserver -u started. */
	// need to check to see if this is still relevant
	if (pn->selected_connection == NULL) {
		if ((pn->type == ePN_settings) || (pn->type == ePN_system)) {
			/* Probably trying to read/write some adapter settings which
			 * is not available for remote connections... only local adapters. */
			return -ENOTSUP;
		}
		/* Cannot lock without knowing which bus since the device trees are bus-specific */
		return -EINVAL ;
	}

	/* Need locking? */
	switch (pn->selected_filetype->format) {
		case ft_directory:
		case ft_subdir:
			return 0;
		default:
			break;
	}

	switch (pn->selected_filetype->change) {
		case fc_static:
		case fc_Astable:
		case fc_Avolatile:
		case fc_statistic:
			return 0;
		default:
			break;
	}

	// Create a devlock block to add to the tree
	local_devicelock = owmalloc(sizeof(struct devlock)) ;
	if ( local_devicelock == NULL ) {
		return -ENOMEM;
	}
	memcpy(local_devicelock->sn, pn->sn, 8);

	DEVTREE_LOCK(pn);
	/* in->dev_db points to the root of a tree of queries that are using this device */
	opaque = (struct dev_opaque *)tsearch(local_devicelock, &(pn->selected_connection->dev_db), dev_compare) ;
	if ( opaque == NULL ) {	// unfound and uncreatable
		DEVTREE_UNLOCK(pn);
		owfree(local_devicelock); // kill the allocated devlock
		return -ENOMEM;
	}
	
	tree_devicelock = opaque->key ;
	if ( local_devicelock == tree_devicelock) {	// new device slot
		// No longer "local" -- the local_device lock now belongs to the device_tree
		// It will need to be freed later, when the user count returns to zero.
		my_pthread_mutex_init(&(tree_devicelock->lock), Mutex.pmattr);	// create a mutex
		tree_devicelock->users = 0 ;
	} else {					// existing device slot
		owfree(local_devicelock); // kill the locally allocated devlock (since there already is a matching devlock)	
	}
	++(tree_devicelock->users); // add our claim to the device
	DEVTREE_UNLOCK(pn);
	my_pthread_mutex_lock(&(tree_devicelock->lock));	// now grab the device
	pn->lock = tree_devicelock; // use this new devlock
#else							/* OW_MT */
	(void) pn;					// suppress compiler warning in the trivial case.
#endif							/* OW_MT */
	return 0;
}

// The device should be unlocked
void LockRelease(struct parsedname *pn)
{
#if OW_MT
	if (pn->lock) { // this is the stored pointer to the device in the appropriate device tree
		// Free the device
		my_pthread_mutex_unlock(&(pn->lock->lock));		/* Serg: This coredump on his 64-bit server */

		// Now mark our disinterest in the device tree (and possibly reap the node))
		DEVTREE_LOCK(pn);
		--pn->lock->users; // remove our interest
		if (pn->lock->users == 0) {
			// No body's interested!
			tdelete(pn->lock, &(pn->selected_connection->dev_db), dev_compare); /* Serg: Address 0x5A0D750 is 0 bytes inside a block of size 32 free'd */
			my_pthread_mutex_destroy(&(pn->lock->lock));
			owfree(pn->lock);
		}
		pn->lock = NULL;
		DEVTREE_UNLOCK(pn);
	}
#else							/* OW_MT */
	(void) pn;					// suppress compiler warning in the trivial case.
#endif							/* OW_MT */
}

void BUS_lock(const struct parsedname *pn)
{
	if (pn) {
		BUS_lock_in(pn->selected_connection);
	}
}

void BUS_unlock(const struct parsedname *pn)
{
	if (pn) {
		BUS_unlock_in(pn->selected_connection);
	}
}

void BUS_lock_in(struct connection_in *in)
{
	if (!in) {
		return;
	}
#if OW_MT
	my_pthread_mutex_lock(&(in->bus_mutex));
	if (in->busmode == bus_i2c && in->connin.i2c.channels > 1) {
		my_pthread_mutex_lock(&(in->connin.i2c.head->connin.i2c.i2c_mutex));
	}
#endif							/* OW_MT */
	gettimeofday(&(in->last_lock), NULL);	/* for statistics */
	STAT_ADD1_BUS(e_bus_locks, in);
}

void BUS_unlock_in(struct connection_in *in)
{
	struct timeval *t;
	long sec, usec;
	if (!in) {
		return;
	}

	gettimeofday(&(in->last_unlock), NULL);

	/* avoid update if system-clock have changed */
	STATLOCK;
	sec = in->last_unlock.tv_sec - in->last_lock.tv_sec;
	if ((sec >= 0) && (sec < 60)) {
		usec = in->last_unlock.tv_usec - in->last_lock.tv_usec;
		total_bus_time.tv_sec += sec;
		total_bus_time.tv_usec += usec;
		if (total_bus_time.tv_usec >= 1000000) {
			total_bus_time.tv_usec -= 1000000;
			++total_bus_time.tv_sec;
		} else if (total_bus_time.tv_usec < 0) {
			total_bus_time.tv_usec += 1000000;
			--total_bus_time.tv_sec;
		}

		t = &in->bus_time;
		t->tv_sec += sec;
		t->tv_usec += usec;
		if (t->tv_usec >= 1000000) {
			t->tv_usec -= 1000000;
			++t->tv_sec;
		} else if (t->tv_usec < 0) {
			t->tv_usec += 1000000;
			--t->tv_sec;
		}
	}
	++in->bus_stat[e_bus_unlocks];
	STATUNLOCK;
#if OW_MT
	if (in->busmode == bus_i2c && in->connin.i2c.channels > 1) {
		my_pthread_mutex_unlock(&(in->connin.i2c.head->connin.i2c.i2c_mutex));
	}
	my_pthread_mutex_unlock(&(in->bus_mutex));
#endif							/* OW_MT */
}
