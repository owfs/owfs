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

#include "owfs_config.h"
#include "ow.h"
#include "ow_counters.h"
#include "ow_connection.h"

#include <sys/time.h> /* for gettimeofday */
/* ------- Globals ----------- */

#ifdef OW_MT
pthread_mutex_t stat_mutex    = PTHREAD_MUTEX_INITIALIZER ;
pthread_mutex_t cache_mutex   = PTHREAD_MUTEX_INITIALIZER ;
pthread_mutex_t store_mutex   = PTHREAD_MUTEX_INITIALIZER ;
pthread_mutex_t fstat_mutex   = PTHREAD_MUTEX_INITIALIZER ;
pthread_mutex_t dir_mutex     = PTHREAD_MUTEX_INITIALIZER ;
pthread_mutex_t libusb_mutex  = PTHREAD_MUTEX_INITIALIZER ;
#ifdef __UCLIBC__
/* vsnprintf() doesn't seem to be thread-safe in uClibc
   even if thread-support is enabled. */
pthread_mutex_t uclibc_mutex = PTHREAD_MUTEX_INITIALIZER ;
#endif
#endif /* OW_MT */

/* maxslots and multithreading are ints to allow address-of */
#ifdef OW_MT
    int multithreading = 1 ;
#else /* OW_MT */
    int multithreading = 0 ;
#endif /* OW_MT */

/* Essentially sets up semaphore for device slots */
void LockSetup( void ) {
#ifdef OW_MT
 #ifdef __UCLIBC__
    pthread_mutex_init(&stat_mutex, pmattr);
    pthread_mutex_init(&cache_mutex, pmattr);
    pthread_mutex_init(&store_mutex, pmattr);
    pthread_mutex_init(&fstat_mutex, pmattr);
    pthread_mutex_init(&dir_mutex, pmattr);
    pthread_mutex_init(&reconnect_mutex, pmattr);
    pthread_mutex_init(&uclibc_mutex, pmattr);
 #endif /* UCLIBC */
#endif /* OW_MT */
}

/* Bad bad C library */
/* implementation of tfind, tsearch returns an opaque structure */
/* you have to know that the first element is a pointer to your data */
struct dev_opaque {
    struct devlock * key ;
    void * other ;
} ;

#ifdef OW_MT
/* compilation error in gcc version 4.0.0 20050519 if dev_compare
 * is defined as an embedded function
 */
static int dev_compare( const void * a , const void * b ) {
    return memcmp( &((const struct devlock *)a)->sn , &((const struct devlock *)b)->sn , 8 ) ;
}
#endif

/* Grabs a device slot, either one already matching, or an empty one */
int LockGet( const struct parsedname * const pn ) {
#ifdef OW_MT
    struct devlock * dlock ;
    struct dev_opaque * opaque ;

    //printf("LockGet() pn->path=%s\n", pn->path);
    if(pn->dev == DeviceSimultaneous) {
      /* Shouldn't call LockGet() on DeviceSimultaneous. No sn exists */
      return 0 ;
    }

    pn->si->lock = NULL ;
    /* Need locking? */
    switch( pn->ft->format ) {
    case ft_directory:
    case ft_subdir:
        return 0 ;
    default:
        break ;
    }
    switch( pn->ft->change ) {
    case ft_static:
    case ft_Astable:
    case ft_Avolatile:
    case ft_statistic:
        return 0 ;
    default:
        break ;
    }
    
    if ( (dlock = malloc( sizeof( struct devlock ) ))==NULL ) return -ENOMEM ;
    memcpy( dlock->sn, pn->sn, 8 ) ;
    
    DEVLOCK(pn);
    if ( (opaque=tsearch(dlock,&(pn->in->dev_db),dev_compare))==NULL ) {
        DEVUNLOCK(pn);
        free(dlock) ;
        return -ENOMEM ;
    } else if ( dlock==opaque->key ) {
        dlock->users = 1 ;
        pthread_mutex_init(&(dlock->lock), pmattr);
        pthread_mutex_lock(&(dlock->lock) ) ;
        DEVUNLOCK(pn);
        pn->si->lock = dlock ;
    } else {
        ++(opaque->key->users) ;
        DEVUNLOCK(pn);
        free(dlock) ;
        pthread_mutex_lock( &(opaque->key->lock) ) ;
        pn->si->lock = opaque->key ;
    }
#endif /* OW_MT */
    return 0 ;
}

void LockRelease( const struct parsedname * const pn ) {
#ifdef OW_MT
    if ( pn->si->lock ) {

        /* Shouldn't call LockRelease() on DeviceSimultaneous. No sn exists */
        if(pn->dev == DeviceSimultaneous) return ;
       
        pthread_mutex_unlock( &(pn->si->lock->lock) ) ;
        DEVLOCK(pn);
        if ( pn->si->lock->users==1 ) {
                tdelete( pn->si->lock, &(pn->in->dev_db), dev_compare ) ;
                DEVUNLOCK(pn);
                pthread_mutex_destroy(&(pn->si->lock->lock) ) ;
                free(pn->si->lock) ;
        } else {
                --pn->si->lock->users;
                DEVUNLOCK(pn);
        }
        pn->si->lock = NULL ;
    }
#endif /* OW_MT */
}

/* Special note on locking:
     The bus lock is universal -- only one thread can hold it
     Therefore, we don't need a STATLOCK for bus_locks and bus_unlocks or bus_time
     Actually, the new design has several busses, so a separate lock
*/

void BUS_lock( const struct parsedname * pn ) {
    if(!pn || !pn->in) return ;
#ifdef OW_MT
    pthread_mutex_lock( &(pn->in->bus_mutex) ) ;
#endif /* OW_MT */
    gettimeofday( &(pn->in->last_lock) , NULL ) ; /* for statistics */
    STATLOCK;
        ++ pn->in->bus_locks ; /* statistics */
        ++ total_bus_locks ; /* statistics */
    STATUNLOCK;
}

void BUS_unlock( const struct parsedname * pn ) {
    struct timeval *t;
    long sec, usec;
    if(!pn || !pn->in) return ;

    gettimeofday( &(pn->in->last_unlock), NULL ) ;

    /* avoid update if system-clock have changed */
    STATLOCK;
        sec = pn->in->last_unlock.tv_sec - pn->in->last_lock.tv_sec;
        if((sec >= 0) && (sec < 60)) {
            usec = pn->in->last_unlock.tv_usec - pn->in->last_lock.tv_usec;
            total_bus_time.tv_sec += sec;
            total_bus_time.tv_usec += usec;
            if ( total_bus_time.tv_usec >= 1000000 ) {
                total_bus_time.tv_usec -= 1000000 ;
                ++total_bus_time.tv_sec;
            } else if ( total_bus_time.tv_usec < 0 ) {
                total_bus_time.tv_usec += 1000000 ;
                --total_bus_time.tv_sec;
            }

            t = &pn->in->bus_time;
            t->tv_sec += sec;
            t->tv_usec += usec;
            if ( t->tv_usec >= 1000000 ) {
                t->tv_usec -= 1000000 ;
                ++t->tv_sec;
            } else if ( t->tv_usec < 0 ) {
                t->tv_usec += 1000000 ;
                --t->tv_sec;
            }
        }
        ++ pn->in->bus_unlocks ; /* statistics */
        ++ total_bus_unlocks ; /* statistics */
    STATUNLOCK;
#ifdef OW_MT
    pthread_mutex_unlock( &(pn->in->bus_mutex) ) ;
#endif /* OW_MT */
}
