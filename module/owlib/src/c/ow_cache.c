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

// 8/18/2004 -- applied Serg Oskin's correction
// 8/20/2004 -- changed everything, specifically no longer using db, tsearch instead!
#include <config.h>
#include "owfs_config.h"
#include "ow.h"
#include "ow_counters.h"

// #define CACHE_DEBUG

#if OW_CACHE
#include <limits.h>

/* Put the globals into a struct to declutter the namespace */
static struct {
	void *new_db;				// current cache database
	void *old_db;				// older cache database
	void *store;				// persistent database
	size_t old_ram;				// cache size
	size_t new_ram;				// cache size
	time_t retired;				// start time of older
	time_t killed;				// deathtime of older
	time_t lifespan;			// lifetime of older
	UINT added;					// items added
} cache;

struct tree_key {
	BYTE sn[8];
	union {
		const struct filetype *ft;
		const char *nm;
		const struct connection_in *in;
	} p;
	int extension;
};

struct tree_node {
	struct tree_key tk;
	time_t expires;
	int dsize;
};

/* Bad bad C library */
/* implementation of tfind, tsearch returns an opaque structure */
/* you have to know that the first element is a pointer to your data */
struct tree_opaque {
	struct tree_node *key;
	void *other;
};

#define TREE_DATA(tn)    ( (BYTE *)(tn) + sizeof(struct tree_node) )

static int Cache_Add_Common(struct tree_node *tn);
static int Cache_Add_Store(struct tree_node *tn);

static int Cache_Get_Common(void *data, size_t * dsize, time_t duration,
							const struct tree_node *tn);
static int Cache_Get_Common_Dir(struct dirblob *db, time_t duration,
								const struct tree_node *tn);
static int Cache_Get_Store(void *data, size_t * dsize, time_t duration,
						   const struct tree_node *tn);

static int Cache_Del_Common(const struct tree_node *tn);
static int Cache_Del_Store(const struct tree_node *tn);

static int Add_Stat(struct cache *scache, const int result);
static int Get_Stat(struct cache *scache, const int result);
static int Del_Stat(struct cache *scache, const int result);
static int tree_compare(const void *a, const void *b);
static time_t TimeOut(const enum fc_change change);

static int tree_compare(const void *a, const void *b)
{
	return memcmp(&((const struct tree_node *) a)->tk,
				  &((const struct tree_node *) b)->tk,
				  sizeof(struct tree_key));
}

static time_t TimeOut(const enum fc_change change)
{
	switch (change) {
	case fc_second:
	case fc_persistent:		/* arbitrary non-zero */
		return 1;
	case fc_volatile:
	case fc_Avolatile:
		return Global.timeout_volatile;
	case fc_stable:
	case fc_Astable:
		return Global.timeout_stable;
	case fc_presence:
		return Global.timeout_presence;
	case fc_directory:
		return Global.timeout_directory;
	default:					/* static or statistic */
		return 0;
	}
}

#ifdef CACHE_DEBUG
/* debug routine -- shows a table */
/* Run it as twalk(dababase, tree_show ) */
static void node_show(struct tree_node *tn)
{
	int i;
	char b[26];
	ctime_r(&tn->expires, b);
	printf("\tNode " SNformat
		   " pointer=%p extension=%d length=%d start=%p expires=%s",
		   SNvar(tn->tk.sn), tn->tk.p.in, tn->tk.extension, tn->dsize, tn,
		   b ? b : "null\n");
	for (i = 0; i < sizeof(struct tree_key); ++i)
		printf("%.2X ", ((uint8_t *) tn)[i]);
	printf("\n");
}

static void tree_show(const void *node, const VISIT which, const int depth)
{
	const struct tree_node *tn = *(struct tree_node * *) node;
	(void) depth;
	if (node) {
		switch (which) {
		case leaf:
		case postorder:
			node_show(tn);
			// fall through
		default:
			break;
		}
	} else {
		printf("Node empty\n");
	}
}
static void new_tree(void)
{
	printf("Walk the new tree:\n");
	twalk(cache.new_db, tree_show);
}
#else							/* CACHE_DEBUG */
#define new_tree(x)
#define node_show(tn)
#endif							/* CACHE_DEBUG */

/* DB cache creation code */
	/* Note: done in single-threaded mode so locking not yet needed */
void Cache_Open(void)
{
	cache.new_db = NULL;
	cache.old_db = NULL;
	cache.old_ram = 0;
	cache.new_ram = 0;
	cache.store = NULL;
	cache.lifespan = TimeOut(fc_stable);
	if (cache.lifespan > 3600)
		cache.lifespan = 3600;	/* 1 hour tops */
	cache.retired = time(NULL);
	cache.killed = cache.retired + cache.lifespan;
}

	/* Note: done in single-threaded mode so locking not yet needed */
void Cache_Close(void)
{
	tdestroy(cache.new_db, free);
	tdestroy(cache.old_db, free);
	tdestroy(cache.store, free);
}

static int Add_Stat(struct cache *scache, const int result)
{
	if (result == 0) {
		STAT_ADD1(scache->adds);
	}
	return result;
}

/* Add an item to the cache */
/* return 0 if good, 1 if not */
int Cache_Add(const void *data, const size_t datasize,
			  const struct parsedname *pn)
{
	if (pn && NotAlarmDir(pn)) {	// do check here to avoid needless processing
		time_t duration = TimeOut(pn->ft->change);
		if (duration > 0) {
			struct tree_node *tn =
				(struct tree_node *) malloc(sizeof(struct tree_node) +
											datasize);
			if (tn) {
				memset(&tn->tk, 0, sizeof(struct tree_key));
				memcpy(tn->tk.sn, pn->sn, 8);
				tn->tk.p.ft = pn->ft;
				tn->tk.extension = pn->extension;
				tn->expires = duration + time(NULL);
				tn->dsize = datasize;
                if ( datasize ) memcpy(TREE_DATA(tn), data, datasize);
				switch (pn->ft->change) {
				case fc_persistent:
					return Add_Stat(&cache_sto, Cache_Add_Store(tn));
				default:
					return Add_Stat(&cache_ext, Cache_Add_Common(tn));
				}
			}
			return -ENOMEM;
		}
	}
	return 0;
}

/* Add a directory entry to the cache */
/* return 0 if good, 1 if not */
int Cache_Add_Dir(const struct dirblob *db, const struct parsedname *pn)
{
	time_t duration = TimeOut(fc_directory);
	size_t size = db->devices * 8;
	//printf("Cache_Add_Dir\n") ;
	if (duration > 0) {			/* in case timeout set to 0 */
		struct tree_node *tn =
			(struct tree_node *) malloc(sizeof(struct tree_node) + size);
		//printf("AddDir tn=%p\n",tn) ;
		if (tn) {
			FS_LoadPath(tn->tk.sn, pn);
			tn->tk.p.in = pn->in;
			tn->tk.extension = 0;
			tn->expires = duration + time(NULL);
			tn->dsize = size;
            if ( size ) memcpy(TREE_DATA(tn), db->snlist, size);
			return Add_Stat(&cache_dir, Cache_Add_Common(tn));
		}
	}
	return -ENOMEM;
}

/* Add a device entry to the cache */
/* return 0 if good, 1 if not */
int Cache_Add_Device(const int bus_nr, const struct parsedname *pn)
{
	time_t duration = TimeOut(fc_presence);
	//printf("Cache_Add_Device\n") ;
	if (duration > 0) {			/* in case timeout set to 0 */
		struct tree_node *tn =
			(struct tree_node *) malloc(sizeof(struct tree_node) +
										sizeof(int));
		if (tn) {
			memset(&tn->tk, 0, sizeof(struct tree_key));
			memcpy(tn->tk.sn, pn->sn, 8);
			tn->tk.p.in = NULL;	// value connected to all in-devices
			//tn->tk.p.in = pn->in ;
			tn->tk.extension = -1;
			tn->expires = duration + time(NULL);
			tn->dsize = sizeof(int);
			memcpy(TREE_DATA(tn), &bus_nr, sizeof(int));
			return Add_Stat(&cache_dev, Cache_Add_Common(tn));
		}
	}
	return -ENOMEM;
}

/* Add an item to the cache */
/* return 0 if good, 1 if not */
int Cache_Add_Internal(const void *data, const size_t datasize,
					   const struct internal_prop *ip,
					   const struct parsedname *pn)
{
	//printf("Cache_Add_Internal\n") ;
	if (pn) {					// do check here to avoid needless processing
		time_t duration = TimeOut(ip->change);
		if (duration > 0) {
			struct tree_node *tn =
				(struct tree_node *) malloc(sizeof(struct tree_node) +
											datasize);
			if (tn) {
				memset(&tn->tk, 0, sizeof(struct tree_key));
				memcpy(tn->tk.sn, pn->sn, 8);
				tn->tk.p.nm = ip->name;
				tn->tk.extension = -2;
				tn->expires = duration + time(NULL);
				tn->dsize = datasize;
                if ( datasize) memcpy(TREE_DATA(tn), data, datasize);
				//printf("ADD INTERNAL name= %s size=%d \n",tn->tk.p.nm,tn->dsize);
				//printf("  ADD INTERNAL data[0]=%d size=%d \n",((BYTE *)data)[0],datasize);
				switch (ip->change) {
				case fc_persistent:
					return Add_Stat(&cache_sto, Cache_Add_Store(tn));
				default:
					return Add_Stat(&cache_int, Cache_Add_Common(tn));
				}
			}
			return -ENOMEM;
		}
	}
	return 0;
}

/* Add an item to the cache */
/* retire the cache (flip) if too old, and start a new one (keep the old one for a while) */
/* return 0 if good, 1 if not */
static int Cache_Add_Common(struct tree_node *tn)
{
	struct tree_opaque *opaque;
	enum { no_add, yes_add, just_update } state = no_add;
	void *flip = NULL;
	//printf("Cache_Add_Common\n") ;
	node_show(tn);
	LEVEL_DEBUG("Add to cache sn " SNformat " in=%p index=%d size=%d\n",
				SNvar(tn->tk.sn), tn->tk.p.in, tn->tk.extension,
				tn->dsize);
	CACHELOCK;
	if (cache.killed < time(NULL)) {	// old database has timed out
		flip = cache.old_db;
		/* Flip caches! old = new. New truncated, reset time and counters and flag */
		cache.old_db = cache.new_db;
		cache.old_ram = cache.new_ram;
		cache.new_db = NULL;
		cache.new_ram = 0;
		cache.added = 0;
		cache.retired = time(NULL);
		cache.killed = cache.retired + cache.lifespan;
	}
	if (Global.cache_size
		&& (cache.old_ram + cache.new_ram > Global.cache_size)) {
		// failed size test
		free(tn);
	} else if ((opaque = tsearch(tn, &cache.new_db, tree_compare))) {
		//printf("Cache_Add_Common to %p\n",opaque) ;
		if (tn != opaque->key) {
			cache.new_ram += sizeof(tn) - sizeof(opaque->key);
			free(opaque->key);
			opaque->key = tn;
			state = just_update;
		} else {
			state = yes_add;
			cache.new_ram += sizeof(tn);
		}
	} else {					// nothing found or added?!? free our memory segment
		free(tn);
	}
	CACHEUNLOCK;
	/* flipped old database is now out of circulation -- can be destroyed without a lock */
	if (flip) {
		tdestroy(flip, free);
		STATLOCK;
		++cache_flips;			/* statistics */
		memcpy(&old_avg, &new_avg, sizeof(struct average));
		AVERAGE_CLEAR(&new_avg)
			STATUNLOCK;
		//printf("FLIP points to: %p\n",flip);
	}
	/* Added or updated, update statistics */
	switch (state) {
	case yes_add:
		//printf("CACHECommon: Yes add\n");
		STATLOCK;
		AVERAGE_IN(&new_avg)
			++ cache_adds;		/* statistics */
		STATUNLOCK;
		return 0;
	case just_update:
		//printf("CACHECommon: Yes update\n");
		STATLOCK;
		AVERAGE_MARK(&new_avg)
			++ cache_adds;		/* statistics */
		STATUNLOCK;
		return 0;
	default:
		//printf("CACHECommon: Error\n");
		return 1;
	}
}

/* Clear the cache (a change was made that might give stale information) */
void Cache_Clear(void)
{
	void *c_new = NULL;
	void *c_old = NULL;
	CACHELOCK;
	c_old = cache.old_db;
	c_new = cache.new_db;
	cache.old_db = NULL;
	cache.new_db = NULL;
	cache.old_ram = 0;
	cache.new_ram = 0;
	cache.added = 0;
	cache.retired = time(NULL);
	cache.killed = cache.retired + cache.lifespan;
	CACHEUNLOCK;
	/* flipped old database is now out of circulation -- can be destroyed without a lock */
	if (c_new)
		tdestroy(c_new, free);
	if (c_old)
		tdestroy(c_old, free);
}

/* Add an item to the cache */
/* retire the cache (flip) if too old, and start a new one (keep the old one for a while) */
/* return 0 if good, 1 if not */
static int Cache_Add_Store(struct tree_node *tn)
{
	struct tree_opaque *opaque;
	enum { no_add, yes_add, just_update } state = no_add;
	//printf("Cache_Add_Store\n") ;
	STORELOCK;
	if ((opaque = tsearch(tn, &cache.store, tree_compare))) {
		//printf("CACHE ADD pointer=%p, key=%p\n",tn,opaque->key);
		if (tn != opaque->key) {
			free(opaque->key);
			opaque->key = tn;
			state = just_update;
		} else {
			state = yes_add;
		}
	} else {					// nothing found or added?!? free our memory segment
		free(tn);
	}
	STOREUNLOCK;
	switch (state) {
	case yes_add:
		STATLOCK;
		AVERAGE_IN(&store_avg)
			STATUNLOCK;
		return 0;
	case just_update:
		STATLOCK;
		AVERAGE_MARK(&store_avg)
			STATUNLOCK;
		return 0;
	default:
		return 1;
	}
}

static int Get_Stat(struct cache *scache, const int result)
{
	//printf("Get_Stat\n") ;
	STATLOCK;
	if (result == 0) {
		++scache->hits;
	} else if (result == -ETIMEDOUT) {
		++scache->expires;
	}
	++scache->tries;
	STATUNLOCK;
	return result;
}

/* Does cache get, but doesn't allow play in data size */
int Cache_Get_Strict(void *data, size_t dsize, const struct parsedname *pn)
{
	size_t size = dsize;
	if (Cache_Get(data, &size, pn) || dsize != size)
		return 1;
	return 0;
}

/* Look in caches, 0=found and valid, 1=not or uncachable in the first place */
int Cache_Get(void *data, size_t * dsize, const struct parsedname *pn)
{
	//printf("Cache_Get\n") ;
	if (pn && IsUncachedDir(pn) && NotAlarmDir(pn)) {	// do check here to avoid needless processing
		time_t duration = TimeOut(pn->ft->change);
		if (duration > 0) {
			struct tree_node tn;
			memset(&tn.tk, 0, sizeof(struct tree_key));
			memcpy(tn.tk.sn, pn->sn, 8);
			tn.tk.p.ft = pn->ft;
			tn.tk.extension = pn->extension;
			switch (pn->ft->change) {
			case fc_persistent:
				return Get_Stat(&cache_sto,
								Cache_Get_Store(data, dsize, duration,
												&tn));
			default:
				return Get_Stat(&cache_ext,
								Cache_Get_Common(data, dsize, duration,
												 &tn));
			}
		}
	}
	return 1;
}

/* Look in caches, 0=found and valid, 1=not or uncachable in the first place */
int Cache_Get_Dir(struct dirblob *db, const struct parsedname *pn)
{
	time_t duration = TimeOut(fc_directory);
	DirblobInit(db);
	//printf("Cache_Get_Dir\n") ;
	if (duration > 0) {
		struct tree_node tn;
		//printf("GetDir tn=%p\n",tn) ;
		memset(&tn.tk, 0, sizeof(struct tree_key));
		FS_LoadPath(tn.tk.sn, pn);
		tn.tk.p.in = pn->in;
		tn.tk.extension = 0;
		return Get_Stat(&cache_dir,
						Cache_Get_Common_Dir(db, duration, &tn));
	}
	return 1;
}

/* Look in caches, 0=found and valid, 1=not or uncachable in the first place */
static int Cache_Get_Common_Dir(struct dirblob *db, time_t duration,
								const struct tree_node *tn)
{
	int ret;
	time_t now = time(NULL);
	size_t size;
	struct tree_opaque *opaque;
	//printf("Cache_Get_Common_Dir\n") ;
	LEVEL_DEBUG("Get from cache sn " SNformat " pointer=%p extension=%d\n",
				SNvar(tn->tk.sn), tn->tk.p.in, tn->tk.extension);
	CACHELOCK;
	if ((opaque = tfind(tn, &cache.new_db, tree_compare))
		|| ((cache.retired + duration > now)
			&& (opaque = tfind(tn, &cache.old_db, tree_compare)))
		) {
		LEVEL_DEBUG("Found in cache\n");
		if (opaque->key->expires >= now) {
			size = opaque->key->dsize;
			if ((db->snlist = (BYTE *) malloc(size))) {
				memcpy(db->snlist, TREE_DATA(opaque->key), size);
				db->allocated = db->devices = size / 8;
				//printf("Cache: snlist=%p, devices=%lu, size=%lu\n",*snlist,devices[0],size) ;
				ret = 0;
			} else {
				ret = -ENOMEM;
			}
		} else {
			//char b[26];
			//printf("GOT DEAD now:%s",ctime_r(&now,b)) ;
			//printf("        then:%s",ctime_r(&opaque->key->expires,b)) ;
			LEVEL_DEBUG("Expired in cache\n");
			ret = -ETIMEDOUT;
		}
	} else {
		LEVEL_DEBUG("Found in cache\n");
		ret = -ENOENT;
	}
	CACHEUNLOCK;
	return ret;
}

/* Look in caches, 0=found and valid, 1=not or uncachable in the first place */
int Cache_Get_Device(void *bus_nr, const struct parsedname *pn)
{
	time_t duration = TimeOut(fc_presence);
	//printf("Cache_Get_Device\n") ;
	if (duration > 0) {
		size_t size = sizeof(int);
		struct tree_node tn;
		memset(&tn.tk, 0, sizeof(struct tree_key));
		memcpy(tn.tk.sn, pn->sn, 8);
		tn.tk.p.in = NULL;		// value connected to all in-devices
		tn.tk.extension = -1;
		return Get_Stat(&cache_dev,
						Cache_Get_Common(bus_nr, &size, duration, &tn));
	}
	return 1;
}

/* Does cache get, but doesn't allow play in data size */
int Cache_Get_Internal_Strict(void *data, size_t dsize,
							  const struct internal_prop *ip,
							  const struct parsedname *pn)
{
	size_t size = dsize;
	if (Cache_Get_Internal(data, &size, ip, pn) || dsize != size)
		return 1;
	return 0;
}

/* Look in caches, 0=found and valid, 1=not or uncachable in the first place */
int Cache_Get_Internal(void *data, size_t * dsize,
					   const struct internal_prop *ip,
					   const struct parsedname *pn)
{
	//printf("Cache_Get_Internal\n") ;
	if (pn) {					// do check here to avoid needless processing
		time_t duration = TimeOut(ip->change);
		if (duration > 0) {
			struct tree_node tn;
			memset(&tn.tk, 0, sizeof(struct tree_key));
			memcpy(tn.tk.sn, pn->sn, 8);
			tn.tk.p.nm = ip->name;
			tn.tk.extension = -2;
			switch (ip->change) {
			case fc_persistent:
				return Get_Stat(&cache_sto,
								Cache_Get_Store(data, dsize, duration,
												&tn));
			default:
				return Get_Stat(&cache_int,
								Cache_Get_Common(data, dsize, duration,
												 &tn));
			}
		}
	}
	return 1;
}

/* Look in caches, 0=found and valid, 1=not or uncachable in the first place */
static int Cache_Get_Common(void *data, size_t * dsize, time_t duration,
							const struct tree_node *tn)
{
	int ret;
	time_t now = time(NULL);
	struct tree_opaque *opaque;
	//printf("Cache_Get_Common\n") ;
	LEVEL_DEBUG("Get from cache sn " SNformat " in=%p index=%d size=%lu\n",
				SNvar(tn->tk.sn), tn->tk.p.in, tn->tk.extension, dsize[0]);
	node_show(tn);
	//printf("\nTree (new):\n");
	new_tree();
	//printf("\nTree (old):\n");
	new_tree();
	CACHELOCK;
	if ((opaque = tfind(tn, &cache.new_db, tree_compare))
		|| ((cache.retired + duration > now)
			&& (opaque = tfind(tn, &cache.old_db, tree_compare)))
		) {
		//printf("CACHE GET 2 opaque=%p tn=%p\n",opaque,opaque->key);
		LEVEL_DEBUG("Found in cache\n");
		if (opaque->key->expires >= now) {
			//printf("CACHE GET 3 buffer size=%lu stored size=%d\n",*dsize,opaque->key->dsize);
			if ((ssize_t) dsize[0] >= opaque->key->dsize) {
				//printf("CACHE GET 4\n");
				dsize[0] = opaque->key->dsize;
				//tree_show(opaque,leaf,0);
				//printf("CACHE GET 5 size=%lu\n",*dsize);
                if ( dsize[0] ) memcpy(data, TREE_DATA(opaque->key), dsize[0] );
				ret = 0;
				//printf("CACHE GOT\n");
				//twalk(cache.new_db,tree_show) ;
			} else {
				ret = -EMSGSIZE;
			}
		} else {
			//char b[26];
			//printf("GOT DEAD now:%s",ctime_r(&now,b)) ;
			//printf("        then:%s",ctime_r(&opaque->key->expires,b)) ;
			LEVEL_DEBUG("Expired in cache\n");
			ret = -ETIMEDOUT;
		}
	} else {
		LEVEL_DEBUG("Not found in cache\n");
		ret = -ENOENT;
	}
	CACHEUNLOCK;
	return ret;
}

/* Look in caches, 0=found and valid, 1=not or uncachable in the first place */
static int Cache_Get_Store(void *data, size_t * dsize, time_t duration,
						   const struct tree_node *tn)
{
	struct tree_opaque *opaque;
	int ret;
	(void) duration;
	//printf("Cache_Get_Store\n") ;
	STORELOCK;
	if ((opaque = tfind(tn, &cache.store, tree_compare))) {
		if ((ssize_t) dsize[0] >= opaque->key->dsize) {
			dsize[0] = opaque->key->dsize;
            if ( dsize[0] ) memcpy(data, TREE_DATA(opaque->key), dsize[0] );
			ret = 0;
		} else {
			ret = -EMSGSIZE;
		}
	} else {
		ret = -ENOENT;
	}
	STOREUNLOCK;
	return ret;
}

static int Del_Stat(struct cache *scache, const int result)
{
	if (result == 0) {
		STAT_ADD1(scache->deletes);
	}
	return result;
}

int Cache_Del(const struct parsedname *pn)
{
	if (pn) {					// do check here to avoid needless processing
		time_t duration = TimeOut(pn->ft->change);
		if (duration > 0) {
			struct tree_node tn;
			memset(&tn.tk, 0, sizeof(struct tree_key));
			memcpy(tn.tk.sn, pn->sn, 8);
			tn.tk.p.ft = pn->ft;
			tn.tk.extension = pn->extension;
			switch (pn->ft->change) {
			case fc_persistent:
				return Del_Stat(&cache_sto, Cache_Del_Store(&tn));
			default:
				return Del_Stat(&cache_ext, Cache_Del_Common(&tn));
			}
		}
	}
	return 1;
}

int Cache_Del_Dir(const struct parsedname *pn)
{
	time_t duration = TimeOut(fc_directory);
	if (duration > 0) {
		struct tree_node tn;
		memset(&tn.tk, 0, sizeof(struct tree_key));
		FS_LoadPath(tn.tk.sn, pn);
		tn.tk.p.in = pn->in;
		tn.tk.extension = 0;
		return Del_Stat(&cache_dir, Cache_Del_Common(&tn));
	}
	return 1;
}

int Cache_Del_Device(const struct parsedname *pn)
{
	time_t duration = TimeOut(fc_presence);
	if (duration > 0) {
		struct tree_node tn;
		memset(&tn.tk, 0, sizeof(struct tree_key));
		memcpy(tn.tk.sn, pn->sn, 8);
		tn.tk.p.in = pn->in;
		tn.tk.extension = -1;
		return Del_Stat(&cache_dev, Cache_Del_Common(&tn));
	}
	return 1;
}

int Cache_Del_Internal(const struct internal_prop *ip,
					   const struct parsedname *pn)
{
	if (pn) {					// do check here to avoid needless processing
		time_t duration = TimeOut(ip->change);
		if (duration > 0) {
			struct tree_node tn;
			memset(&tn.tk, 0, sizeof(struct tree_key));
			memcpy(tn.tk.sn, pn->sn, 8);
			tn.tk.p.nm = ip->name;
			tn.tk.extension = 0;
			switch (ip->change) {
			case fc_persistent:
				return Del_Stat(&cache_sto, Cache_Del_Store(&tn));
			default:
				return Del_Stat(&cache_int, Cache_Del_Common(&tn));
			}
		}
	}
	return 1;
}

static int Cache_Del_Common(const struct tree_node *tn)
{
	struct tree_opaque *opaque;
	time_t now = time(NULL);
	int ret = 1;
	LEVEL_DEBUG("Delete from cache sn " SNformat " in=%p index=%d\n",
				SNvar(tn->tk.sn), tn->tk.p.in, tn->tk.extension);
	CACHELOCK;
	if ((opaque = tfind(tn, &cache.new_db, tree_compare))
		|| ((cache.killed > now)
			&& (opaque = tfind(tn, &cache.old_db, tree_compare)))
		) {
		opaque->key->expires = now - 1;
		ret = 0;
	}
	CACHEUNLOCK;
	return ret;
}

static int Cache_Del_Store(const struct tree_node *tn)
{
	struct tree_opaque *opaque;
	struct tree_node *tn_found = NULL;
	STORELOCK;
	if ((opaque = tfind(tn, &cache.store, tree_compare))) {
		tn_found = opaque->key;
		tdelete(tn, &cache.store, tree_compare);
	}
	STOREUNLOCK;
	if (tn_found) {
		free(tn_found);
		STATLOCK;
		AVERAGE_OUT(&store_avg)
			STATUNLOCK;
		return 0;
	}
	return 1;
}

#endif							/* OW_CACHE */
