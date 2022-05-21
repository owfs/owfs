#include "ow_testhelper.h"

// Global "owq" which, if used, is destroyed in teardown
struct one_wire_query* owq;

static void LockTeardown();

/**
 * Setup a basic owlib stack. Should be setup via
 *
 * 		tcase_add_checked_fixture(tc, owlib_test_setup, owlib_test_teardown);
 *
 */
void owlib_test_setup(void) {
	LockSetup();
	Cache_Open();
	Detail_Init();
	DeviceSort();
	SetLocalControlFlags() ; // reset by every option and other change.

	owq = NULL;
}

void owlib_test_teardown(void) {
	if(owq) {
		OWQ_destroy(owq);
		owq = NULL;
	}
	LockTeardown();
	Detail_Close();
}

static void LockTeardown() {
	/* global mutex attribute */
	_MUTEX_ATTR_DESTROY(Mutex.mattr);

	_MUTEX_DESTROY(Mutex.stat_mutex);
	_MUTEX_DESTROY(Mutex.controlflags_mutex);
	_MUTEX_DESTROY(Mutex.fstat_mutex);
	_MUTEX_DESTROY(Mutex.dir_mutex);
#if OW_USB
	_MUTEX_DESTROY(Mutex.libusb_mutex);
#endif							/* OW_USB */
	_MUTEX_DESTROY(Mutex.typedir_mutex);
	_MUTEX_DESTROY(Mutex.externaldir_mutex);
	_MUTEX_DESTROY(Mutex.namefind_mutex);
	_MUTEX_DESTROY(Mutex.aliaslist_mutex);
	_MUTEX_DESTROY(Mutex.externalcount_mutex);
	_MUTEX_DESTROY(Mutex.timegm_mutex);
	_MUTEX_DESTROY(Mutex.detail_mutex);

	RWLOCK_DESTROY(Mutex.lib);
	RWLOCK_DESTROY(Mutex.cache);
	RWLOCK_DESTROY(Mutex.persistent_cache);
	RWLOCK_DESTROY(Mutex.connin);
	RWLOCK_DESTROY(Mutex.monitor);
}
