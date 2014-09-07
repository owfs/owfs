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

void my_rwlock_init(my_rwlock_t * my_rwlock)
{
	_MUTEX_INIT(my_rwlock->protect_reader); // mutex 3 in article
	_MUTEX_INIT(my_rwlock->protect_writer); // mutex 2 in article
	_MUTEX_INIT(my_rwlock->protect_reader_count); // mutex 1 in article
	_SEM_INIT(  my_rwlock->allow_readers, 0, 1); // r in article
	_SEM_INIT(  my_rwlock->allow_writers , 0, 1); // w in article
	my_rwlock->readcount = 0;
	my_rwlock->writecount = 0;
}

inline void my_rwlock_write_lock(my_rwlock_t * my_rwlock)
{
	_MUTEX_LOCK(my_rwlock->protect_writer);
		++my_rwlock->writecount ;
		if ( my_rwlock->writecount == 1 ) {
			sem_wait(&(my_rwlock->allow_readers));
		}
	_MUTEX_UNLOCK(my_rwlock->protect_writer);

	sem_wait(&(my_rwlock->allow_writers));
}

inline void my_rwlock_write_unlock(my_rwlock_t * my_rwlock)
{
	sem_post(&(my_rwlock->allow_writers));

	_MUTEX_LOCK(my_rwlock->protect_writer);
		--my_rwlock->writecount ;
		if ( my_rwlock->writecount == 0 ) {
			sem_post(&(my_rwlock->allow_readers));
		}
	_MUTEX_UNLOCK(my_rwlock->protect_writer);

}

inline void my_rwlock_read_lock(my_rwlock_t * my_rwlock)
{
	_MUTEX_LOCK(my_rwlock->protect_reader);
		sem_wait(&(my_rwlock->allow_readers));

		_MUTEX_LOCK(my_rwlock->protect_reader_count);
			++my_rwlock->readcount ;
			if ( my_rwlock->readcount == 1 ) {
				sem_wait(&(my_rwlock->allow_writers));
			}
		_MUTEX_UNLOCK(my_rwlock->protect_reader_count);
		
		sem_post(&(my_rwlock->allow_readers));
	_MUTEX_UNLOCK(my_rwlock->protect_reader);
}

inline void my_rwlock_read_unlock(my_rwlock_t * my_rwlock)
{
	_MUTEX_LOCK(my_rwlock->protect_reader_count);
		--my_rwlock->readcount ;
		if ( my_rwlock->readcount == 0 ) {
			sem_post(&(my_rwlock->allow_writers));
		}
	_MUTEX_UNLOCK(my_rwlock->protect_reader_count);
}

void my_rwlock_destroy(my_rwlock_t * my_rwlock)
{
	_MUTEX_DESTROY(my_rwlock->protect_reader_count);
	_MUTEX_DESTROY(my_rwlock->protect_reader);
	_MUTEX_DESTROY(my_rwlock->protect_writer);
	sem_destroy(&(my_rwlock->allow_readers));
	sem_destroy(&(my_rwlock->allow_writers));
}
