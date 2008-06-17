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

/* ------- Globalss ----------- */

#ifdef OW_MT

void rwlock_init( rwlock_t * rwlock )
{
    pthread_mutex_init( &(rwlock->protect_reader_count), Mutex.pmattr ) ;
    sem_init( &(rwlock->allow_readers), 0, 1 ) ;
    sem_init( &(rwlock->no_processes), 0, 1 ) ;
    rwlock->reader_count = 1 ;
}

inline void rwlock_write_lock( rwlock_t * rwlock )
{
    sem_wait(&(rwlock->allow_readers)) ;

    sem_wait(&(rwlock->no_processes)) ;
}

inline void rwlock_write_unlock( rwlock_t * rwlock )
{
    sem_post(&(rwlock->allow_readers)) ;

    sem_post(&(rwlock->no_processes)) ;
}

inline void rwlock_read_lock( rwlock_t * rwlock )
{
    sem_wait(&(rwlock->allow_readers)) ;
    sem_post(&(rwlock->allow_readers)) ;

    pthread_mutex_lock( &(rwlock->protect_reader_count) ) ;
    if ( ++rwlock->reader_count == 1 ) {
        sem_wait(&(rwlock->no_processes)) ;
    }
    pthread_mutex_unlock( &(rwlock->protect_reader_count) ) ;
}

inline void rwlock_read_unlock( rwlock_t * rwlock )
{
    pthread_mutex_lock( &(rwlock->protect_reader_count) ) ;
    if ( --rwlock->reader_count == 0 ) {
        sem_post(&(rwlock->no_processes)) ;
    }
    pthread_mutex_unlock( &(rwlock->protect_reader_count) ) ;
}

void rwlock_destroy( rwlock_t * rwlock )
{
    pthread_mutex_destroy( &(rwlock->protect_reader_count) ) ;
    sem_destroy( &(rwlock->allow_readers)) ;
    sem_destroy( &(rwlock->no_processes)) ;
}

#endif /* OW_MT */
