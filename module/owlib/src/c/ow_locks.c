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
#include <sys/time.h> /* for gettimeofday */
/* ------- Globals ----------- */
/* time elapsed in lock */
static struct timeval tv ; /* statistics */

#ifdef OW_MT
pthread_mutex_t bus_mutex = PTHREAD_MUTEX_INITIALIZER ;
pthread_mutex_t dev_mutex = PTHREAD_MUTEX_INITIALIZER ;
pthread_mutex_t stat_mutex = PTHREAD_MUTEX_INITIALIZER ;
#define DEVLOCKS    10
struct devlock DevLock[DEVLOCKS] ;
sem_t devlocks ;
int multithreading = 1 ;
int maxslots = DEVLOCKS ;
#else /* OW_MT */
int multithreading = 0 ;
int maxslots = 1 ;
#endif /* OW_MT */

void LockSetup( void ) {
#ifdef OW_MT
    int i ;
    for ( i=0 ; i<DEVLOCKS ; ++i) {
        DevLock[i].users = 0 ;
        pthread_mutex_init( &DevLock[i].lock, NULL ) ;
    }
    sem_init( &devlocks, 0, DEVLOCKS ) ;
#endif /* OW_MT */
}

void LockGet( const struct parsedname * const pn ) {
#ifdef OW_MT
    int i ;
    int empty ;
    /* Exclude requests that don't need locking */
    pn->si->lock = -1 ;
    switch( pn->ft->format ) {
    case ft_directory:
    case ft_subdir:
        return ;
    default:
        break ;
    }
    switch( pn->ft->change ) {
    case ft_static:
    case ft_Astable:
    case ft_Avolatile:
    case ft_statistic:
        return ;
    default:
        break ;
    }
//{ int e; sem_getvalue( &devlocks, &e);
//printf("LOCK %.2X.%.2X%.2X%.2X%.2X%.2X%.2X pre slots=%d\n",pn->sn[0],pn->sn[1],pn->sn[2],pn->sn[3],pn->sn[4],pn->sn[5],pn->sn[6],e);
//}
    /* potentially need an empty slot */
    sem_wait( &devlocks ) ;
//{ int e; sem_getvalue( &devlocks, &e);
//printf("LOCK %.2X.%.2X%.2X%.2X%.2X%.2X%.2X post slots=%d\n",pn->sn[0],pn->sn[1],pn->sn[2],pn->sn[3],pn->sn[4],pn->sn[5],pn->sn[6],e);
//}

    pthread_mutex_lock( &dev_mutex ) ;
    for ( i=0 ; i<DEVLOCKS ; ++i ) {
        if ( DevLock[i].users == 0 ) {
            empty = i ;
        } else if ( memcmp(DevLock[i].sn, pn->sn, 8) == 0 ) {
            sem_post( &devlocks ) ;
            ++DevLock[i].users ;
            pn->si->lock = i ;
//printf("LOCK %.2X.%.2X%.2X%.2X%.2X%.2X%.2X FOUND match=%d users=%d\n",pn->sn[0],pn->sn[1],pn->sn[2],pn->sn[3],pn->sn[4],pn->sn[5],pn->sn[6],pn->si->lock,DevLock[i].users) ;
            pthread_mutex_unlock( &dev_mutex ) ;
            pthread_mutex_lock( &DevLock[i].lock ) ;
            return ;
        }
    }
    memcpy(DevLock[empty].sn, pn->sn, 8) ;
    DevLock[empty].users = 1 ;
    pn->si->lock = empty ;
//printf("LOCK %.2X.%.2X%.2X%.2X%.2X%.2X%.2X NOT FOUND match=%d users=%d\n",pn->sn[0],pn->sn[1],pn->sn[2],pn->sn[3],pn->sn[4],pn->sn[5],pn->sn[6],pn->si->lock,DevLock[empty].users) ;
    pthread_mutex_lock(&DevLock[empty].lock) ;
    pthread_mutex_unlock( &dev_mutex ) ;
#endif /* OW_MT */
}

void LockRelease( const struct parsedname * const pn ) {
#ifdef OW_MT
    int lock = pn->si->lock ;
    if ( lock < 0 ) return ;
//printf("UNLOCK %.2X.%.2X%.2X%.2X%.2X%.2X%.2X PRE slot=%d users=%d \n",pn->sn[0],pn->sn[1],pn->sn[2],pn->sn[3],pn->sn[4],pn->sn[5],pn->sn[6],lock,DevLock[lock].users) ;
    pthread_mutex_lock( &dev_mutex ) ;
    pthread_mutex_unlock(&DevLock[lock].lock) ;
    if ( (--DevLock[lock].users) == 0 ) sem_post( &devlocks ) ;
//printf("UNLOCK %.2X.%.2X%.2X%.2X%.2X%.2X%.2X POST slot=%d users=%d \n",pn->sn[0],pn->sn[1],pn->sn[2],pn->sn[3],pn->sn[4],pn->sn[5],pn->sn[6],lock,DevLock[lock].users) ;
    pthread_mutex_unlock( &dev_mutex ) ;
#endif /* OW_MT */
}

/* Special note on locking:
     The bus lock is universal -- only one thread can hold it
     Therefore, we don't need a STATLOCK for bos_locks and bus_unlocks or bus_time
*/

void BUS_lock( void ) {
#ifdef OW_MT
    pthread_mutex_lock( &bus_mutex ) ;
#else /* OW_MT */
//    flock( devfd, LOCK_EX) ;
#endif /* OW_MT */
    gettimeofday( &tv , NULL ) ; /* for statistics */
    ++ bus_locks ; /* statistics */
}

void BUS_unlock( void ) {
    struct timeval tv2 ;
    gettimeofday( &tv2, NULL ) ;
    bus_time.tv_sec += tv2.tv_sec - tv.tv_sec ;
    bus_time.tv_usec += tv2.tv_usec - tv.tv_usec ;
    if ( bus_time.tv_usec > 100000000 ) {
        bus_time.tv_usec -= 100000000 ;
        bus_time.tv_sec  += 100 ;
	} else if ( bus_time.tv_usec < -100000000 ) {
        bus_time.tv_usec += 100000000 ;
        bus_time.tv_sec  -= 100 ;
    }
        ++ bus_unlocks ; /* statistics */
#ifdef OW_MT
    pthread_mutex_unlock( &bus_mutex ) ;
#else /* OW_MT */
//    flock( devfd, LOCK_UN) ;
#endif /* OW_MT */
}
