/*

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

#ifndef OW_MUTEXES_H			/* tedious wrapper */
#define OW_MUTEXES_H

#include <pthread.h>
#define DEFAULT_THREAD_ATTR	NULL

extern struct mutexes {
	pthread_mutex_t stat_mutex;
	pthread_mutex_t controlflags_mutex;
	pthread_mutex_t fstat_mutex;
	pthread_mutex_t simul_mutex;
	pthread_mutex_t dir_mutex;
	pthread_mutex_t libusb_mutex;
	pthread_mutex_t typedir_mutex;
	pthread_mutex_t externaldir_mutex;
	pthread_mutex_t namefind_mutex;
	pthread_mutex_t aliasfind_mutex;
	pthread_mutex_t aliaslist_mutex;
	pthread_mutex_t externalcount_mutex;
	pthread_mutex_t timegm_mutex;
	pthread_mutex_t detail_mutex;
	
	pthread_mutexattr_t mattr; // mutex attribute -- used for all mutexes
	my_rwlock_t lib;
	my_rwlock_t cache;
	my_rwlock_t persistent_cache;
  #ifdef __UCLIBC__
	pthread_mutex_t uclibc_mutex;
  #endif							/* __UCLIBC__ */
} Mutex;



#define _MUTEX_ATTR_INIT(at)	my_pthread_mutexattr_init(    &(at) )
#define _MUTEX_ATTR_SET(at,typ)	my_pthread_mutexattr_settype( &(at) , typ )
#define _MUTEX_ATTR_DESTROY(at)	my_pthread_mutexattr_destroy( &(at) )

#define _MUTEX_INIT(mut)	my_pthread_mutex_init(    &(mut) , &(Mutex.mattr) )
#define _MUTEX_DESTROY(mut)	my_pthread_mutex_destroy( &(mut) )

#define _MUTEX_LOCK(mut)	my_pthread_mutex_lock(   &(mut) )
#define _MUTEX_UNLOCK(mut)	my_pthread_mutex_unlock( &(mut) )

#define RWLOCK_INIT(rw)		my_rwlock_init(    &(rw) )
#define RWLOCK_DESTROY(rw)	my_rwlock_destroy( &(rw) )

#define _SEM_INIT(sem, shared, value)	my_sem_init(  &(sem) , (shared), (value) )

#define RWLOCK_WLOCK(mut)	my_rwlock_write_lock(   &(mut) )
#define RWLOCK_WUNLOCK(mut)	my_rwlock_write_unlock( &(mut) )

#define RWLOCK_RLOCK(mut)	my_rwlock_read_lock(   &(mut) )
#define RWLOCK_RUNLOCK(mut)	my_rwlock_read_unlock( &(mut) )

#define LIB_WLOCK         	RWLOCK_WLOCK(   Mutex.lib    )
#define LIB_WUNLOCK       	RWLOCK_WUNLOCK( Mutex.lib    )
#define LIB_RLOCK         	RWLOCK_RLOCK(   Mutex.lib    )
#define LIB_RUNLOCK       	RWLOCK_RUNLOCK( Mutex.lib    )

#define CACHE_WLOCK       	RWLOCK_WLOCK(   Mutex.cache  )
#define CACHE_WUNLOCK     	RWLOCK_WUNLOCK( Mutex.cache  )
#define CACHE_RLOCK       	RWLOCK_RLOCK(   Mutex.cache  )
#define CACHE_RUNLOCK     	RWLOCK_RUNLOCK( Mutex.cache  )

#define PERSISTENT_WLOCK    RWLOCK_WLOCK(   Mutex.persistent_cache )
#define PERSISTENT_WUNLOCK  RWLOCK_WUNLOCK( Mutex.persistent_cache )
#define PERSISTENT_RLOCK    RWLOCK_RLOCK(   Mutex.persistent_cache )
#define PERSISTENT_RUNLOCK  RWLOCK_RUNLOCK( Mutex.persistent_cache )

#define CONNIN_WLOCK      	RWLOCK_WLOCK(   Inbound_Control.lock )
#define CONNIN_WUNLOCK    	RWLOCK_WUNLOCK( Inbound_Control.lock )
#define CONNIN_RLOCK      	RWLOCK_RLOCK(   Inbound_Control.lock )
#define CONNIN_RUNLOCK    	RWLOCK_RUNLOCK( Inbound_Control.lock )

#define MONITOR_WLOCK      	RWLOCK_WLOCK(   Inbound_Control.monitor_lock )
#define MONITOR_WUNLOCK    	RWLOCK_WUNLOCK( Inbound_Control.monitor_lock )
#define MONITOR_RLOCK      	RWLOCK_RLOCK(   Inbound_Control.monitor_lock )
#define MONITOR_RUNLOCK    	RWLOCK_RUNLOCK( Inbound_Control.monitor_lock )

#define STATLOCK          	_MUTEX_LOCK(  Mutex.stat_mutex   )
#define STATUNLOCK        	_MUTEX_UNLOCK(Mutex.stat_mutex   )

#define CONTROLFLAGSLOCK  	_MUTEX_LOCK(  Mutex.controlflags_mutex  )
#define CONTROLFLAGSUNLOCK	_MUTEX_UNLOCK(Mutex.controlflags_mutex  )

#define FSTATLOCK         	_MUTEX_LOCK(  Mutex.fstat_mutex  )
#define FSTATUNLOCK       	_MUTEX_UNLOCK(Mutex.fstat_mutex  )

#define SIMULLOCK         	_MUTEX_LOCK(  Mutex.simul_mutex  )
#define SIMULUNLOCK       	_MUTEX_UNLOCK(Mutex.simul_mutex  )

#define DIRLOCK           	_MUTEX_LOCK(  Mutex.dir_mutex    )
#define DIRUNLOCK         	_MUTEX_UNLOCK(Mutex.dir_mutex    )

#define LIBUSBLOCK        	_MUTEX_LOCK(  Mutex.libusb_mutex )
#define LIBUSBUNLOCK      	_MUTEX_UNLOCK(Mutex.libusb_mutex )

#define TYPEDIRLOCK       	_MUTEX_LOCK(  Mutex.typedir_mutex)
#define TYPEDIRUNLOCK     	_MUTEX_UNLOCK(Mutex.typedir_mutex)

#define EXTERNALDIRLOCK       _MUTEX_LOCK(  Mutex.externaldir_mutex)
#define EXTERNALDIRUNLOCK     _MUTEX_UNLOCK(Mutex.externaldir_mutex)

#define NAMEFINDLOCK      	_MUTEX_LOCK(  Mutex.namefind_mutex)
#define NAMEFINDUNLOCK    	_MUTEX_UNLOCK(Mutex.namefind_mutex)

#define ALIASFINDLOCK     	_MUTEX_LOCK(  Mutex.aliasfind_mutex)
#define ALIASFINDUNLOCK   	_MUTEX_UNLOCK(Mutex.aliasfind_mutex)

#define ALIASLISTLOCK     	_MUTEX_LOCK(  Mutex.aliaslist_mutex)
#define ALIASLISTUNLOCK   	_MUTEX_UNLOCK(Mutex.aliaslist_mutex)

#define EXTERNALCOUNTLOCK   _MUTEX_LOCK(  Mutex.externalcount_mutex)
#define EXTERNALCOUNTUNLOCK _MUTEX_UNLOCK(Mutex.externalcount_mutex)

#define TIMEGMLOCK   		_MUTEX_LOCK(  Mutex.timegm_mutex)
#define TIMEGMUNLOCK 		_MUTEX_UNLOCK(Mutex.timegm_mutex)

#define DETAILLOCK   		_MUTEX_LOCK(  Mutex.detail_mutex)
#define DETAILUNLOCK 		_MUTEX_UNLOCK(Mutex.detail_mutex)

#define BUSLOCK(pn)       	BUS_lock(pn)
#define BUSUNLOCK(pn)     	BUS_unlock(pn)
#define BUSLOCKIN(in)     	BUS_lock_in(in)
#define BUSUNLOCKIN(in)   	BUS_unlock_in(in)
#define CHANNELLOCKIN(in)   CHANNEL_lock_in(in)
#define CHANNELUNLOCKIN(in) CHANNEL_unlock_in(in)
#define PORTLOCKIN(in)     	PORT_lock_in(in)
#define PORTUNLOCKIN(in)   	PORT_unlock_in(in)
#ifdef __UCLIBC__
#define UCLIBCLOCK        	_MUTEX_LOCK(  Mutex.uclibc_mutex)
#define UCLIBCUNLOCK      	_MUTEX_UNLOCK(Mutex.uclibc_mutex)
#else							/* __UCLIBC__ */
#define UCLIBCLOCK			return_ok()
#define UCLIBCUNLOCK		return_ok()
#endif							/* __UCLIBC__ */

#define DETACH_THREAD		pthread_detach(pthread_self())

#endif							/* OW_MUTEXES_H */
