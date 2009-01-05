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
	my_pthread_mutex_init(&(my_rwlock->protect_reader_count), Mutex.pmattr);
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
	sem_wait(&(my_rwlock->allow_readers));
	sem_post(&(my_rwlock->allow_readers));

	my_pthread_mutex_lock(&(my_rwlock->protect_reader_count));
	if (++my_rwlock->reader_count == 1) {
		sem_wait(&(my_rwlock->no_processes));
	}
	my_pthread_mutex_unlock(&(my_rwlock->protect_reader_count));
}

inline void my_rwlock_read_unlock(my_rwlock_t * my_rwlock)
{
	my_pthread_mutex_lock(&(my_rwlock->protect_reader_count));
	if (--my_rwlock->reader_count == 0) {
		sem_post(&(my_rwlock->no_processes));
	}
	my_pthread_mutex_unlock(&(my_rwlock->protect_reader_count));
}

void my_rwlock_destroy(my_rwlock_t * my_rwlock)
{
	my_pthread_mutex_destroy(&(my_rwlock->protect_reader_count));
	sem_destroy(&(my_rwlock->allow_readers));
	sem_destroy(&(my_rwlock->no_processes));
}

#endif							/* OW_MT */
