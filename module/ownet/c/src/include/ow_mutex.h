/*
$Id$
    OW -- One-Wire filesystem
    version 0.4 7/2/2003


    Written 2003 Paul H Alfille
        Fuse code based on "fusexmp" {GPL} by Miklos Szeredi, mszeredi@inf.bme.hu
        Serial code based on "xt" {GPL} by David Querbach, www.realtime.bc.ca
        in turn based on "miniterm" by Sven Goldt, goldt@math.tu.berlin.de
    GPL license
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License
    as published by the Free Software Foundation; either version 2
    of the License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    Other portions based on Dallas Semiconductor Public Domain Kit,
    ---------------------------------------------------------------------------
    Copyright (C) 2000 Dallas Semiconductor Corporation, All Rights Reserved.
        Permission is hereby granted, free of charge, to any person obtaining a
        copy of this software and associated documentation files (the "Software"),
        to deal in the Software without restriction, including without limitation
        the rights to use, copy, modify, merge, publish, distribute, sublicense,
        and/or sell copies of the Software, and to permit persons to whom the
        Software is furnished to do so, subject to the following conditions:
        The above copyright notice and this permission notice shall be included
        in all copies or substantial portions of the Software.
    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
    OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
    MERCHANTABILITY,  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
    IN NO EVENT SHALL DALLAS SEMICONDUCTOR BE LIABLE FOR ANY CLAIM, DAMAGES
    OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
    ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
    OTHER DEALINGS IN THE SOFTWARE.
        Except as contained in this notice, the name of Dallas Semiconductor
        shall not be used except as stated in the Dallas Semiconductor
        Branding Policy.
    ---------------------------------------------------------------------------
    Implementation:
    25-05-2003 iButtonLink device
*/


/* Cannot stand alone -- part of ow.h and only separated out for clarity */

#ifndef OW_MUTEX_H			/* tedious wrapper */
#define OW_MUTEX_H

extern const char mutex_init_failed[];
extern const char mutex_destroy_failed[];
extern const char mutex_lock_failed[];
extern const char mutex_unlock_failed[];
extern const char mutexattr_init_failed[];
extern const char mutexattr_destroy_failed[];
extern const char mutexattr_settype_failed[];
extern const char rwlock_init_failed[];
extern const char rwlock_read_lock_failed[];
extern const char rwlock_read_unlock_failed[];
extern const char cond_timedwait_failed[];
extern const char cond_signal_failed[];
extern const char cond_broadcast_failed[];
extern const char cond_wait_failed[];
extern const char cond_init_failed[];
extern const char cond_destroy_failed[];
extern const char sem_init_failed[];
extern const char sem_post_failed[];
extern const char sem_wait_failed[];
extern const char sem_trywait_failed[];
extern const char sem_timedwait_failed[];
extern const char sem_destroy_failed[];

/* FreeBSD might fail on sem_init() since they are limited per machine */
#define my_sem_init(sem, shared, value)                                 \
	do {																\
		int mrc = sem_init(sem, shared, value);							\
		if(mrc != 0) {													\
			FATAL_ERROR(sem_init_failed, mrc, strerror(mrc));			\
		}																\
		if (Globals.error_level>=10) {									\
			LEVEL_DEFAULT("sem_init %lX, %d, %d\n", (unsigned long)sem, shared, value); \
		}																\
	} while(0)

#define my_sem_post(sem)                                                \
	do {																\
		int mrc = sem_post(sem);										\
		if(mrc != 0) {													\
			FATAL_ERROR(sem_post_failed, mrc, strerror(mrc));			\
		}																\
		if (Globals.error_level>=10) {									\
			LEVEL_DEFAULT("sem_post %lX done", (unsigned long)sem);		\
		}																\
	} while(0)

#define my_sem_wait(sem)                                                \
	do {																\
		int mrc = sem_wait(sem);										\
		if(mrc != 0) {													\
			FATAL_ERROR(sem_wait_failed, mrc, strerror(mrc));			\
		}																\
		if (Globals.error_level>=10) {									\
			LEVEL_DEFAULT("sem_wait %lX done", (unsigned long)sem);		\
		}																\
	} while(0)

#define my_sem_trywait(sem, res)                                        \
	do {																\
		int mrc = sem_trywait(sem);										\
		if((mrc < 0) && (errno != EAGAIN)) {							\
			FATAL_ERROR(sem_trywait_failed, mrc, strerror(mrc));		\
		}																\
		*(res) = mrc;													\
		if (Globals.error_level>=10) {									\
			LEVEL_DEFAULT("sem_trywait %lX done", (unsigned long)sem);	\
		}																\
	} while(0)

#define my_sem_timedwait(sem, ts, res)                                  \
	do {																\
		int mrc;														\
		while(((mrc = sem_timedwait(sem, ts)) == -1) && (errno == EINTR)) { \
			continue; /* Restart if interrupted by handler */			\
		}																\
		if((mrc < 0) && (errno != ETIMEDOUT)) {							\
			FATAL_ERROR(sem_timedwait_failed, mrc, strerror(mrc));		\
		}																\
		*(res) = mrc; /* res=-1 timeout, res=0 succeed */				\
		if (Globals.error_level>=10) {									\
			LEVEL_DEFAULT("sem_timedwait %lX done", (unsigned long)sem); \
		}																\
	} while(0)

#define my_sem_destroy(sem)                                             \
	do {																\
		int mrc = sem_destroy(sem);										\
		if(mrc != 0) {													\
			FATAL_ERROR(sem_destroy_failed, mrc, strerror(mrc));		\
		}																\
		if (Globals.error_level>=10) {									\
			LEVEL_DEFAULT("sem_destroy %lX", (unsigned long)sem);		\
		}																\
	} while(0)


#define my_pthread_mutex_init(mutex, attr)                              \
	do {																\
		int mrc;														\
		if (Globals.error_level>=e_err_debug) {							\
			LEVEL_DEFAULT("pthread_mutex_init %lX begin", (unsigned long)mutex); \
		}																\
		mrc = pthread_mutex_init(mutex, attr);							\
		if(mrc != 0) {													\
			FATAL_ERROR(mutex_init_failed, mrc, strerror(mrc));			\
		}																\
		if (Globals.error_level>=10) {									\
			LEVEL_DEFAULT("pthread_mutex_init %lX done", (unsigned long)mutex); \
		}																\
	} while(0)

#define my_pthread_mutex_destroy(mutex)                                 \
	do {																\
		int mrc = pthread_mutex_destroy(mutex);							\
		if (Globals.error_level>=e_err_debug) {							\
			LEVEL_DEFAULT("pthread_mutex_destroy %lX begin", (unsigned long)mutex); \
		}																\
		if(mrc != 0) {													\
			LEVEL_DEFAULT(mutex_destroy_failed, mrc, strerror(mrc));	\
		}																\
		if (Globals.error_level>=10) {									\
			LEVEL_DEFAULT("pthread_mutex_destroy %lX done", (unsigned long)mutex); \
		}																\
	} while(0)

#define my_pthread_mutex_lock(mutex)                                    \
	do {																\
		int mrc;														\
		if (Globals.error_level>=e_err_debug) {							\
			LEVEL_DEFAULT("pthread_mutex_lock %lX begin", (unsigned long)mutex); \
		}																\
		mrc = pthread_mutex_lock(mutex);								\
		if(mrc != 0) {													\
			FATAL_ERROR(mutex_lock_failed, mrc, strerror(mrc));			\
		}																\
		if (Globals.error_level>=10) {									\
			LEVEL_DEFAULT("pthread_mutex_lock %lX done", (unsigned long)mutex); \
		}																\
	} while(0)

#define my_pthread_mutex_unlock(mutex)                                  \
	do {																\
		int mrc;														\
		if (Globals.error_level>=e_err_debug) {							\
			LEVEL_DEFAULT("pthread_mutex_unlock %lX begin", (unsigned long)mutex); \
		}																\
		mrc = pthread_mutex_unlock(mutex);								\
		if(mrc != 0) {													\
			FATAL_ERROR(mutex_unlock_failed, mrc, strerror(mrc));		\
		}																\
		if (Globals.error_level>=10) {									\
			LEVEL_DEFAULT("pthread_mutex_unlock %lX done", (unsigned long)mutex); \
		}																\
	} while(0)

#define my_pthread_mutexattr_init(attr)                                 \
	do {																\
		int mrc = pthread_mutexattr_init(attr);							\
		if(mrc != 0) {													\
			FATAL_ERROR(mutexattr_init_failed, mrc, strerror(mrc));		\
		}																\
	} while(0)

#define my_pthread_mutexattr_destroy(attr)							   \
	do {                                                               \
		int mrc = pthread_mutexattr_destroy(attr);					   \
		if(mrc != 0) {												   \
			FATAL_ERROR(mutexattr_destroy_failed, mrc, strerror(mrc)); \
		}															   \
	} while(0)

#define my_pthread_mutexattr_settype(attr, typ)                         \
	do {																\
		int mrc = pthread_mutexattr_settype(attr, typ);					\
		if(mrc != 0) {													\
			FATAL_ERROR(mutexattr_settype_failed, mrc, strerror(mrc));	\
		}																\
	} while(0)

#define my_pthread_cond_timedwait(cond, mutex, abstime)				   \
	do {                                                               \
		int mrc = pthread_cond_timedwait(cond, mutex, abstime);		   \
		if(mrc != 0) {												   \
			FATAL_ERROR(cond_timedwait_failed, mrc, strerror(mrc));	   \
		}															   \
	} while(0)

#define my_pthread_cond_wait(cond, mutex)							 \
	do {                                                             \
		int mrc = pthread_cond_wait(cond, mutex);					 \
		if(mrc != 0) {												 \
			FATAL_ERROR(cond_wait_failed, mrc, strerror(mrc));		 \
		}															 \
	} while(0)

#define my_pthread_cond_signal(cond)								\
	do {                                                            \
		int mrc = pthread_cond_signal(cond);						\
		if(mrc != 0) {												\
			FATAL_ERROR(cond_signal_failed, mrc, strerror(mrc));	\
		}															\
	} while(0)

#define my_pthread_cond_broadcast(cond)								   \
	do {                                                               \
		int mrc = pthread_cond_broadcast(cond);						   \
		if(mrc != 0) {												   \
			FATAL_ERROR(cond_broadcast_failed, mrc, strerror(mrc));	   \
		}															   \
	} while(0)

#define my_pthread_cond_init(cond, attr)                                \
	do {																\
		int mrc = pthread_cond_init(cond, attr);						\
		if(mrc != 0) {													\
			FATAL_ERROR(cond_init_failed, mrc, strerror(mrc));			\
		}																\
	} while(0)

#define my_pthread_cond_destroy(cond)								 \
	do {                                                             \
		int mrc = pthread_cond_destroy(cond);						 \
		if(mrc != 0) {												 \
			FATAL_ERROR(cond_destroy_failed, mrc, strerror(mrc));	 \
		}															 \
	} while(0)

#endif
