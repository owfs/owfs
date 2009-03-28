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
#include "ow_connection.h"

//#define CACHE_DEBUG

#if OW_CACHE
#include <limits.h>

#define EXTENSION_DEVICE	-1
#define EXTENSION_INTERNAL	-2

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
	time_t cooked_now;			// special "now" for ignoring volatile and simultaneous
	UINT added;					// items added
} cache;

/* Key used for sorting/retrieving cache data
   sn is for device serial number
   p is a pointer to filetype, or other things (guaranteed unique and fast lookup
   extension is used for indexed array properties
*/
struct tree_key {
	BYTE sn[8];
	void *p;
	int extension;
};

/* How we organize the data in the binary tree used for cache storage
   A key (see above)
   An expiration time
   And a size in bytes
   Actaully size bytes follows for the data
*/
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

static int Cache_Get_Common(void *data, size_t * dsize, time_t duration, const struct tree_node *tn);
static int Cache_Get_Common_Dir(struct dirblob *db, time_t duration, const struct tree_node *tn);
static int Cache_Get_Store(void *data, size_t * dsize, time_t duration, const struct tree_node *tn);

static int Cache_Del_Common(const struct tree_node *tn);
static int Cache_Del_Store(const struct tree_node *tn);

static int Add_Stat(struct cache *scache, const int result);
static int Get_Stat(struct cache *scache, const int result);
static int Del_Stat(struct cache *scache, const int result);
static int tree_compare(const void *a, const void *b);
static time_t TimeOut(const enum fc_change change);

/* used for the sort/search b-tree routines */
static int tree_compare(const void *a, const void *b)
{
	return memcmp(&((const struct tree_node *) a)->tk, &((const struct tree_node *) b)->tk, sizeof(struct tree_key));
}

/* Gives the delay for a given property type */
static time_t TimeOut(const enum fc_change change)
{
	switch (change) {
	case fc_second:
	case fc_persistent:		/* arbitrary non-zero */
		return 1;
	case fc_volatile:
	case fc_Avolatile:
		return Globals.timeout_volatile;
	case fc_stable:
	case fc_Astable:
		return Globals.timeout_stable;
	case fc_presence:
		return Globals.timeout_presence;
	case fc_directory:
		return Globals.timeout_directory;
	case fc_alias:
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
	fprintf(stderr,"\tNode " SNformat
		   " pointer=%p extension=%d length=%d start=%p expires=%s", SNvar(tn->tk.sn), tn->tk.p, tn->tk.extension, tn->dsize, tn, b ? b : "null\n");
	for (i = 0; i < sizeof(struct tree_key); ++i)
		printf("%.2X ", ((uint8_t *) tn)[i]);
	fprintf(stderr,"\n");
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
		fprintf(stderr,"Node empty\n");
	}
}
static void new_tree(void)
{
	fprintf(stderr,"Walk the new tree:\n");
	twalk(cache.new_db, tree_show);
}
#else							/* CACHE_DEBUG */
#define new_tree()
#define node_show(tn)
#endif							/* CACHE_DEBUG */

/* DB cache creation code */
/* Note: done in single-threaded mode so locking not yet needed */
void Cache_Open(void)
{
	memset(&cache, 0, sizeof(struct cache));

	cache.lifespan = TimeOut(fc_stable);
	if (cache.lifespan > 3600) {
		cache.lifespan = 3600;	/* 1 hour tops */
	}
	cache.retired = time(NULL);
	cache.killed = cache.retired + cache.lifespan;
	cache.cooked_now = time(NULL);	// current time, or in the future for simultaneous */
}

/* Note: done in single-threaded mode so locking not yet needed */
void Cache_Close(void)
{
	tdestroy(cache.new_db, free);
	cache.new_db = NULL;
	tdestroy(cache.old_db, free);
	cache.old_db = NULL;
	tdestroy(cache.store, free);
	cache.store = NULL;
}

static int Add_Stat(struct cache *scache, const int result)
{
	if (result == 0) {
		STAT_ADD1(scache->adds);
	}
	return result;
}

int OWQ_Cache_Add(const struct one_wire_query *owq)
{
	const struct parsedname *pn = PN(owq);
	if (pn->extension == EXTENSION_ALL) {
		switch (pn->selected_filetype->format) {
		case ft_ascii:
		case ft_vascii:
		case ft_binary:
			return 1;			// cache of string arrays not supported
		case ft_integer:
		case ft_unsigned:
		case ft_yesno:
		case ft_date:
		case ft_float:
		case ft_temperature:
		case ft_tempgap:
			return Cache_Add(OWQ_array(owq), (pn->selected_filetype->ag->elements) * sizeof(union value_object), pn);
		default:
			return 1;
		}
	} else {
		switch (pn->selected_filetype->format) {
		case ft_ascii:
		case ft_vascii:
		case ft_binary:
			if (OWQ_offset(owq) > 0) {
				return 1;
			}
			return Cache_Add(OWQ_buffer(owq), OWQ_length(owq), pn);
		case ft_integer:
		case ft_unsigned:
		case ft_yesno:
		case ft_date:
		case ft_float:
		case ft_temperature:
		case ft_tempgap:
			return Cache_Add(&OWQ_val(owq), sizeof(union value_object), pn);
		default:
			return 1;
		}
	}
}


/* Add an item to the cache */
/* return 0 if good, 1 if not */
int Cache_Add(const void *data, const size_t datasize, const struct parsedname *pn)
{
	struct tree_node *tn;
	time_t duration;

	if (!pn || IsAlarmDir(pn)) {
		return 0;				// do check here to avoid needless processing
	}

	duration = TimeOut(pn->selected_filetype->change);
	if (duration <= 0) {
		return 0;				/* in case timeout set to 0 */
	}

	tn = (struct tree_node *) owmalloc(sizeof(struct tree_node) + datasize);
	if (!tn) {
		return -ENOMEM;
	}
	memset(&tn->tk, 0, sizeof(struct tree_key));

	LEVEL_DEBUG(SNformat " size=%d\n", SNvar(pn->sn), (int) datasize);
	memcpy(tn->tk.sn, pn->sn, 8);
	tn->tk.p = pn->selected_filetype;
	tn->tk.extension = pn->extension;
	tn->expires = duration + time(NULL);
	tn->dsize = datasize;
	if (datasize) {
		memcpy(TREE_DATA(tn), data, datasize);
	}
	switch (pn->selected_filetype->change) {
	case fc_persistent:
		return Add_Stat(&cache_sto, Cache_Add_Store(tn));
	default:
		return Add_Stat(&cache_ext, Cache_Add_Common(tn));
	}
}

/* Add a directory entry to the cache */
/* return 0 if good, 1 if not */
int Cache_Add_Dir(const struct dirblob *db, const struct parsedname *pn)
{
	time_t duration = TimeOut(fc_directory);
	struct tree_node *tn;
	size_t size = DirblobElements(db) * 8;
	struct parsedname pn_directory;
	//printf("Cache_Add_Dir\n") ;
	if (pn==NULL || pn->selected_connection==NULL) {
		return 0;				// do check here to avoid needless processing
	}

	switch ( pn->selected_connection->busmode ) {
		case bus_fake:
		case bus_tester:
		case bus_mock:
		case bus_w1:
		case bus_bad:
		case bus_unknown:
			return 0 ;
		default:
			break ;
	}

	if (duration <= 0) {
		return 0;				/* in case timeout set to 0 */
	}

	tn = (struct tree_node *) owmalloc(sizeof(struct tree_node) + size);
	//printf("AddDir tn=%p\n",tn) ;
	if (!tn) {
		return -ENOMEM;
	}
	memset(&tn->tk, 0, sizeof(struct tree_key));

	LEVEL_DEBUG(SNformat " elements=%d\n", SNvar(pn->sn), DirblobElements(db));
	FS_LoadDirectoryOnly(&pn_directory, pn);
	memcpy(tn->tk.sn, pn_directory.sn, 8);
	tn->tk.p = pn->selected_connection;
	tn->tk.extension = 0;
	tn->expires = duration + time(NULL);
	tn->dsize = size;
	if (size) {
		memcpy(TREE_DATA(tn), db->snlist, size);
	}
	return Add_Stat(&cache_dir, Cache_Add_Common(tn));
}

/* Add a device entry to the cache */
/* return 0 if good, 1 if not */
int Cache_Add_Device(const int bus_nr, const BYTE * sn)
{
	time_t duration = TimeOut(fc_presence);
	struct tree_node *tn;
	//printf("Cache_Add_Device\n");

	if (duration <= 0) {
		return 0;				/* in case timeout set to 0 */
	}

	tn = (struct tree_node *) owmalloc(sizeof(struct tree_node) + sizeof(int));
	if (!tn) {
		return -ENOMEM;
	}
	memset(&tn->tk, 0, sizeof(struct tree_key));

	LEVEL_DEBUG(SNformat " bus=%d\n", SNvar(sn), (int) bus_nr);
	memcpy(tn->tk.sn, sn, 8);
	tn->tk.p = NULL;			// value connected to all in-devices
	//tn->tk.p.selected_connection = pn->selected_connection ;
	tn->tk.extension = EXTENSION_DEVICE;
	tn->expires = duration + time(NULL);
	tn->dsize = sizeof(int);
	memcpy(TREE_DATA(tn), &bus_nr, sizeof(int));
	return Add_Stat(&cache_dev, Cache_Add_Common(tn));
}

/* What do we cache?
Type       sn             extension             p         What
directory  0=root         0                     *in       dirblob
           DS2409_branch  0                     *in       dirblob
device     device sn      EXTENSION_DEVICE=-1   NULL      bus_nr
internal   device sn      EXTENSION_INTERNAL=-2 ip->name  binary data
property   device sn      extension             *ft       binary data
*/

/* Add an item to the cache */
/* return 0 if good, 1 if not */
int Cache_Add_Internal(const void *data, const size_t datasize, const struct internal_prop *ip, const struct parsedname *pn)
{
	struct tree_node *tn;
	time_t duration;
	//printf("Cache_Add_Internal\n");
	if (!pn) {
		return 0;				// do check here to avoid needless processing
	}

	duration = TimeOut(ip->change);
	if (duration <= 0) {
		return 0;				/* in case timeout set to 0 */
	}

	tn = (struct tree_node *) owmalloc(sizeof(struct tree_node) + datasize);
	if (!tn) {
		return -ENOMEM;
	}
	memset(&tn->tk, 0, sizeof(struct tree_key));

	LEVEL_DEBUG(SNformat " size=%d\n", SNvar(pn->sn), (int) datasize);
	memcpy(tn->tk.sn, pn->sn, 8);
	tn->tk.p = ip->name;
	tn->tk.extension = EXTENSION_INTERNAL;
	tn->expires = duration + time(NULL);
	tn->dsize = datasize;
	if (datasize) {
		memcpy(TREE_DATA(tn), data, datasize);
	}
	//printf("ADD INTERNAL name= %s size=%d \n",tn->tk.p.nm,tn->dsize);
	//printf("  ADD INTERNAL data[0]=%d size=%d \n",((BYTE *)data)[0],datasize);
	switch (ip->change) {
	case fc_persistent:
		return Add_Stat(&cache_sto, Cache_Add_Store(tn));
	default:
		return Add_Stat(&cache_int, Cache_Add_Common(tn));
	}
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
	LEVEL_DEBUG("Add to cache sn " SNformat " pointer=%p index=%d size=%d\n", SNvar(tn->tk.sn), tn->tk.p, tn->tk.extension, tn->dsize);
	CACHE_WLOCK;
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
	if (Globals.cache_size && (cache.old_ram + cache.new_ram > Globals.cache_size)) {
		// failed size test
		owfree(tn);
	} else if ((opaque = tsearch(tn, &cache.new_db, tree_compare))) {
		//printf("Cache_Add_Common to %p\n",opaque);
		if (tn != opaque->key) {
			cache.new_ram += sizeof(tn) - sizeof(opaque->key);
			owfree(opaque->key);
			opaque->key = tn;
			state = just_update;
		} else {
			state = yes_add;
			cache.new_ram += sizeof(tn);
		}
	} else {					// nothing found or added?!? free our memory segment
		owfree(tn);
	}
	CACHE_WUNLOCK;
	/* flipped old database is now out of circulation -- can be destroyed without a lock */
	if (flip) {
		LEVEL_DEBUG("flip cache. tdestroy() will be called.\n");
		tdestroy(flip, free);
		STATLOCK;
		++cache_flips;			/* statistics */
		memcpy(&old_avg, &new_avg, sizeof(struct average));
		AVERAGE_CLEAR(&new_avg);
		STATUNLOCK;
		//printf("FLIP points to: %p\n",flip);
	}
	/* Added or updated, update statistics */
	switch (state) {
	case yes_add:
		//printf("CACHECommon: Yes add\n");
		STATLOCK;
		AVERAGE_IN(&new_avg);
		++cache_adds;			/* statistics */
		STATUNLOCK;
		return 0;
	case just_update:
		//printf("CACHECommon: Yes update\n");
		STATLOCK;
		AVERAGE_MARK(&new_avg);
		++cache_adds;			/* statistics */
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
	CACHE_WLOCK;
	c_old = cache.old_db;
	c_new = cache.new_db;
	cache.old_db = NULL;
	cache.new_db = NULL;
	cache.old_ram = 0;
	cache.new_ram = 0;
	cache.added = 0;
	cache.retired = time(NULL);
	cache.killed = cache.retired + cache.lifespan;
	CACHE_WUNLOCK;
	/* flipped old database is now out of circulation -- can be destroyed without a lock */
	if (c_new) {
		tdestroy(c_new, free);
	}
	if (c_old) {
		tdestroy(c_old, free);
	}
}

/* Add an item to the cache */
/* retire the cache (flip) if too old, and start a new one (keep the old one for a while) */
/* return 0 if good, 1 if not */
static int Cache_Add_Store(struct tree_node *tn)
{
	struct tree_opaque *opaque;
	enum { no_add, yes_add, just_update } state = no_add;
	//printf("Cache_Add_Store\n") ;
	STORE_WLOCK;
	if ((opaque = tsearch(tn, &cache.store, tree_compare))) {
		//printf("CACHE ADD pointer=%p, key=%p\n",tn,opaque->key);
		if (tn != opaque->key) {
			owfree(opaque->key);
			opaque->key = tn;
			state = just_update;
		} else {
			state = yes_add;
		}
	} else {					// nothing found or added?!? free our memory segment
		owfree(tn);
	}
	STORE_WUNLOCK;
	switch (state) {
	case yes_add:
		STATLOCK;
		AVERAGE_IN(&store_avg);
		STATUNLOCK;
		return 0;
	case just_update:
		STATLOCK;
		AVERAGE_MARK(&store_avg);
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
	if (Cache_Get(data, &size, pn) || dsize != size) {
		return 1;
	}
	return 0;
}


int OWQ_Cache_Get(struct one_wire_query *owq)
{
	struct parsedname *pn = PN(owq);
	if (pn->extension == EXTENSION_ALL) {
		switch (pn->selected_filetype->format) {
		case ft_ascii:
		case ft_vascii:
		case ft_binary:
			return 1;			// string arrays not supported
		case ft_integer:
		case ft_unsigned:
		case ft_yesno:
		case ft_date:
		case ft_float:
		case ft_temperature:
		case ft_tempgap:
			return Cache_Get_Strict(OWQ_array(owq), (pn->selected_filetype->ag->elements) * sizeof(union value_object), pn);
		default:
			return 1;
		}
	} else {
		switch (pn->selected_filetype->format) {
		case ft_ascii:
		case ft_vascii:
		case ft_binary:
			if (OWQ_offset(owq) > 0) {
				return 1;
			}
			OWQ_length(owq) = OWQ_size(owq);
			return Cache_Get(OWQ_buffer(owq), &OWQ_length(owq), pn);
		case ft_integer:
		case ft_unsigned:
		case ft_yesno:
		case ft_date:
		case ft_float:
		case ft_temperature:
		case ft_tempgap:
			return Cache_Get_Strict(&OWQ_val(owq), sizeof(union value_object), pn);
		default:
			return 1;
		}
	}
}


/* Look in caches, 0=found and valid, 1=not or uncachable in the first place */
int Cache_Get(void *data, size_t * dsize, const struct parsedname *pn)
{
	time_t duration;
	struct tree_node tn;
	//printf("Cache_Get\n") ;
	// do check here to avoid needless processing
	if (IsUncachedDir(pn) || IsAlarmDir(pn)) {
		return 1;
	}

	duration = TimeOut(pn->selected_filetype->change);
	if (duration <= 0) {
		return 1;
	}

	LEVEL_DEBUG(SNformat " size=%d IsUncachedDir=%d\n", SNvar(pn->sn), (int) dsize[0], IsUncachedDir(pn));
	memset(&tn.tk, 0, sizeof(struct tree_key));
	memcpy(tn.tk.sn, pn->sn, 8);
	tn.tk.p = pn->selected_filetype;
	tn.tk.extension = pn->extension;
	switch (pn->selected_filetype->change) {
	case fc_persistent:
		return Get_Stat(&cache_sto, Cache_Get_Store(data, dsize, duration, &tn));
	default:
		return Get_Stat(&cache_ext, Cache_Get_Common(data, dsize, duration, &tn));
	}
}

/* Look in caches, 0=found and valid, 1=not or uncachable in the first place */
int Cache_Get_Dir(struct dirblob *db, const struct parsedname *pn)
{
	time_t duration = TimeOut(fc_directory);
	struct tree_node tn;
	struct parsedname pn_directory;
	DirblobInit(db);
	//printf("Cache_Get_Dir\n") ;
	if (duration <= 0) {
		return 1;
	}

	LEVEL_DEBUG(SNformat "\n", SNvar(pn->sn));
	//printf("GetDir tn=%p\n",tn) ;
	memset(&tn.tk, 0, sizeof(struct tree_key));
	FS_LoadDirectoryOnly(&pn_directory, pn);
	memcpy(tn.tk.sn, pn_directory.sn, 8);
	tn.tk.p = pn->selected_connection;
	tn.tk.extension = 0;
	return Get_Stat(&cache_dir, Cache_Get_Common_Dir(db, duration, &tn));
}

/* Look in caches, 0=found and valid, 1=not or uncachable in the first place */
static int Cache_Get_Common_Dir(struct dirblob *db, time_t duration, const struct tree_node *tn)
{
	int ret;
	time_t now = time(NULL);
	size_t size;
	struct tree_opaque *opaque;
	//printf("Cache_Get_Common_Dir\n") ;
	LEVEL_DEBUG("Get from cache sn " SNformat " pointer=%p extension=%d\n", SNvar(tn->tk.sn), tn->tk.p, tn->tk.extension);
	CACHE_RLOCK;
	if ((opaque = tfind(tn, &cache.new_db, tree_compare))
		|| ((cache.retired + duration > now)
			&& (opaque = tfind(tn, &cache.old_db, tree_compare)))
		) {
		LEVEL_DEBUG("dir found in cache\n");
		if (opaque->key->expires >= now) {
			size = opaque->key->dsize;
			if (DirblobRecreate(TREE_DATA(opaque->key), size, db) == 0) {
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
		LEVEL_DEBUG("dir not found in cache\n");
		ret = -ENOENT;
	}
	CACHE_RUNLOCK;
	return ret;
}

/* Look in caches, 0=found and valid, 1=not or uncachable in the first place */
int Cache_Get_Device(void *bus_nr, const struct parsedname *pn)
{
	time_t duration = TimeOut(fc_presence);
	size_t size = sizeof(int);
	struct tree_node tn;
	//printf("Cache_Get_Device\n") ;
	if (duration <= 0) {
		return 1;
	}

	LEVEL_DEBUG(SNformat "\n", SNvar(pn->sn));
	memset(&tn.tk, 0, sizeof(struct tree_key));
	memcpy(tn.tk.sn, pn->sn, 8);
	tn.tk.p = NULL;				// value connected to all in-devices
	tn.tk.extension = EXTENSION_DEVICE;
	return Get_Stat(&cache_dev, Cache_Get_Common(bus_nr, &size, duration, &tn));
}

/* Does cache get, but doesn't allow play in data size */
int Cache_Get_Internal_Strict(void *data, size_t dsize, const struct internal_prop *ip, const struct parsedname *pn)
{
	size_t size = dsize;
	if (Cache_Get_Internal(data, &size, ip, pn) || dsize != size) {
		return 1;
	}
	return 0;
}

/* Look in caches, 0=found and valid, 1=not or uncachable in the first place */
int Cache_Get_Internal(void *data, size_t * dsize, const struct internal_prop *ip, const struct parsedname *pn)
{
	struct tree_node tn;
	time_t duration;
	//printf("Cache_Get_Internal\n");
	if (!pn) {
		return 1;				// do check here to avoid needless processing
	}

	duration = TimeOut(ip->change);
	if (duration <= 0) {
		return 1;				/* in case timeout set to 0 */
	}

	LEVEL_DEBUG(SNformat " size=%d\n", SNvar(pn->sn), (int) dsize[0]);
	memset(&tn.tk, 0, sizeof(struct tree_key));
	memcpy(tn.tk.sn, pn->sn, 8);
	tn.tk.p = ip->name;
	tn.tk.extension = EXTENSION_INTERNAL;
	switch (ip->change) {
	case fc_persistent:
		return Get_Stat(&cache_sto, Cache_Get_Store(data, dsize, duration, &tn));
	default:
		return Get_Stat(&cache_int, Cache_Get_Common(data, dsize, duration, &tn));
	}
}

/* Do the processing for finding the correct "current" time
   if device presence (tn.tk.p==NULL) use now
   if cache.cooked_now < now, use now
   else used cache.cooked_now
*/
static time_t Cooked_Now(const struct tree_node *tn)
{
	time_t now = time(NULL);	// true current time
	if (tn->tk.p == NULL) {
		return now;				// device presence
	}
	if (cache.cooked_now < now) {
		return now;				// not post-dated
	}
	return cache.cooked_now;
}

/* "Post-date" the current time to invalidate all volatile properties
   used when Simultaneous is done to prevent conflicts
*/
void CookTheCache(void)
{
	CACHE_WLOCK;
	cache.cooked_now = time(NULL) + TimeOut(fc_volatile);
	CACHE_WUNLOCK;
}

/* Look in caches, 0=found and valid, 1=not or uncachable in the first place */
static int Cache_Get_Common(void *data, size_t * dsize, time_t duration, const struct tree_node *tn)
{
	int ret;
	time_t now = Cooked_Now(tn);
	struct tree_opaque *opaque;
	//printf("Cache_Get_Common\n") ;
	LEVEL_DEBUG("Get from cache sn " SNformat " pointer=%p index=%d size=%lu\n", SNvar(tn->tk.sn), tn->tk.p, tn->tk.extension, dsize[0]);
	node_show(tn);
	//printf("\nTree (new):\n");
	new_tree();
	//printf("\nTree (old):\n");
	new_tree();
	CACHE_RLOCK;
	if ((opaque = tfind(tn, &cache.new_db, tree_compare))
		|| ((cache.retired + duration > now)
			&& (opaque = tfind(tn, &cache.old_db, tree_compare)))
		) {
		//printf("CACHE GET 2 opaque=%p tn=%p\n",opaque,opaque->key);
		LEVEL_DEBUG("value found in cache\n");
		if (opaque->key->expires >= now) {
			//printf("CACHE GET 3 buffer size=%lu stored size=%d\n",*dsize,opaque->key->dsize);
			if ((ssize_t) dsize[0] >= opaque->key->dsize) {
				//printf("CACHE GET 4\n");
				dsize[0] = opaque->key->dsize;
				//tree_show(opaque,leaf,0);
				//printf("CACHE GET 5 size=%lu\n",*dsize);
				if (dsize[0]) {
					memcpy(data, TREE_DATA(opaque->key), dsize[0]);
				}
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
		LEVEL_DEBUG("value not found in cache\n");
		ret = -ENOENT;
	}
	CACHE_RUNLOCK;
	return ret;
}

/* Look in caches, 0=found and valid, 1=not or uncachable in the first place */
static int Cache_Get_Store(void *data, size_t * dsize, time_t duration, const struct tree_node *tn)
{
	struct tree_opaque *opaque;
	int ret;
	(void) duration;
	//printf("Cache_Get_Store\n") ;
	STORE_RLOCK;
	if ((opaque = tfind(tn, &cache.store, tree_compare))) {
		if ((ssize_t) dsize[0] >= opaque->key->dsize) {
			dsize[0] = opaque->key->dsize;
			if (dsize[0]) {
				memcpy(data, TREE_DATA(opaque->key), dsize[0]);
			}
			ret = 0;
		} else {
			ret = -EMSGSIZE;
		}
	} else {
		ret = -ENOENT;
	}
	STORE_RUNLOCK;
	return ret;
}

static int Del_Stat(struct cache *scache, const int result)
{
	if (result == 0) {
		STAT_ADD1(scache->deletes);
	}
	return result;
}

int OWQ_Cache_Del(const struct one_wire_query *owq)
{
	return Cache_Del(PN(owq));
}

int Cache_Del(const struct parsedname *pn)
{
	struct tree_node tn;
	time_t duration;
	//printf("Cache_Del\n") ;
	if (!pn) {
		return 1;				// do check here to avoid needless processing
	}

	duration = TimeOut(pn->selected_filetype->change);
	if (duration <= 0) {
		return 1;				/* in case timeout set to 0 */
	}

	memset(&tn.tk, 0, sizeof(struct tree_key));
	memcpy(tn.tk.sn, pn->sn, 8);
	tn.tk.p = pn->selected_filetype;
	tn.tk.extension = pn->extension;
	switch (pn->selected_filetype->change) {
	case fc_persistent:
		return Del_Stat(&cache_sto, Cache_Del_Store(&tn));
	default:
		return Del_Stat(&cache_ext, Cache_Del_Common(&tn));
	}
}

int Cache_Del_Dir(const struct parsedname *pn)
{
	struct tree_node tn;
	struct parsedname pn_directory;
	time_t duration = TimeOut(fc_directory);
	if (duration <= 0) {
		return 1;
	}

	memset(&tn.tk, 0, sizeof(struct tree_key));
	FS_LoadDirectoryOnly(&pn_directory, pn);
	memcpy(tn.tk.sn, pn_directory.sn, 8);
	tn.tk.p = pn->selected_connection;
	tn.tk.extension = 0;
	return Del_Stat(&cache_dir, Cache_Del_Common(&tn));
}

int Cache_Del_Device(const struct parsedname *pn)
{
	struct tree_node tn;
	time_t duration = TimeOut(fc_presence);
	if (duration <= 0) {
		return 1;
	}

	memset(&tn.tk, 0, sizeof(struct tree_key));
	memcpy(tn.tk.sn, pn->sn, 8);
	tn.tk.p = pn->selected_connection;
	tn.tk.extension = EXTENSION_DEVICE;
	return Del_Stat(&cache_dev, Cache_Del_Common(&tn));
}

int Cache_Del_Internal(const struct internal_prop *ip, const struct parsedname *pn)
{
	struct tree_node tn;
	time_t duration;
	//printf("Cache_Del_Internal\n") ;
	if (!pn) {
		return 1;				// do check here to avoid needless processing
	}

	duration = TimeOut(ip->change);
	if (duration <= 0) {
		return 1;				/* in case timeout set to 0 */
	}

	memset(&tn.tk, 0, sizeof(struct tree_key));
	memcpy(tn.tk.sn, pn->sn, 8);
	tn.tk.p = ip->name;
	tn.tk.extension = 0;
	switch (ip->change) {
	case fc_persistent:
		return Del_Stat(&cache_sto, Cache_Del_Store(&tn));
	default:
		return Del_Stat(&cache_int, Cache_Del_Common(&tn));
	}
}

static int Cache_Del_Common(const struct tree_node *tn)
{
	struct tree_opaque *opaque;
	time_t now = time(NULL);
	int ret = 1;
	LEVEL_DEBUG("Delete from cache sn " SNformat " in=%p index=%d\n", SNvar(tn->tk.sn), tn->tk.p, tn->tk.extension);
	CACHE_WLOCK;
	if ((opaque = tfind(tn, &cache.new_db, tree_compare))
		|| ((cache.killed > now)
			&& (opaque = tfind(tn, &cache.old_db, tree_compare)))
		) {
		opaque->key->expires = now - 1;
		ret = 0;
	}
	CACHE_WUNLOCK;
	return ret;
}

static int Cache_Del_Store(const struct tree_node *tn)
{
	struct tree_opaque *opaque;
	struct tree_node *tn_found = NULL;
	STORE_WLOCK;
	if ((opaque = tfind(tn, &cache.store, tree_compare))) {
		tn_found = opaque->key;
		tdelete(tn, &cache.store, tree_compare);
	}
	STORE_WUNLOCK;
	if (!tn_found) {
		return 1;
	}

	owfree(tn_found);
	STATLOCK;
	AVERAGE_OUT(&store_avg);
	STATUNLOCK;
	return 0;
}

/* Cache to another property from the same device,
   e.g. a flag status like IAD for vis measurement in the DS2438
   Some things are different
   1. Bus is known (since the original device is already determined)
   2. Read is local
   3. Bus is already locked
   4. Device is locked

   owq_shallow_copy comes in as a single, not aggregate, even if the original value was an aggregate
*/
void FS_cache_sibling(char *property, struct one_wire_query *owq_shallow_copy)
{
	struct parsedname *pn = PN(owq_shallow_copy);	// already set up with native device and property

	if (FS_ParseProperty_for_sibling(property, pn)) {
		return;
	}

	/* There are a few types that are not supported: aggregates (.ALL) */
	if (pn->extension == EXTENSION_ALL) {
		return;
	}

	OWQ_Cache_Add(owq_shallow_copy);
}

#endif							/* OW_CACHE */
