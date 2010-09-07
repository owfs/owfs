/* From Paul Alfille */
/* $Id$ */
/* Implementation of Reader/Writer locks using semaphores and mutexes */
/* based on "The Little Book of Semaphores" bu Allen B. Downey */
/* http://www.greenteapress.com */

/* Note, these are relatively light-weight rwlocks,
    only solve writer starvation and no timeout or
    queued wakeup */

#ifndef RWLOCK_H
#define RWLOCK_H

#include "owfs_config.h"

#if OW_MT

#include <pthread.h>
#include "sem.h"

typedef struct {
	pthread_mutex_t protect_reader_count;
	int reader_count;
	sem_t allow_readers;
	sem_t no_processes;
} my_rwlock_t;

void my_rwlock_init(my_rwlock_t * rwlock);
inline void my_rwlock_write_lock(my_rwlock_t * rwlock);
inline void my_rwlock_write_unlock(my_rwlock_t * rwlock);
inline void my_rwlock_read_lock(my_rwlock_t * rwlock);
inline void my_rwlock_read_unlock(my_rwlock_t * rwlock);
void my_rwlock_destroy(my_rwlock_t * rwlock);

#else /* not OW_MT */

typedef int my_rwlock_t ;

#endif							/* OW_MT */

#endif							/* RWLOCK */
