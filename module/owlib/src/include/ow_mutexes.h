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

#ifndef OW_MUTEXES_H			/* tedious wrapper */
#define OW_MUTEXES_H


#if OW_MT
#include <pthread.h>
#endif							/* OW_MT */

extern struct mutexes {
#if OW_MT
	pthread_mutex_t stat_mutex;
	pthread_mutex_t sg_mutex;
	pthread_mutex_t fstat_mutex;
	pthread_mutex_t simul_mutex;
	pthread_mutex_t dir_mutex;
	pthread_mutex_t libusb_mutex;
	pthread_mutexattr_t *pmattr;
	my_rwlock_t lib;
	my_rwlock_t cache;
	my_rwlock_t store;
#ifdef __UCLIBC__
	pthread_mutexattr_t mattr;
	pthread_mutex_t uclibc_mutex;
#endif							/* __UCLIBC__ */
#endif							/* OW_MT */
} Mutex;


#if OW_MT
#define LIB_WLOCK         my_rwlock_write_lock(   &Mutex.lib    ) ;
#define LIB_WUNLOCK       my_rwlock_write_unlock( &Mutex.lib    ) ;
#define LIB_RLOCK         my_rwlock_read_lock(    &Mutex.lib    ) ;
#define LIB_RUNLOCK       my_rwlock_read_unlock(  &Mutex.lib    ) ;

#define CACHE_WLOCK       my_rwlock_write_lock(   &Mutex.cache  ) ;
#define CACHE_WUNLOCK     my_rwlock_write_unlock( &Mutex.cache  ) ;
#define CACHE_RLOCK       my_rwlock_read_lock(    &Mutex.cache  ) ;
#define CACHE_RUNLOCK     my_rwlock_read_unlock(  &Mutex.cache  ) ;

#define STORE_WLOCK       my_rwlock_write_lock(   &Mutex.store ) ;
#define STORE_WUNLOCK     my_rwlock_write_unlock( &Mutex.store ) ;
#define STORE_RLOCK       my_rwlock_read_lock(    &Mutex.store ) ;
#define STORE_RUNLOCK     my_rwlock_read_unlock(  &Mutex.store ) ;

#define CONNIN_WLOCK      my_rwlock_write_lock(   &Inbound_Control.lock ) ;
#define CONNIN_WUNLOCK    my_rwlock_write_unlock( &Inbound_Control.lock ) ;
#define CONNIN_RLOCK      my_rwlock_read_lock(    &Inbound_Control.lock ) ;
#define CONNIN_RUNLOCK    my_rwlock_read_unlock(  &Inbound_Control.lock ) ;

#define STATLOCK          pthread_mutex_lock(  &Mutex.stat_mutex   )
#define STATUNLOCK        pthread_mutex_unlock(&Mutex.stat_mutex   )
#define SGLOCK            pthread_mutex_lock(  &Mutex.sg_mutex  )
#define SGUNLOCK          pthread_mutex_unlock(&Mutex.sg_mutex  )
#define FSTATLOCK         pthread_mutex_lock(  &Mutex.fstat_mutex  )
#define FSTATUNLOCK       pthread_mutex_unlock(&Mutex.fstat_mutex  )
#define SIMULLOCK         pthread_mutex_lock(  &Mutex.simul_mutex  )
#define SIMULUNLOCK       pthread_mutex_unlock(&Mutex.simul_mutex  )
#define DIRLOCK           pthread_mutex_lock(  &Mutex.dir_mutex    )
#define DIRUNLOCK         pthread_mutex_unlock(&Mutex.dir_mutex    )
#define LIBUSBLOCK        pthread_mutex_lock(  &Mutex.libusb_mutex    )
#define LIBUSBUNLOCK      pthread_mutex_unlock(&Mutex.libusb_mutex    )
#define BUSLOCK(pn)       BUS_lock(pn)
#define BUSUNLOCK(pn)     BUS_unlock(pn)
#define BUSLOCKIN(in)       BUS_lock_in(in)
#define BUSUNLOCKIN(in)     BUS_unlock_in(in)
#ifdef __UCLIBC__
#define UCLIBCLOCK     pthread_mutex_lock(  &Mutex.uclibc_mutex)
#define UCLIBCUNLOCK   pthread_mutex_unlock(&Mutex.uclibc_mutex)
#else							/* __UCLIBC__ */
#define UCLIBCLOCK
#define UCLIBCUNLOCK
#endif							/* __UCLIBC__ */

#else							/* OW_MT */
#define LIB_WLOCK
#define LIB_WUNLOCK
#define LIB_RLOCK
#define LIB_RUNLOCK

#define CACHE_WLOCK
#define CACHE_WUNLOCK
#define CACHE_RLOCK
#define CACHE_RUNLOCK

#define STORE_WLOCK
#define STORE_WUNLOCK
#define STORE_RLOCK
#define STORE_RUNLOCK

#define CONNIN_WLOCK
#define CONNIN_WUNLOCK
#define CONNIN_RLOCK
#define CONNIN_RUNLOCK

#define STATLOCK
#define STATUNLOCK
#define SGLOCK
#define SGUNLOCK
#define FSTATLOCK
#define FSTATUNLOCK
#define SIMULLOCK
#define SIMULUNLOCK
#define DIRLOCK
#define DIRUNLOCK
#define UCLIBCLOCK
#define UCLIBCUNLOCK
#define LIBUSBLOCK
#define LIBUSBUNLOCK
#define UCLIBCLOCK
#define UCLIBCUNLOCK
#define BUSLOCK(pn)
#define BUSUNLOCK(pn)
#define BUSLOCKIN(in)
#define BUSUNLOCKIN(in)
#endif							/* OW_MT */

#endif							/* OW_MUTEXES_H */
