/* From Paul Alfille */
/* Implementation of Reader/Writer locks using semaphores and mutexes */

/* Note, these are relatively light-weight rwlocks,
   only solve writer starvation and no timeout or
   queued wakeup */

#ifndef RWLOCK_H
#define RWLOCK_H

#include "ow_stateinfo.h"
#include "ow_debug.h" // debug_crash()

#include <pthread.h>

#if PTHREAD_RWLOCK
typedef pthread_rwlock_t my_rwlock_t;
#else
#include "sem.h"

typedef struct {
	pthread_mutex_t protect_reader_count;
	int reader_count;
	sem_t allow_readers;
	sem_t no_processes;
} my_rwlock_t;
#endif

#ifndef EXTENDED_RWLOCK_DEBUG
void my_rwlock_init(my_rwlock_t * rwlock);
void my_rwlock_destroy(my_rwlock_t * rwlock);
int my_rwlock_write_lock(my_rwlock_t * rwlock);
int my_rwlock_write_unlock(my_rwlock_t * rwlock);
int my_rwlock_read_lock(my_rwlock_t * rwlock);
int my_rwlock_read_unlock(my_rwlock_t * rwlock);
#endif /* EXTENDED_RWLOCK_DEBUG */


#if PTHREAD_RWLOCK

#ifdef EXTENDED_RWLOCK_DEBUG
#define my_rwlock_init(rwlock)											\
	{																	\
		int semrc;														\
		semrc = pthread_rwlock_init(rwlock, NULL);						\
		if(semrc != 0) {												\
			LEVEL_DEFAULT("semrc=%d [%s]\n", semrc, strerror(errno));	\
			debug_crash();												\
		}																\
	}
	
#define my_rwlock_destroy(rwlock)										\
	{																	\
		int semrc;														\
		semrc = pthread_rwlock_destroy(rwlock);							\
		if(semrc != 0) {												\
			LEVEL_DEFAULT("semrc=%d [%s]\n", semrc, strerror(errno));	\
			debug_crash();												\
		}																\
	}

#define my_rwlock_write_lock(rwlock)									\
	{																	\
		int semrc;														\
		semrc = pthread_rwlock_wrlock(rwlock);							\
		if(semrc != 0) {												\
			LEVEL_DEFAULT("semrc=%d [%s]\n", semrc, strerror(errno));	\
			debug_crash();												\
		}																\
	}

#define my_rwlock_write_unlock(rwlock)									\
	{																	\
		int semrc;														\
		semrc = pthread_rwlock_unlock(rwlock);							\
		if(semrc != 0) {												\
			LEVEL_DEFAULT("semrc=%d [%s]\n", semrc, strerror(errno));	\
			debug_crash();												\
		}																\
	}

#define my_rwlock_read_lock(rwlock)										\
	{																	\
		int semrc;														\
		semrc = pthread_rwlock_rdlock(rwlock);							\
		if(semrc != 0) {												\
			LEVEL_DEFAULT("semrc=%d [%s]\n", semrc, strerror(errno));	\
			debug_crash();												\
		}																\
	}

#define my_rwlock_read_unlock(rwlock)									\
	{																	\
		int semrc;														\
		semrc = pthread_rwlock_unlock(rwlock);							\
		if(semrc != 0) {												\
			LEVEL_DEFAULT("semrc=%d [%s]\n", semrc, strerror(errno));	\
			debug_crash();												\
		}																\
	}

#endif /* EXTENDED_RWLOCK_DEBUG */

#else                      /* PTHREAD_RWLOCK */

/* rwlock-implementation with semaphores */
#ifdef EXTENDED_RWLOCK_DEBUG

/* Don't use the builtin rwlock-support in pthread */
#define my_rwlock_init(rwlock)											\
	{																	\
		int semrc = 0;													\
		memset((rwlock), 0, sizeof(my_rwlock_t));						\
		_MUTEX_INIT((rwlock)->protect_reader_count);					\
		semrc |= sem_init(&((rwlock)->allow_readers), 0, 1);			\
		if(semrc != 0) {												\
			LEVEL_DEFAULT("rc=%d [%s]", semrc, strerror(errno));		\
			debug_crash();												\
		}																\
		semrc |= sem_init(&((rwlock)->no_processes), 0, 1);				\
		if(semrc != 0) {												\
			LEVEL_DEFAULT("rc=%d [%s]", semrc, strerror(errno));		\
			debug_crash();												\
		}																\
		(rwlock)->reader_count = 0;										\
	}

#define my_rwlock_destroy(rwlock)										\
	{																	\
		int semrc = 0;													\
		_MUTEX_DESTROY((rwlock)->protect_reader_count);					\
		semrc |= sem_destroy(&((rwlock)->allow_readers));				\
		if(semrc != 0) {												\
			LEVEL_DEFAULT("rc=%d [%s]", semrc, strerror(errno));		\
			debug_crash();												\
		}																\
		semrc |= sem_destroy(&((rwlock)->no_processes));				\
		if(semrc != 0) {												\
			LEVEL_DEFAULT("rc=%d [%s]", semrc, strerror(errno));		\
			debug_crash();												\
		}																\
		memset((rwlock), 0, sizeof(my_rwlock_t));						\
	}

#define my_rwlock_write_lock(rwlock)									\
	{																	\
		int sval, semrc;												\
		semrc = sem_wait(&((rwlock)->allow_readers));					\
		if(semrc != 0) {												\
			LEVEL_DEFAULT("semrc=%d [%s]\n", semrc, strerror(errno));	\
			debug_crash();												\
		}																\
		if((sem_getvalue(&((rwlock)->allow_readers), &sval) < 0) || (sval != 0)) { \
			LEVEL_DEFAULT("sval=%d\n", sval);							\
			debug_crash();												\
		}																\
		semrc = sem_wait(&((rwlock)->no_processes));					\
		if(semrc != 0) {												\
			LEVEL_DEFAULT("semrc=%d [%s]\n", semrc, strerror(errno));	\
			debug_crash();												\
		}																\
		if((sem_getvalue(&((rwlock)->no_processes), &sval) < 0) || (sval != 0)) { \
			LEVEL_DEFAULT("sval=%d\n", sval);							\
			debug_crash();												\
		}																\
	}

#define my_rwlock_write_unlock(rwlock)									\
	{																	\
		int sval, semrc;												\
		if((sem_getvalue(&((rwlock)->allow_readers), &sval) < 0) || (sval != 0)) { \
			LEVEL_DEFAULT("sval=%d != 0\n", sval);						\
			debug_crash();												\
		}																\
		semrc = sem_post(&((rwlock)->allow_readers));					\
		if(semrc != 0) {												\
			LEVEL_DEFAULT("semrc=%d [%s]\n", semrc, strerror(errno));	\
			debug_crash();												\
		}																\
		if((sem_getvalue(&((rwlock)->no_processes), &sval) < 0) || (sval != 0)) { \
			LEVEL_DEFAULT("sval=%d != 0\n", sval);						\
			debug_crash();												\
		}																\
		semrc = sem_post(&((rwlock)->no_processes));					\
		if(semrc != 0) {												\
			LEVEL_DEFAULT("semrc=%d [%s]\n", semrc, strerror(errno));	\
			debug_crash();												\
		}																\
	}

#define my_rwlock_read_lock(rwlock)										\
	{																	\
		int semrc;														\
		semrc = sem_wait(&((rwlock)->allow_readers));					\
		if(semrc != 0) {												\
			LEVEL_DEFAULT("semrc=%d [%s]\n", semrc, strerror(errno));	\
			debug_crash();												\
		}																\
		my_pthread_mutex_lock(&((rwlock)->protect_reader_count));		\
		if (++((rwlock)->reader_count) == 1) {							\
			sem_wait(&((rwlock)->no_processes));						\
		}																\
		my_pthread_mutex_unlock(&((rwlock)->protect_reader_count));		\
		semrc = sem_post(&((rwlock)->allow_readers));					\
		if(semrc != 0) {												\
			LEVEL_DEFAULT("semrc=%d [%s]\n", semrc, strerror(errno));	\
			debug_crash();												\
		}																\
	}

#define my_rwlock_read_unlock(rwlock)									\
	{																	\
		int semrc;														\
		my_pthread_mutex_lock(&((rwlock)->protect_reader_count));		\
		if ((rwlock)->reader_count <= 0) {								\
			LEVEL_DEFAULT("count<=0 %d\n", (rwlock)->reader_count);		\
			debug_crash();												\
		}																\
		if (--((rwlock)->reader_count) == 0) {							\
			semrc = sem_post(&((rwlock)->no_processes));				\
			if(semrc != 0) {											\
				LEVEL_DEFAULT("semrc=%d [%s]\n", semrc, strerror(errno)); \
				debug_crash();											\
			}															\
		}																\
		my_pthread_mutex_unlock(&((rwlock)->protect_reader_count));		\
	}
#endif /* EXTENDED_RWLOCK_DEBUG */

#endif /* PTHREAD_RWLOCK */

#endif							/* RWLOCK */
