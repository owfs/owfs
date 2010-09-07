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
#include "ow_connection.h"

# if OW_MT

// dynamically created access control for a 1-wire device
// used to negotiate between different threads (queries)
struct devlock {
	pthread_mutex_t lock;
	BYTE sn[SERIAL_NUMBER_SIZE];
	UINT users;
};


/*
We keep all the devlocks, organized in a tree for faster searching.
*/
#define DEVTREE_LOCK(pn)           _MUTEX_LOCK(   ((pn)->selected_connection)->dev_mutex )
#define DEVTREE_UNLOCK(pn)         _MUTEX_UNLOCK( ((pn)->selected_connection)->dev_mutex )

/* Bad bad C library */
/* implementation of tfind, tsearch returns an opaque structure */
/* you have to know that the first element is a pointer to your data */

/* This is a tree element for the devlocks */
struct dev_opaque {
	struct devlock *key;
	void *other;
};

/* compilation error in gcc version 4.0.0 20050519 if dev_compare
 * is defined as an embedded function
 */
/* Use the serial numbers to find the right devlock */
static int dev_compare(const void *a, const void *b)
{
	return memcmp(&((const struct devlock *) a)->sn, &((const struct devlock *) b)->sn, SERIAL_NUMBER_SIZE);
}

/* Grabs a device lock, either one already matching, or creates one */
/* called per-adapter */
/* The device locks (devlock) are kept in a tree */
ZERO_OR_ERROR DeviceLockGet(struct parsedname *pn)
{
	struct devlock *local_devicelock;
	struct devlock *tree_devicelock;
	struct dev_opaque *opaque;

	if (pn->selected_device == DeviceSimultaneous) {
		/* Shouldn't call DeviceLockGet() on DeviceSimultaneous. No sn exists */
		return 0;
	}

	/* Cannot lock without knowing which bus since the device trees are bus-specific */
	if (pn->selected_connection == NULL) {
		return -EINVAL ;
	}

	/* Need locking? */
	// Test type
	switch (pn->selected_filetype->format) {
		case ft_directory:
		case ft_subdir:
			return 0;
		default:
			break;
	}

	// Ignore static and atomic
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
	memcpy(local_devicelock->sn, pn->sn, SERIAL_NUMBER_SIZE);

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
		_MUTEX_INIT(tree_devicelock->lock);	// create a mutex
		tree_devicelock->users = 0 ;
	} else {					// existing device slot
		owfree(local_devicelock); // kill the locally allocated devlock (since there already is a matching devlock)	
	}
	++(tree_devicelock->users); // add our claim to the device
	DEVTREE_UNLOCK(pn);
	_MUTEX_LOCK(tree_devicelock->lock);	// now grab the device
	pn->lock = tree_devicelock; // use this new devlock
	return 0;
}

// Unlock the device
void DeviceLockRelease(struct parsedname *pn)
{
	if (pn->lock) { // this is the stored pointer to the device in the appropriate device tree
		// Free the device
		_MUTEX_UNLOCK(pn->lock->lock);		/* Serg: This coredump on his 64-bit server */

		// Now mark our disinterest in the device tree (and possibly reap the node))
		DEVTREE_LOCK(pn);
		--pn->lock->users; // remove our interest
		if (pn->lock->users == 0) {
			// Nobody's interested!
			tdelete(pn->lock, &(pn->selected_connection->dev_db), dev_compare); /* Serg: Address 0x5A0D750 is 0 bytes inside a block of size 32 free'd */
			_MUTEX_DESTROY(pn->lock->lock);
			owfree(pn->lock);
		}
		DEVTREE_UNLOCK(pn);
		pn->lock = NULL;
	}
}

#else							/* not OW_MT */

ZERO_OR_ERROR DeviceLockGet(struct parsedname *pn)
{
	(void) pn;					// suppress compiler warning in the trivial case.
	return 0 ;
}

void DeviceLockRelease(struct parsedname *pn)
{
	(void) pn;					// suppress compiler warning in the trivial case.
}

#endif							/* OW_MT */
