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

#include "owfs_config.h"
#include "ow.h"

#ifdef OW_CACHE
#include <limits.h>
#include <db.h>

int Cache_Add_common( const char * const k, const size_t ksize, const void * data, const size_t dsize, const enum ft_change change );
int Cache_Get_common( const char * const k, const size_t ksize, void * data, size_t * dsize, const enum ft_change change );
int Cache_Del_common( const char * const k, const size_t ksize );


/* Defines for db->open */
/* Berkeley DB changed the format for version 4.1+, adding a "transaction parameter" */
#if   ( (DB_VERSION_MAJOR > 3) && (DB_VERSION_MINOR > 0) ) || (DB_VERSION_MAJOR > 4)
    #define DBOPEN(DBASE)    DBASE->open(DBASE,NULL,NULL,NULL,DB_HASH,DB_CREATE|DB_THREAD,0)
#else /* DB41 */
    #define DBOPEN(DBASE)    DBASE->open(DBASE,NULL,NULL,DB_HASH,DB_CREATE|DB_THREAD,0)
#endif /* DB41 */

static DB * dbstore = NULL ;

/* Flipping cache databases, some parameters to determine when to flip */
#define ADDS_TILL_OLDTIME (100) /* transactions before flip */
#define CACHEPATHLENGTH  (2)

static struct {
    DB * new_db ; // current cache database
    DB * old_db ; // older cache database
    int old_is ; // does old exist
    time_t retired ; // start time of older
    time_t killed ; // deathtime of older
    time_t lifespan ; // lifetime of older
    unsigned int added ; // items added
} cache ;

time_t TimeOut( enum ft_change ftc ) {
    switch( ftc ) {
    case ft_second:
        return 1 ;
    case ft_volatile:
        return timeout ;
    case ft_stable:
        return timeout_slow;
    default: /* static or statistic */
        return 0 ;
    }
}

char * Cache_Version( void ) {
    return db_version( NULL, NULL, NULL ) ;
}

    /* DB cache creation code */
    /* Note: done in single-threaded mode so locking not yet needed */
void Cache_Open( void ) {
    if ( db_create( &cache.new_db, NULL, 0 ) || DBOPEN(cache.new_db) ) {
        syslog( LOG_WARNING, "Cannot create first database table for caching, error=%s, will proceed uncached.",db_strerror(errno) );
    } else { /* Cache ok */
        cacheavailable = 1 ;
        if ( db_create( &cache.old_db, NULL, 0 ) || DBOPEN(cache.new_db) ) {
            syslog( LOG_WARNING, "Cannot create second database table for caching, error=%s, will proceed with unpruned cache.",db_strerror(errno) ) ;
        } else { /* Old ok */
            cache.old_is = 0 ; // not yet is use
            cache.lifespan = TimeOut( ft_stable ) ;
            if (cache.lifespan>3600 ) cache.lifespan = 3600 ; /* 1 hour tops */
        }
    }

    if ( db_create( &dbstore, NULL, 0 ) || DBOPEN(dbstore) ) {
        syslog( LOG_WARNING, "Cannot create database table for storage, error=%s, will proceed unstored.",db_strerror(errno) ) ;
    }
}

    /* Note: done in single-threaded mode so locking not yet needed */
void Cache_Close( void ) {
   if ( cache.new_db ) {
        cache.new_db->close( cache.new_db, DB_NOSYNC ) ;
        cache.new_db = NULL ;
    }
   if ( cache.old_db ) {
        cache.old_db->close( cache.old_db, DB_NOSYNC ) ;
        cache.old_db = NULL ;
    }
   if ( dbstore ) {
        dbstore->close( dbstore, DB_NOSYNC ) ;
        dbstore = NULL ;
    }
}

/* Add an item to the cache */
/* retire the cache (flip) if too old, and start a new one (keep the old one for a while) */
/* return 0 if good, 1 if not */
int Cache_Add( const struct parsedname * const pn, const void * data, const size_t datasize ) {
    if ( cache.new_db && pn ) { // do check here to avoid needless processing
        unsigned int l = strlen( pn->ft->name ) ;
        char k[8+l] ;
        /* Key is serialnumber and name */
        memcpy( k , pn->sn , 8 ) ;
        memcpy( &k[8], pn->ft->name , l ) ;
        return Cache_Add_common( k, 8+l, data, datasize, pn->ft->change ) ;
    }
    return 1 ;
}

/* Add an item to the cache */
/* retire the cache (flip) if too old, and start a new one (keep the old one for a while) */
/* return 0 if good, 1 if not */
int Cache_Add_Internal( const struct parsedname * const pn, const char * const name, const void * data, const size_t datasize ) {
    if ( cache.new_db && pn ) { // do check here to avoid needless processing
        char k[8+4] ;
        /* Key is serialnumber and name */
        memcpy( k , pn->sn , 8 ) ;
        k[8] = 0xFF ; // invalid char for ral filename
        memcpy( &k[9], name , 3 ) ; // use only first 3 chars
        return Cache_Add_common( k, 8+4, data, datasize, ft_stable ) ;
    }
    return 1 ;
}

/* Add an item to the cache */
/* retire the cache (flip) if too old, and start a new one (keep the old one for a while) */
/* Already tested for existence of cache.new_db */
/* return 0 if good, 1 if not */
int Cache_Add_common( const char * const k, const size_t ksize, const void * data, const size_t dsize, const enum ft_change change ) {
    DBT key, val ;
    time_t duration = TimeOut( change ) ;
    size_t vsize = dsize+sizeof(time_t) ;
    unsigned char v[vsize] ;
    unsigned int discards ;

    if ( duration == 0 ) return 0 ; /* Uncached parameter */
//printf("CacheAdd test add_till=%d cache.old_is=%d\n",add_till,cache.old_is);
    CACHELOCK
        if ( cache.old_db ) {
            if ( cache.old_is ) {
                if  (cache.killed < time(NULL) ) cache.old_is = 0 ; /* oldcache completely timed out */
            } else if ( ++cache.added>ADDS_TILL_OLDTIME
#if DB_VERSION_MAJOR >= 4
			    && cache.old_db->truncate(cache.old_db,NULL,&discards,0)==0
#endif
			    ) {
    //printf("CacheAdd flipping add_till=%d cache.old_is=%d\n",add_till,cache.old_is);
                /* Flip caches! old = new. New truncated, reset time and counters and flag */
                DB * swap = cache.old_db ;
                cache.old_db  = cache.new_db ;
                cache.new_db = swap ;
                cache.added = 0 ;
                cache.old_is = 1 ;
                cache.retired = time(NULL) ;
                cache.killed = cache.retired + cache.lifespan ;
                STATLOCK
                    ++ cache_flips ; /* statistics */
                STATUNLOCK
    //printf("CacheAdd flipped add_till=%d cache.old_is=%d\n",add_till,cache.old_is);
            }
        }
    CACHEUNLOCK
    /* Key is serialnumber and name */
    memset( &key, 0, sizeof(key) ) ;
    key.size = ksize ;
    key.data = k ;

    memset( &val, 0, sizeof(val) ) ;
    duration += time(NULL) ;
//printf("ADD path=%s, size=%d, data=%s, time=%s\n",path,size,data,ctime(&to)) ;
    memcpy(v,&duration,sizeof(time_t)) ;
    memcpy(v+sizeof(time_t),data,dsize );
    val.size=vsize ;
    val.data = v ;
    STATLOCK
        ++ cache_adds ; /* statistics */
    STATUNLOCK
    return cache.new_db->put(cache.new_db,NULL,&key,&val,0) ;
}

/* Look in caches, 0=found and valid, 1=not or uncachable in the first place */
int Cache_Get( const struct parsedname * const pn, void * data, size_t * dsize ) {
    if ( cache.new_db && pn ) {
        unsigned int l = strlen( pn->ft->name ) ;
        char k[8+l] ;
        memcpy( k , pn->sn , 8 ) ;
        memcpy( &k[8], pn->ft->name , l ) ;
        return Cache_Get_common( k, 8+l, data, dsize, pn->ft->change ) ;
    }
    return 1 ;
}

/* Look in caches, 0=found and valid, 1=not or uncachable in the first place */
int Cache_Get_Internal( const struct parsedname * const pn, const char * const name, void * data, size_t * dsize ) {
    if ( cache.new_db && pn ) {
        char k[8+4] ;
        memcpy( k , pn->sn , 8 ) ;
        k[8] = 0xFF ;
        memcpy( &k[9], name , 3 ) ;
        return Cache_Get_common( k, 8+4, data, dsize, ft_stable ) ;
    }
    return 1 ;
}

/* Look in caches, 0=found and valid, 1=not or uncachable in the first place */
/* Already tested for existence of cache.new_db */
int Cache_Get_common( const char * const k, const size_t ksize, void * data, size_t * dsize, const enum ft_change change ) {
    DBT key, val ;
    size_t s ;
    time_t duration = TimeOut(change) ;
//printf("GET path=%s size=%d ft_change=%d\n",path,(int)(*size),change);
    if ( duration == 0 ) return 1 ; /* uncached item */
    STATLOCK
        ++ cache_tries ; /* statistics */
    STATUNLOCK
    memset( &key, 0, sizeof(key) ) ;
    memset( &val, 0, sizeof(val) ) ;
    key.size = ksize ;
    key.data = k ;
    CACHELOCK
        if ( cache.old_is && (cache.killed < time(NULL)) ) {
            cache.old_is = 0 ; /* oldcache completely timed out */
        }
    CACHEUNLOCK
//printf("GET about to get new\n") ;
    if ( cache.new_db->get(cache.new_db,NULL,&key,&val,0) ) { /* look in newcache */
//printf("GET about to get old 1\n") ;
        if ( cache.old_is && (cache.retired + duration > time(NULL)) ) { /* can we look in oldcache */
//printf("GET about to get old 2\n") ;
            if ( cache.old_db->get(cache.old_db,NULL,&key,&val,0) ) {
                STATLOCK
                    ++ cache_misses ; /* statistics */
                STATUNLOCK
                return 1 ; /* found in oldcache? */
            }
        } else {
            STATLOCK
                ++ cache_misses ; /* statistics */
            STATUNLOCK
            return 1 ; /* Old cache shouldnt be searched */
        }
    }
//printf("GOT \n") ;
    memcpy(&duration, val.data, sizeof(time_t)) ;
//printf("GOT expire=%s\n",ctime(&to)) ;
    if ( duration <= time(NULL) ) {
        STATLOCK
            ++ cache_expired ; /* statistics */
        STATUNLOCK
        return 1 ; /* check time of item */
    }
//printf("GOT unexpired\n") ;
    s = val.size - sizeof(time_t) ;
    if ( s>= *dsize ) return -EMSGSIZE ; /* buffer too small */
    *dsize = s ;
    memcpy(data,val.data+sizeof(time_t), s );
    STATLOCK
        ++ cache_hits ; /* statistics */
    STATUNLOCK
    return 0 ;
}

int Cache_Del( const struct parsedname * const pn ) {
    if ( cache.new_db && pn ) {
        unsigned int l = strlen( pn->ft->name ) ;
        char k[8+l] ;
        memcpy( k , pn->sn , 8 ) ;
        memcpy( &k[8], pn->ft->name , l ) ;
        return Cache_Del_common( k, 8+l ) ;
    }
    return 1 ;
}

int Cache_Del_Internal( const struct parsedname * const pn, const char * const name) {
    if ( cache.new_db && pn ) {
        char k[8+4] ;
        memcpy( k , pn->sn , 8 ) ;
        k[8] = 0xFF ;
        memcpy( &k[9], name , 3 ) ;
        return Cache_Del_common( k, 8+4 ) ;
    }
    return 1 ;
}

int Cache_Del_common( const char * const k, const size_t ksize ) {
    DBT key ;

    STATLOCK
        ++ cache_dels ; /* statistics */
    STATUNLOCK
    memset( &key, 0, sizeof(key) ) ;
    key.size = ksize ;
    key.data = k ;
    if ( cache.old_is ) cache.old_db->del(cache.old_db,NULL,&key,0) ;
    return cache.new_db->del(cache.new_db,NULL,&key,0) ;
}

/* Add a persistent entry -- no time signature */
int Storage_Add( const char * path, const size_t size, const void * data ) {
    if ( dbstore ) {
        DBT key, val ;
//int * d = data ;

        memset( &key, 0, sizeof(key) ) ;
        key.size = strlen(path) ;
        key.data = path ;
//printf("STORAGE_ADD key=%s, val=%d %d %d %d\n",path, d[0],d[1],d[2],d[3] ) ;

        memset( &val, 0, sizeof(val) ) ;
        val.size=size ;
        val.data = data ;
        return dbstore->put(dbstore,NULL,&key,&val,0) ;
//{
//int r = dbstore->put(dbstore,NULL,&key,&val,0) ;
//printf("STORAGE ADD = %d error=%s\n",r,db_strerror(r)) ;
//return r ;
//}
    }
    return 1 ;
}

int Storage_Get( const char * path, size_t *size, void * data ) {
    if ( dbstore ) {
        DBT key, val ;
//int * d ;
        memset( &key, 0, sizeof(key) ) ;
        key.size = strlen(path) ;
        key.data = path ;
//printf("STORAGE preget key=%s\n",key.data);

        memset( &val, 0, sizeof(val) ) ;
        if ( dbstore->get(dbstore,NULL,&key,&val,0) ) return 1 ;
//{
//int r = dbstore->get(dbstore,NULL,&key,&val,0) ;
//printf("STORAGE GET = %d error=%s\n",r,db_strerror(r)) ;
//if (r) return 1 ;
//}
//d = val.data ;
//printf("STORAGE_GET key=%s, val=%d %d %d %d\n",path, d[0],d[1],d[2],d[3] ) ;
        if ( val.size > *size ) return 1 ;
        *size = val.size ;
        memcpy(data,val.data, *size );
        return 0 ;
    }
    return 1 ;
}

int Storage_Del( const char * path ) {
    if ( dbstore ) {
        DBT key ;
        memset( &key, 0, sizeof(key) ) ;
        key.size = strlen(path) ;
        key.data = (char *) path ;
        return dbstore->del(dbstore,NULL,&key,0) ;
    }
    return 1 ;
}

#else /* OW_CACHE is unset */

char * Cache_Version( void )
    { return ""; }
void Cache_Open( void )
    {}
void Cache_Close( void )
    {}

int Cache_Add(          const struct parsedname * const pn,                          const void * data, const size_t datasize )
    { return 1; }
int Cache_Add_Internal( const struct parsedname * const pn, const char * const name, const void * data, const size_t datasize )
    { return 1; }

int Cache_Get(          const struct parsedname * const pn,                                void * data,       size_t * dsize )
    { return 1; }
int Cache_Get_Internal( const struct parsedname * const pn, const char * const name,       void * data,       size_t * dsize )
    { return 1; }

int Cache_Del(          const struct parsedname * const pn                                                                   )
    { return 1; }
int Cache_Del_Internal( const struct parsedname * const pn, const char * const name                                          )
    { return 1; }

int Storage_Add( const char * path, const size_t size, const void * data )
    { return 1; }
int Storage_Get( const char * path, size_t *size, void * data )
    { return 1; }
int Storage_Del( const char * path )
    { return 1; }

#endif /* OW_CACHE */
