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

#ifdef OW_CACHE

#include "owfs_config.h"
#include "ow.h"

/* Defines for db->open */
/* Berkeley DB changed the format for version 4.1+, adding a "transaction parameter" */
//#define DB41
#ifdef DB41
    #define DBOPEN(DBASE)	DBASE->open(DBASE,NULL,NULL,NULL,DB_HASH,DB_CREATE,0)
#else /* DB41 */
    #define DBOPEN(DBASE)	DBASE->open(DBASE,NULL,NULL,DB_HASH,DB_CREATE,0)
#endif /* DB41 */

DB * dbnewcache = NULL ;
DB * dboldcache = NULL ;
DB * dbstore = NULL ;

/* Flipping cache databases, some parameters to determine when to flip */
#define ADDS_TILL_OLDTIME (100) /* transactions before flip */
time_t oldstart ; /* When flipped (last new transation in old) */
int old_going = 0 ; /* old exists? */
time_t maxold ; /* Expire time for old */
#define CACHEPATHLENGTH  (2)

static int MakeKey( void * key, const struct parsedname * const pn ) ;

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

	/* DB cache code */
void Cache_Open( void ) {
    if ( db_create( &dbnewcache, NULL, 0 ) || DBOPEN(dbnewcache) ) {
	    syslog( LOG_WARNING, "Cannot create first database table for caching, error=%s, will proceed uncached.",db_strerror(errno) ) ;
	} else { /* Cache ok */
	    cacheavailable = 1 ;
		if ( db_create( &dboldcache, NULL, 0 ) || DBOPEN(dbnewcache) ) {
			syslog( LOG_WARNING, "Cannot create second database table for caching, error=%s, will proceed with unpruned cache.",db_strerror(errno) ) ;
		} else { /* Old ok */
			maxold = TimeOut( ft_stable ) ;
			if (maxold>3600 ) maxold = 3600 ; /* 1 hour tops */
		}
	}

	if ( db_create( &dbstore, NULL, 0 ) || DBOPEN(dbstore) ) {
	    syslog( LOG_WARNING, "Cannot create database table for storage, error=%s, will proceed unstored.",db_strerror(errno) ) ;
	}
}

void Cache_Close( void ) {
   if ( dbnewcache ) {
	    dbnewcache->close( dbnewcache, DB_NOSYNC ) ;
		dbnewcache = NULL ;
	}
   if ( dboldcache ) {
	    dboldcache->close( dboldcache, DB_NOSYNC ) ;
		dboldcache = NULL ;
	}
   if ( dbstore ) {
	    dbstore->close( dbstore, DB_NOSYNC ) ;
		dbstore = NULL ;
	}
}

/* Add an item to the cache */
/* retire the cache (flip) if too old, and start a new one (keep the old one for a while) */
/* return 0 if good, 1 if not */
int Cache_Add( const char * path, const size_t size, const void * data, const enum ft_change change ) {
    static int add_till = 0 ; /* counter up to ADDS_TILL_OLDTIME */
    if ( dbnewcache ) {
		DBT key, val ;
	    time_t to = TimeOut( change ) ;
		size_t vsize = size+sizeof(time_t) ;
		unsigned char v[vsize] ;
		unsigned int discards ;

        if ( to == 0 ) return 0 ; /* Uncached parameter */
//printf("CacheAdd test add_till=%d old_going=%d\n",add_till,old_going);
        if ( dboldcache ) {
			if ( old_going ) {
				if  (oldstart + maxold < time(NULL) ) old_going = 0 ; /* oldcache completely timed out */
			} else if ( ++add_till>ADDS_TILL_OLDTIME && dboldcache->truncate(dboldcache,NULL,&discards,0)==0 ) {
//printf("CacheAdd flipping add_till=%d old_going=%d\n",add_till,old_going);
				/* Flip caches! old = new. New truncated, reset time and counters and flag */
				DB * swap = dboldcache ;
				dboldcache  = dbnewcache ;
				dbnewcache = swap ;
				add_till = 0 ;
				old_going = 1 ;
				oldstart = time(NULL) ;
						++ cache_flips ; /* statistics */
//printf("CacheAdd flipped add_till=%d old_going=%d\n",add_till,old_going);
    		}
		}
		memset( &key, 0, sizeof(key) ) ;
		key.size = strlen(path) ;
		key.data = (char *) path ;
		memset( &val, 0, sizeof(val) ) ;
		to += time(NULL) ;
//printf("ADD path=%s, size=%d, data=%s, time=%s\n",path,size,data,ctime(&to)) ;
		memcpy(v,&to,sizeof(time_t)) ;
		memcpy(v+sizeof(time_t),data,size );
		val.size=vsize ;
		val.data = v ;
                ++ cache_adds ; /* statistics */
		return dbnewcache->put(dbnewcache,NULL,&key,&val,0) ;
	}
	return 1 ;
}

/* Look in caches, 0=found and valid, 1=not or uncachable in the first place */
int Cache_Get( const char * path, size_t *size, void * data, const enum ft_change change ) {
    if ( dbnewcache ) {
		DBT key, val ;
		size_t s ;
		time_t to = TimeOut(change) ;
//printf("GET path=%s size=%d ft_change=%d\n",path,(int)(*size),change);
        if ( to == 0 ) return 1 ; /* uncached item */
            ++ cache_tries ; /* statistics */
		if ( old_going && (oldstart + maxold < time(NULL)) ) {
		    old_going = 0 ; /* oldcache completely timed out */
		}
		memset( &key, 0, sizeof(key) ) ;
		memset( &val, 0, sizeof(val) ) ;
		key.size = strlen(path) ;
		key.data = (char *) path ;
//printf("GET about to get new\n") ;
		if ( dbnewcache->get(dbnewcache,NULL,&key,&val,0) ) { /* look in newcache */
//printf("GET about to get old 1\n") ;
		    if ( old_going && (oldstart + to > time(NULL)) ) { /* can we look in oldcache */
//printf("GET about to get old 2\n") ;
			if ( dboldcache->get(dboldcache,NULL,&key,&val,0) ) {
                            ++ cache_misses ; /* statistics */
                            return 1 ; /* found in oldcache? */
                        }
                    } else {
                        ++ cache_misses ; /* statistics */
		        return 1 ; /* Old cache shouldnt be searched */
		    }
		}
//printf("GOT \n") ;
		memcpy(&to, val.data, sizeof(time_t)) ;
//printf("GOT expire=%s\n",ctime(&to)) ;
		if ( to <= time(NULL) ) {
                    ++ cache_expired ; /* statistics */
                    return 1 ; /* check time of item */
                }
//printf("GOT unexpired\n") ;
		s = val.size - sizeof(time_t) ;
		if (s<*size) *size = s ;
		memcpy(data,val.data+sizeof(time_t), *size );
                ++ cache_hits ; /* statistics */
		return 0 ;
	}
	return 1 ;
}

int Cache_Del( const char * path ) {
    if ( dbnewcache ) {
		DBT key ;

         ++ cache_dels ; /* statistics */
		memset( &key, 0, sizeof(key) ) ;
		key.size = strlen(path) ;
		key.data = (char *) path ;
		if ( old_going ) dboldcache->del(dboldcache,NULL,&key,0) ;
		return dbnewcache->del(dbnewcache,NULL,&key,0) ;
	}
	return 1 ;
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

int MakeKey( void * key, const struct parsedname * const pn ) {
    int spn = sizeof( struct parsedname ) ;
    int sbp = pn->pathlength * sizeof(struct buspath) ;
    if ( pn->pathlength > CACHEPATHLENGTH ) {
        key = malloc( spn + sbp ) ;
        if ( key==NULL ) return -ENOMEM ;
    }
    memcpy( key, pn, spn ) ;
    memcpy( key + spn, pn->bp, sbp ) ;
    ((struct parsedname *) key) -> bp = 0 ;
    return 0 ;
}


#endif /* OW_CACHE */
