/*
    OWFS -- One-Wire filesystem
    OWHTTPD -- One-Wire Web Server
    Written 2003 Paul H Alfille
	email: paul.alfille@gmail.com
	Released under the GPL
	See the header file: ow.h for full attribution
	1wire/iButton system from Dallas Semiconductor
*/

/* locks are to handle multithreading */
/* From: Communications of the ACM :Concurrent Control with "Readers" and "Writers" P.J. Courtois,* F. H, 1971 */

#include <config.h>
#include "owfs_config.h"
#include "ow.h"

#define LOCK_DEBUG(...)  if ( Globals.locks != 0 ) { LEVEL_DEFAULT(__VA_ARGS__) ; }

#if PTHREAD_RWLOCK

#ifndef EXTENDED_RWLOCK_DEBUG
void my_rwlock_init(my_rwlock_t * rwlock)
{
	int semrc;
	semrc = pthread_rwlock_init(rwlock, NULL);
	if(semrc != 0) {
		LOCK_DEBUG("semrc=%d [%s] RWLOCK INIT", semrc, strerror(errno));
		debug_crash();
	}
}

void my_rwlock_destroy(my_rwlock_t * rwlock)
{
	int semrc;
	semrc = pthread_rwlock_destroy(rwlock);
	if(semrc != 0) {
		/* Might return EBUSY */
		LOCK_DEBUG("semrc=%d [%s] RWLOCK DESTROY", semrc, strerror(errno));
		debug_crash();
	}
}

int my_rwlock_write_lock(my_rwlock_t * rwlock)
{
	int semrc;
	semrc = pthread_rwlock_wrlock(rwlock);
	if(semrc != 0) {
		LOCK_DEBUG("semrc=%d [%s] RWLOCK WLOCK", semrc, strerror(errno));
		debug_crash();
	}
	return semrc;
}

int my_rwlock_write_unlock(my_rwlock_t * rwlock)
{
	int semrc;
	semrc = pthread_rwlock_unlock(rwlock);
	if(semrc != 0) {
		LEVEL_DEFAULT("semrc=%d [%s] RWLOCK WUNLOCK", semrc, strerror(errno));
		debug_crash();
	}
	return semrc;
}

int my_rwlock_read_lock(my_rwlock_t * rwlock)
{
	int semrc;
	semrc = pthread_rwlock_rdlock(rwlock);
	if(semrc != 0) {
		LOCK_DEBUG("semrc=%d [%s] RWLOCK RLOCK", semrc, strerror(errno));
		debug_crash();
	}
	return semrc;
}

int my_rwlock_read_unlock(my_rwlock_t * rwlock)
{
	int semrc;
	semrc = pthread_rwlock_unlock(rwlock);
	if(semrc != 0) {
		LOCK_DEBUG("semrc=%d [%s] RWLOCK RUNLOCK", semrc, strerror(errno));
		debug_crash();
	}
	return semrc;
}
#endif

#else

#ifndef EXTENDED_RWLOCK_DEBUG
/* rwlock-implementation with semaphores */
/* Don't use the builtin rwlock-support in pthread */
void my_rwlock_init(my_rwlock_t * my_rwlock)
{
	int semrc = 0;
	memset(my_rwlock, 0, sizeof(my_rwlock_t));
	_MUTEX_INIT(my_rwlock->protect_reader_count);
	semrc |= sem_init(&(my_rwlock->allow_readers), 0, 1);
	if(semrc != 0) {
		LOCK_DEBUG("rc=%d [%s] RWLOCK INIT1", semrc, strerror(errno));
		debug_crash();
	}
	semrc |= sem_init(&(my_rwlock->no_processes), 0, 1);
	if(semrc != 0) {
		LOCK_DEBUG("rc=%d [%s] RWLOCK INIT2", semrc, strerror(errno));
		debug_crash();
	}
	my_rwlock->reader_count = 0;
}

void my_rwlock_destroy(my_rwlock_t * my_rwlock)
{
	int semrc = 0;
	_MUTEX_DESTROY(my_rwlock->protect_reader_count);
	semrc |= sem_destroy(&(my_rwlock->allow_readers));
	if(semrc != 0) {
		LOCK_DEBUG("rc=%d [%s] RWLOCK DESTROY1", semrc, strerror(errno));
		debug_crash();
	}
	semrc |= sem_destroy(&(my_rwlock->no_processes));
	if(semrc != 0) {
		LOCK_DEBUG("rc=%d [%s] RWLOCK DESTROY2", semrc, strerror(errno));
		debug_crash();
	}
	memset(my_rwlock, 0, sizeof(my_rwlock_t));
}

int my_rwlock_write_lock(my_rwlock_t * my_rwlock)
{
	sem_wait(&(my_rwlock->allow_readers));

	sem_wait(&(my_rwlock->no_processes));
	return 0;
}

int my_rwlock_write_unlock(my_rwlock_t * my_rwlock)
{
	sem_post(&(my_rwlock->allow_readers));

	sem_post(&(my_rwlock->no_processes));
	return 0;
}

int my_rwlock_read_lock(my_rwlock_t * my_rwlock)
{
	sem_wait(&(my_rwlock->allow_readers));

	_MUTEX_LOCK(my_rwlock->protect_reader_count);
	if (++my_rwlock->reader_count == 1) {
		sem_wait(&(my_rwlock->no_processes));
	}
	_MUTEX_UNLOCK(my_rwlock->protect_reader_count);

	sem_post(&(my_rwlock->allow_readers));
	return 0;
}

int my_rwlock_read_unlock(my_rwlock_t * my_rwlock)
{
	_MUTEX_LOCK(my_rwlock->protect_reader_count);
	if (--my_rwlock->reader_count == 0) {
		sem_post(&(my_rwlock->no_processes));
	}
	if(my_rwlock->reader_count < 0) {
		LOCK_DEBUG("RWLOCK RUNLOCK");
		debug_crash();
	}
	_MUTEX_UNLOCK(my_rwlock->protect_reader_count);
	return 0;
}

#endif /* EXTENDED_RWLOCK_DEBUG */

#endif /* PTHREAD_RWLOCK */
