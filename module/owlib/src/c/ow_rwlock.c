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

#if OW_MT

const char mutex_init_failed[] = "mutex_init failed rc=%d [%s]";
const char mutex_destroy_failed[] = "mutex_destroy failed rc=%d [%s]";
const char mutex_lock_failed[] = "mutex_lock failed rc=%d [%s]";
const char mutex_unlock_failed[] = "mutex_unlock failed rc=%d [%s]";
const char rwlock_init_failed[] = "rwlock_init failed rc=%d [%s]";
const char rwlock_read_lock[] = "rwlock_read_lock failed rc=%d [%s]";
const char rwlock_read_unlock[] = "rwlock_read_unlock failed rc=%d [%s]";
const char cond_timedwait_failed[] = "cond_timedwait failed rc=%d [%s]";
const char cond_signal_failed[] = "cond_signal failed rc=%d [%s]";
const char cond_wait_failed[] = "cond_wait failed rc=%d [%s]";

inline int my_pthread_cond_timedwait(pthread_cond_t *cond,
	pthread_mutex_t *mutex,
	const struct timespec *abstime)
{
	int rc = pthread_cond_timedwait(cond, mutex, abstime);
	if(rc != 0) {
		/* FILE and LINE will not show corrct value until it's a define */
		FATAL_ERROR(cond_timedwait_failed, rc, strerror(rc));
	}
	return rc;
}

inline int my_pthread_cond_wait(pthread_cond_t *cond,
	pthread_mutex_t *mutex)
{
	int rc = pthread_cond_wait(cond, mutex);
	if(rc != 0) {
		/* FILE and LINE will not show corrct value until it's a define */
		FATAL_ERROR(cond_wait_failed, rc, strerror(rc));
	}
	return rc;
}

inline int my_pthread_cond_signal(pthread_cond_t *cond)
{
	int rc = pthread_cond_signal(cond);
	if(rc != 0) {
		/* FILE and LINE will not show corrct value until it's a define */
		FATAL_ERROR(cond_signal_failed, rc, strerror(rc));
	}
	return rc;
}



void my_rwlock_init(my_rwlock_t * my_rwlock)
{
	int rc = pthread_mutex_init(&(my_rwlock->protect_reader_count), Mutex.pmattr);
	if(rc != 0) {
		/* FILE and LINE will not show corrct value until it's a define */
		FATAL_ERROR(mutex_init_failed, rc, strerror(rc));
	}
	sem_init(&(my_rwlock->allow_readers), 0, 1);
	sem_init(&(my_rwlock->no_processes), 0, 1);
	my_rwlock->reader_count = 1;
}

inline void my_rwlock_write_lock(my_rwlock_t * my_rwlock)
{
	sem_wait(&(my_rwlock->allow_readers));

	sem_wait(&(my_rwlock->no_processes));
}

inline void my_rwlock_write_unlock(my_rwlock_t * my_rwlock)
{
	sem_post(&(my_rwlock->allow_readers));

	sem_post(&(my_rwlock->no_processes));
}

inline void my_rwlock_read_lock(my_rwlock_t * my_rwlock)
{
	int rc;
	sem_wait(&(my_rwlock->allow_readers));
	sem_post(&(my_rwlock->allow_readers));

	rc = pthread_mutex_lock(&(my_rwlock->protect_reader_count));
	if(rc != 0) {
		/* FILE and LINE will not show corrct value until it's a define */
		FATAL_ERROR(mutex_lock_failed, rc, strerror(rc));
	}
	if (++my_rwlock->reader_count == 1) {
		sem_wait(&(my_rwlock->no_processes));
	}
	rc = pthread_mutex_unlock(&(my_rwlock->protect_reader_count));
	if(rc != 0) {
		FATAL_ERROR(mutex_unlock_failed, rc, strerror(rc));
	}
}

inline void my_rwlock_read_unlock(my_rwlock_t * my_rwlock)
{
	int rc = pthread_mutex_lock(&(my_rwlock->protect_reader_count));
	if(rc != 0) {
		/* FILE and LINE will not show corrct value until it's a define */
		FATAL_ERROR(mutex_lock_failed, rc, strerror(rc));
	}
	if (--my_rwlock->reader_count == 0) {
		sem_post(&(my_rwlock->no_processes));
	}
	rc = pthread_mutex_unlock(&(my_rwlock->protect_reader_count));
	if(rc != 0) {
		FATAL_ERROR(mutex_unlock_failed, rc, strerror(rc));
	}
}

void my_rwlock_destroy(my_rwlock_t * my_rwlock)
{
	int rc = pthread_mutex_destroy(&(my_rwlock->protect_reader_count));
	if(rc != 0) {
		/* FILE and LINE will not show corrct value until it's a define */
		FATAL_ERROR(mutex_destroy_failed, rc, strerror(rc));
	}
	sem_destroy(&(my_rwlock->allow_readers));
	sem_destroy(&(my_rwlock->no_processes));
}

#endif							/* OW_MT */
