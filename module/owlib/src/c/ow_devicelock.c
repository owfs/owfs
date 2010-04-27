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

#include <config.h>
#include "owfs_config.h"
#include "ow.h"
//#include "ow_counters.h"
#include "ow_connection.h"

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
	#define DEVTREE_LOCK(pn)           MUTEX_LOCK(   ((pn)->selected_connection)->dev_mutex )
	#define DEVTREE_UNLOCK(pn)         MUTEX_UNLOCK( ((pn)->selected_connection)->dev_mutex )
#endif							/* OW_MT */

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
ZERO_OR_ERROR DeviceLockGet(struct parsedname *pn)
{
#if OW_MT
	struct devlock *local_devicelock;
	struct devlock *tree_devicelock;
	struct dev_opaque *opaque;

	if (pn->selected_device == DeviceSimultaneous) {
		/* Shouldn't call DeviceLockGet() on DeviceSimultaneous. No sn exists */
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
		MUTEX_INIT(tree_devicelock->lock);	// create a mutex
		tree_devicelock->users = 0 ;
	} else {					// existing device slot
		owfree(local_devicelock); // kill the locally allocated devlock (since there already is a matching devlock)	
	}
	++(tree_devicelock->users); // add our claim to the device
	DEVTREE_UNLOCK(pn);
	MUTEX_LOCK(tree_devicelock->lock);	// now grab the device
	pn->lock = tree_devicelock; // use this new devlock
#else							/* OW_MT */
	(void) pn;					// suppress compiler warning in the trivial case.
#endif							/* OW_MT */
	return 0;
}

// Unlock the device
void DeviceLockRelease(struct parsedname *pn)
{
#if OW_MT
	if (pn->lock) { // this is the stored pointer to the device in the appropriate device tree
		// Free the device
		MUTEX_UNLOCK(pn->lock->lock);		/* Serg: This coredump on his 64-bit server */

		// Now mark our disinterest in the device tree (and possibly reap the node))
		DEVTREE_LOCK(pn);
		--pn->lock->users; // remove our interest
		if (pn->lock->users == 0) {
			// No body's interested!
			tdelete(pn->lock, &(pn->selected_connection->dev_db), dev_compare); /* Serg: Address 0x5A0D750 is 0 bytes inside a block of size 32 free'd */
			MUTEX_DESTROY(pn->lock->lock);
			owfree(pn->lock);
		}
		pn->lock = NULL;
		DEVTREE_UNLOCK(pn);
	}
#else							/* OW_MT */
	(void) pn;					// suppress compiler warning in the trivial case.
#endif							/* OW_MT */
}
