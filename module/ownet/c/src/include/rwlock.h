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

#ifdef OW_MT

#include <pthread.h>
#ifdef HAVE_SEMAPHORE_H
#include <semaphore.h>
#endif

typedef struct {
    pthread_mutex_t protect_reader_count;
    int reader_count ;
    sem_t allow_readers ;
    sem_t no_processes ;
} rwlock_t ;

void rwlock_init( rwlock_t * rwlock ) ;
inline void rwlock_write_lock( rwlock_t * rwlock ) ;
inline void rwlock_write_unlock( rwlock_t * rwlock ) ;
inline void rwlock_read_lock( rwlock_t * rwlock ) ;
inline void rwlock_read_unlock( rwlock_t * rwlock ) ;
void rwlock_destroy( rwlock_t * rwlock ) ;

#endif /* OW_MT */
        
#endif							/* RWLOCK */
