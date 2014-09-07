/* From Paul Alfille */
/* Implementation of Reader/Writer locks using semaphores and mutexes */

/* Note, these are relatively light-weight rwlocks,
    only solve writer starvation and no timeout or
    queued wakeup */

/* From: Communications of the ACM :Concurrent Control with "Readers" and "Writers" P.J. Courtois,* F. H, 1971 */

#ifndef RWLOCK_H
#define RWLOCK_H

#include "owfs_config.h"

#include <pthread.h>
#include "sem.h"

typedef struct {
	pthread_mutex_t protect_reader; // mutex 3 in article
	pthread_mutex_t protect_writer; // mutex 2 in article
	pthread_mutex_t protect_reader_count; // mutex 2 in article
	int readcount;
	int writecount;
	sem_t allow_readers; // 'r'
	sem_t no_processes; // 'w'
} my_rwlock_t;

void my_rwlock_init(my_rwlock_t * rwlock);
void my_rwlock_write_lock(my_rwlock_t * rwlock);
void my_rwlock_write_unlock(my_rwlock_t * rwlock);
void my_rwlock_read_lock(my_rwlock_t * rwlock);
void my_rwlock_read_unlock(my_rwlock_t * rwlock);
void my_rwlock_destroy(my_rwlock_t * rwlock);

#endif							/* RWLOCK */
