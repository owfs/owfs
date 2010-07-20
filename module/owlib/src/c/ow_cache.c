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

#define EXTENSION_INTERNAL  -2

// Directories are bus-specific, but buses are dynamic
// The numbering is sequential, however, so use that and an arbitrary
// generic unique address for directories.
int DirMarkerLoc ;
void * Directory_Marker = &DirMarkerLoc ;

// Devices are sn-based
// generic unique address for devices.
int DevMarkerLoc ;
void * Device_Marker = &DevMarkerLoc ;

// Aliases are sn-based
// generic unique address for devices.
int AliasMarkerLoc ;
void * Alias_Marker = &AliasMarkerLoc ;

// Simultaneous Dir are bus-based
// generic unique address for Simultaneous.
int SimulMarkerLoc[simul_end] ;
void * Simul_Marker[] = { &SimulMarkerLoc[simul_temp], &SimulMarkerLoc[simul_volt], } ;

/* Put the globals into a struct to declutter the namespace */
struct cache_data {
	void *temporary_tree_new;				// current cache database
	void *temporary_tree_old;				// older cache database
	void *permanent_tree;				// persistent database
	size_t old_ram;				// cache size
	size_t new_ram;				// cache size
	time_t retired;				// start time of older
	time_t killed;				// deathtime of older
	time_t lifespan;			// lifetime of older
	UINT added;					// items added
};
static struct cache_data cache;

/* Cache elements are placed in a Red/Black binary tree
	-- standard glibc implementation
	-- use gnu tdestroy extension
	-- compatibility implementation included
  Cache key has 3 components: (struct tree_key)
	sn -- 8 byte serial number of 1-wire slave
	p -- pointer to internal structure like filetype (guaranteed unique if non-portable)
	extension -- integer used for array elements
  Cache node is the entire node not including data payload (struct key_node)
	key -- sorted component as above
	expires -- the time that the element is no longer valid
	dsize -- length in bytes of trailing data
  Cache data is the actual data
	allocated at same call as cache node
	access via macro TREE_DATA
	freed when cache node is freed
  Note: This means that cache data must be a copy of program data
        both on creation and retrieval
*/

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
   Actaully size bytes follows with the data
*/
struct tree_node {
	struct tree_key tk;
	time_t expires;
	size_t dsize;
};


/* Bad bad C library */
/* implementation of tfind, tsearch returns an opaque structure */
/* you have to know that the first element is a pointer to your data */
struct tree_opaque {
	struct tree_node *key;
	void *other;
};

#define TREE_DATA(tn)    ( (BYTE *)(tn) + sizeof(struct tree_node) )
#define CONST_TREE_DATA(tn)    ( (const BYTE *)(tn) + sizeof(struct tree_node) )

static void * GetFlippedTree( void ) ;
static void DeleteFlippedTree( void * retired_tree ) ;

static int Cache_Type_Store( const struct parsedname * pn ) ;

static int Cache_Add_Common(struct tree_node *tn);
static int Cache_Add_Store(struct tree_node *tn);

static int Cache_Get_Common(void *data, size_t * dsize, time_t * duration, const struct tree_node *tn);
static int Cache_Get_Common_Dir(struct dirblob *db, time_t * duration, const struct tree_node *tn);
static int Cache_Get_Store(void *data, size_t * dsize, time_t * duration, const struct tree_node *tn);
static int Cache_Get_Simultaneous(enum simul_type type, struct one_wire_query *owq) ;

static int Cache_Del_Common(const struct tree_node *tn);
static int Cache_Del_Store(const struct tree_node *tn);

static int Add_Stat(struct cache *scache, const int result);
static int Get_Stat(struct cache *scache, const int result);
static int Del_Stat(struct cache *scache, const int result);
static int tree_compare(const void *a, const void *b);
static time_t TimeOut(const enum fc_change change);
static void Aliasfindaction(const void *node, const VISIT which, const int depth) ;
static void LoadTK( const BYTE * sn, void * p, int extension, struct tree_node * tn ) ;

/* used for the sort/search b-tree routines */
/* big voodoo pointer fuss to just do a standard memory compare of the "key" */
static int tree_compare(const void *a, const void *b)
{
	return memcmp(&((const struct tree_node *) a)->tk, &((const struct tree_node *) b)->tk, sizeof(struct tree_key));
}

/* Gives the delay for a given property type */
/* Values in seconds (as defined in Globals structure and modified by command line and "settings") */
static time_t TimeOut(const enum fc_change change)
{
	switch (change) {
	case fc_second:
	case fc_persistent:		/* arbitrary non-zero */
		return 1;
	case fc_volatile:
	case fc_Avolatile:
	case fc_simultaneous_temperature:
	case fc_simultaneous_voltage:
		return Globals.timeout_volatile;
	case fc_stable:
	case fc_read_stable:
	case fc_Astable:
		return Globals.timeout_stable;
	case fc_presence:
		return Globals.timeout_presence;
	case fc_directory:
		return Globals.timeout_directory;
	case fc_link:
	case fc_page:
	case fc_subdir:
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
	for (i = 0; i < sizeof(struct tree_key); ++i) {
		fprintf(stderr,"%.2X ", ((uint8_t *) tn)[i]);
	}
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
	twalk(cache.temporary_tree_new, tree_show);
}
#else							/* CACHE_DEBUG */
#define new_tree()
#define node_show(tn)
#endif							/* CACHE_DEBUG */

static int Cache_Type_Store( const struct parsedname * pn )
{
	return (pn->selected_filetype->change==fc_persistent) || (pn->selected_connection->busmode==bus_mock) ;
}

/* DB cache creation code */
/* Note: done in single-threaded mode so locking not yet needed */
void Cache_Open(void)
{
	memset(&cache, 0, sizeof(struct cache_data));

	cache.lifespan = TimeOut(fc_stable);
	if (cache.lifespan > 3600) {
		cache.lifespan = 3600;	/* 1 hour tops */
	}
	cache.retired = time(NULL);
	cache.killed = cache.retired + cache.lifespan;
}

/* Note: done in a simgle single thread mode so locking not needed */
void Cache_Close(void)
{
	Cache_Clear() ;
	SAFETDESTROY( cache.permanent_tree, owfree_func);
}

/* Clear the cache (a change was made that might give stale information) */
void Cache_Clear(void)
{
	void *c_new = NULL;
	void *c_old = NULL;
	CACHE_WLOCK;
	c_old = cache.temporary_tree_old;
	c_new = cache.temporary_tree_new;
	cache.temporary_tree_old = NULL;
	cache.temporary_tree_new = NULL;
	cache.old_ram = 0;
	cache.new_ram = 0;
	cache.added = 0;
	cache.retired = time(NULL);
	cache.killed = cache.retired + cache.lifespan;
	CACHE_WUNLOCK;
	/* flipped old database is now out of circulation -- can be destroyed without a lock */
	SAFETDESTROY( c_new, owfree_func);
	SAFETDESTROY( c_old, owfree_func);
}

/* Wrapper to perform a cache function and add statistics */
static int Add_Stat(struct cache *scache, const int result)
{
	if (result == 0) {
		STAT_ADD1(scache->adds);
	}
	return result;
}

/* Higher level add of a one-wire-query object */
int OWQ_Cache_Add(const struct one_wire_query *owq)
{
	const struct parsedname *pn = PN(owq);
	if (pn->extension == EXTENSION_ALL) {
		switch (pn->selected_filetype->format) {
		case ft_ascii:
		case ft_vascii:
		case ft_alias:
		case ft_binary:
			return 1;			// cache of string arrays not supported
		case ft_integer:
		case ft_unsigned:
		case ft_yesno:
		case ft_date:
		case ft_float:
		case ft_pressure:
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
		case ft_alias:
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
		case ft_pressure:
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

	// allocate space for the node and data
	tn = (struct tree_node *) owmalloc(sizeof(struct tree_node) + datasize);
	if (!tn) {
		return -ENOMEM;
	}

	LEVEL_DEBUG(SNformat " size=%d", SNvar(pn->sn), (int) datasize);

	// populate the node structure with data
	LoadTK( pn->sn, pn->selected_filetype, pn->extension, tn );
	tn->expires = duration + time(NULL);
	tn->dsize = datasize;
	if (datasize) {
		memcpy(TREE_DATA(tn), data, datasize);
	}
	return Cache_Type_Store(pn)?
		Add_Stat(&cache_sto, Cache_Add_Store(tn)) :
		Add_Stat(&cache_ext, Cache_Add_Common(tn)) ;
}

/* Add a directory entry to the cache */
/* return 0 if good, 1 if not */
int Cache_Add_Dir(const struct dirblob *db, const struct parsedname *pn)
{
	time_t duration = TimeOut(fc_directory);
	struct tree_node *tn;
	size_t size = DirblobElements(db) * SERIAL_NUMBER_SIZE;
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
	
	// allocate space for the node and data
	tn = (struct tree_node *) owmalloc(sizeof(struct tree_node) + size);
	if (!tn) {
		return -ENOMEM;
	}
	
	LEVEL_DEBUG(SNformat " elements=%d", SNvar(pn->sn), DirblobElements(db));
	
	// populate node with directory name and dirblob
	FS_LoadDirectoryOnly(&pn_directory, pn);
	LoadTK( pn_directory.sn, Directory_Marker, pn->selected_connection->index, tn );
	tn->expires = duration + time(NULL);
	tn->dsize = size;
	if (size) {
		memcpy(TREE_DATA(tn), db->snlist, size);
	}
	return Add_Stat(&cache_dir, Cache_Add_Common(tn));
}

/* Add a Simultaneous entry to the cache */
/* return 0 if good, 1 if not */
int Cache_Add_Simul(const enum simul_type type, const struct parsedname *pn)
{
	time_t duration = TimeOut(fc_volatile);
	struct tree_node *tn;
	struct parsedname pn_directory;
	//printf("Cache_Add_Dir\n") ;
	if (pn==NULL || pn->selected_connection==NULL) {
		return 0;				// do check here to avoid needless processing
	}
	
	switch ( pn->selected_connection->busmode ) {
		case bus_fake:
		case bus_tester:
		case bus_mock:
		case bus_bad:
		case bus_unknown:
			return 0 ;
		default:
			break ;
	}
	
	if (duration <= 0) {
		return 0;				/* in case timeout set to 0 */
	}
	
	// allocate space for the node and data
	tn = (struct tree_node *) owmalloc(sizeof(struct tree_node));
	if (!tn) {
		return -ENOMEM;
	}
	
	LEVEL_DEBUG(SNformat, SNvar(pn->sn));
	
	// populate node with directory name and dirblob
	FS_LoadDirectoryOnly(&pn_directory, pn);
	LoadTK( pn_directory.sn, Simul_Marker[type], pn->selected_connection->index, tn) ;
	LEVEL_DEBUG("Simultaneous add type=%d",type);
	tn->expires = duration + time(NULL);
	tn->dsize = 0;
	return Add_Stat(&cache_dir, Cache_Add_Common(tn));
}

/* Add a device entry to the cache */
/* return 0 if good, 1 if not */
int Cache_Add_Device(const int bus_nr, const BYTE * sn)
{
	time_t duration = TimeOut(fc_presence);
	struct tree_node *tn;

	if (duration <= 0) {
		return 0;				/* in case timeout set to 0 */
	}

	if ( sn[0] == 0 ) { //bad serial number
		return 0 ;
	}

	tn = (struct tree_node *) owmalloc(sizeof(struct tree_node) + sizeof(int));
	if (!tn) {
		return -ENOMEM;
	}

	LEVEL_DEBUG(SNformat " bus=%d", SNvar(sn), (int) bus_nr);
	LoadTK(sn, Device_Marker, 0, tn );
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

	LEVEL_DEBUG(SNformat " size=%d", SNvar(pn->sn), (int) datasize);
	LoadTK( pn->sn, ip->name, EXTENSION_INTERNAL, tn );
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
/* return 0 if good, 1 if not */
int Cache_Add_Alias(const ASCII *name, const BYTE * sn)
{
	struct tree_node *tn;
	size_t size = strlen(name) ;

	tn = (struct tree_node *) owmalloc(sizeof(struct tree_node) + size + 1 );
	if (!tn) {
		return -ENOMEM;
	}

	LEVEL_DEBUG("Adding " SNformat " alias=%s", SNvar(sn), name);
	LoadTK( sn, Alias_Marker, 0, tn );
	tn->expires = time(NULL);
	tn->dsize = size+1;
	strcpy((ASCII *)TREE_DATA(tn), name);
	return Add_Stat(&cache_sto, Cache_Add_Store(tn));
}

/* Moves new to old tree, initializes new tree, and returns former old tree location */
static void * GetFlippedTree( void )
{
	void * flip = cache.temporary_tree_old;
	/* Flip caches! old = new. New truncated, reset time and counters and flag */
	cache.temporary_tree_old = cache.temporary_tree_new;
	cache.old_ram = cache.new_ram;
	cache.temporary_tree_new = NULL;
	cache.new_ram = 0;
	cache.added = 0;
	cache.retired = time(NULL);
	cache.killed = cache.retired + cache.lifespan;
	return flip ;
}

static void DeleteFlippedTree( void * retired_tree )
{
	LEVEL_DEBUG("flip cache. tdestroy() will be called.");
	SAFETDESTROY( retired_tree, owfree_func);
	STATLOCK;
	++cache_flips;			/* statistics */
	memcpy(&old_avg, &new_avg, sizeof(struct average));
	AVERAGE_CLEAR(&new_avg);
	STATUNLOCK;
}

/* Add an item to the cache */
/* retire the cache (flip) if too old, and start a new one (keep the old one for a while) */
/* return 0 if good, 1 if not */
static int Cache_Add_Common(struct tree_node *tn)
{
	struct tree_opaque *opaque;
	enum { no_add, yes_add, just_update } state = no_add;
	void *retired_tree = NULL; // used a a flag for cache flip

	node_show(tn);
	LEVEL_DEBUG("Add to cache sn " SNformat " pointer=%p index=%d size=%d", SNvar(tn->tk.sn), tn->tk.p, tn->tk.extension, tn->dsize);
	CACHE_WLOCK;
	if (cache.killed < time(NULL)) {	// old database has timed out
		retired_tree = GetFlippedTree() ;
	}
	if (Globals.cache_size && (cache.old_ram + cache.new_ram > Globals.cache_size)) {
		// failed size test
		owfree(tn);
	} else if ((opaque = tsearch(tn, &cache.temporary_tree_new, tree_compare))) {
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
	if (retired_tree) {
		DeleteFlippedTree( retired_tree ) ;
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

/* Add an item to the cache */
/* retire the cache (flip) if too old, and start a new one (keep the old one for a while) */
/* return 0 if good, 1 if not */
static int Cache_Add_Store(struct tree_node *tn)
{
	struct tree_opaque *opaque;
	enum { no_add, yes_add, just_update } state = no_add;
	//printf("Cache_Add_Store\n") ;
	STORE_WLOCK;
	if ((opaque = tsearch(tn, &cache.permanent_tree, tree_compare))) {
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

	// do check here to avoid needless processing
	if (IsUncachedDir(pn) || IsAlarmDir(pn)) {
		return 1;
	}

	switch (pn->selected_filetype->change) {
	case fc_simultaneous_temperature:
		return Cache_Get_Simultaneous(simul_temp, owq) ;
	case fc_simultaneous_voltage:
		return Cache_Get_Simultaneous(simul_volt, owq) ;
	default:
		break ;
	}

	if (pn->extension == EXTENSION_ALL) {
		switch (pn->selected_filetype->format) {
		case ft_ascii:
		case ft_vascii:
		case ft_alias:
		case ft_binary:
			return 1;			// string arrays not supported
		case ft_integer:
		case ft_unsigned:
		case ft_yesno:
		case ft_date:
		case ft_float:
		case ft_pressure:
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
		case ft_alias:
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
		case ft_pressure:
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

	LEVEL_DEBUG(SNformat " size=%d IsUncachedDir=%d", SNvar(pn->sn), (int) dsize[0], IsUncachedDir(pn));
	LoadTK( pn->sn, pn->selected_filetype, pn->extension, &tn );
	return Cache_Type_Store(pn) ?
		Get_Stat(&cache_sto, Cache_Get_Store(data, dsize, &duration, &tn)) :
		Get_Stat(&cache_ext, Cache_Get_Common(data, dsize, &duration, &tn));
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

	LEVEL_DEBUG(SNformat, SNvar(pn->sn));
	FS_LoadDirectoryOnly(&pn_directory, pn);
	LoadTK( pn_directory.sn, Directory_Marker, pn->selected_connection->index, &tn) ;
	return Get_Stat(&cache_dir, Cache_Get_Common_Dir(db, &duration, &tn));
}

/* Look in caches, 0=found and valid, 1=not or uncachable in the first place */
static int Cache_Get_Common_Dir(struct dirblob *db, time_t * duration, const struct tree_node *tn)
{
	int ret;
	time_t now = time(NULL);
	size_t size;
	struct tree_opaque *opaque;
	LEVEL_DEBUG("Get from cache sn " SNformat " pointer=%p extension=%d", SNvar(tn->tk.sn), tn->tk.p, tn->tk.extension);
	CACHE_RLOCK;
	if ((opaque = tfind(tn, &cache.temporary_tree_new, tree_compare))
		|| ((cache.retired + duration[0] > now)
			&& (opaque = tfind(tn, &cache.temporary_tree_old, tree_compare)))
		) {
		LEVEL_DEBUG("dir found in cache");
		duration[0] = opaque->key->expires - now ;
		if (duration[0] >= 0) {
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
			LEVEL_DEBUG("Expired in cache");
			ret = -ETIMEDOUT;
		}
	} else {
		LEVEL_DEBUG("dir not found in cache");
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

	LEVEL_DEBUG(SNformat, SNvar(pn->sn));
	LoadTK( pn->sn, Device_Marker, 0, &tn ) ;
	return Get_Stat(&cache_dev, Cache_Get_Common(bus_nr, &size, &duration, &tn));
}

/* Does cache get, but doesn't allow play in data size */
GOOD_OR_BAD Cache_Get_Internal_Strict(void *data, size_t dsize, const struct internal_prop *ip, const struct parsedname *pn)
{
	size_t size = dsize;
	if (Cache_Get_Internal(data, &size, ip, pn) || dsize != size) {
		return gbBAD;
	}
	return gbGOOD;
}

/* Look in caches, 0=found and valid, 1=not or uncachable in the first place */
int Cache_Get_Internal(void *data, size_t * dsize, const struct internal_prop *ip, const struct parsedname *pn)
{
	struct tree_node tn;
	time_t duration;
	//printf("Cache_Get_Internal");
	if (!pn) {
		return 1;				// do check here to avoid needless processing
	}
	
	duration = TimeOut(ip->change);
	if (duration <= 0) {
		return 1;				/* in case timeout set to 0 */
	}
	
	LEVEL_DEBUG(SNformat " size=%d", SNvar(pn->sn), (int) dsize[0]);
	LoadTK( pn->sn, ip->name, EXTENSION_INTERNAL, &tn) ;
	switch (ip->change) {
		case fc_persistent:
			return Get_Stat(&cache_sto, Cache_Get_Store(data, dsize, &duration, &tn));
		default:
			return Get_Stat(&cache_int, Cache_Get_Common(data, dsize, &duration, &tn));
	}
}

/* Test for a  simultaneous property
If the property isn't independently cached, return false (1)
If the simultaneous conversion is more recent, return false (1)
Else return the cached value and true (0)
*/
int Cache_Get_Simul_Time(enum simul_type type, time_t * start_time, const struct parsedname * pn)
{
	// valid cached primary data -- see if a simultaneous conversion should be used instead
	struct tree_node tn;
	time_t duration ;
	time_t duration_simul ;
	size_t dsize_simul = 0 ;
	struct parsedname pn_directory ;

	duration = duration_simul = TimeOut(ipSimul[type].change);
	if (duration_simul <= 0) {
		return 1;
	}
	
	FS_LoadDirectoryOnly(&pn_directory, pn);
	LoadTK(pn_directory.sn, Simul_Marker[type], pn->selected_connection->index, &tn ) ;
	if ( Get_Stat(&cache_int, Cache_Get_Common(NULL, &dsize_simul, &duration_simul, &tn)) ) {
		return 1 ;
	}
	start_time[0] = duration_simul - duration + time(NULL) ;
	return 0 ;
}

/* Test for a simultaneous property
If the property isn't independently cached, return false (1)
If the simultaneous conversion is more recent, return false (1)
Else return the cached value and true (0)
*/
static int Cache_Get_Simultaneous(enum simul_type type, struct one_wire_query *owq)
{
	struct tree_node tn;
	time_t duration;
	struct parsedname * pn = PN(owq) ;
	size_t dsize = sizeof(union value_object) ;
	
	duration = TimeOut(pn->selected_filetype->change);
	if (duration <= 0) {
		return 1;
	}
	
	LoadTK( pn->sn, pn->selected_filetype, pn->extension, &tn ) ;
	
	if ( Get_Stat(&cache_ext, Cache_Get_Common(&OWQ_val(owq), &dsize, &duration, &tn)) == 0 ) {
		// valid cached primary data -- see if a simultaneous conversion should be used instead
		time_t start_time ;
		time_t duration_simul;
		
		duration_simul = TimeOut(ipSimul[type].change);
		if (duration_simul <= 0) {
			return 0; /* use cached property */
		}
		
		if ( Cache_Get_Simul_Time(type,&start_time,pn) ) {
			// Simul not found
			LEVEL_DEBUG("Simultaneous conversion not found.") ;
			return 0 ;
		}
		if ( start_time-time(NULL)+duration_simul > duration ) {
			LEVEL_DEBUG("Simultaneous conversion is newer than previous reading.") ;
			return 1 ; // Simul is newer
		}
		// Cached data is newer, so use it
		LEVEL_DEBUG("Simultaneous conversion is older than actual reading.") ;
		return 0 ;
	}
	// fall through -- no cached primary data so simultaneous is irrelevant
	return 1 ;
}

/* Look in caches, 0=found and valid, 1=not or uncachable in the first place */
/* space already allocated in buffer */
int Cache_Get_Alias(ASCII * name, size_t length, const BYTE * sn)
{
	struct tree_node tn;
	struct tree_opaque *opaque;
	int ret = -ENOENT ;

	LoadTK(sn, Alias_Marker, 0, &tn ) ;

	STORE_RLOCK;
	if ((opaque = tfind(&tn, &cache.permanent_tree, tree_compare))) {
		if ( opaque->key->dsize < length ) {
			strncpy(name,(ASCII *)TREE_DATA(opaque->key),length);
			ret = 0 ;
			LEVEL_DEBUG("Retrieving " SNformat " alias=%s", SNvar(sn), SAFESTRING(name) );
		} else {
			ret = -EMSGSIZE ;
		}
	}
	STORE_RUNLOCK;
	return ret ;
}

struct {
	const ASCII *name;
	size_t dsize ;
	BYTE * sn;
	int ret ;
} global_aliasfind_struct;

static void Aliasfindaction(const void *node, const VISIT which, const int depth)
{
	const struct tree_node *p = *(struct tree_node * const *) node;
	(void) depth;
	if ( global_aliasfind_struct.ret==0 ) {
		return ;
	}

	//printf("Comparing %s|%s with %s\n",p->name ,p->code , Namefindname ) ;
	switch (which) {
	case leaf:
	case postorder:
		if ( p->tk.p != Alias_Marker ) {
			return ;
		}
		//printf("Compare %s and %s\n",global_aliasfind_struct.name,(const ASCII *)CONST_TREE_DATA(p));
		if ( p->dsize != global_aliasfind_struct.dsize ) {
			return ;
		}
		if ( memcmp(global_aliasfind_struct.name,(const ASCII *)CONST_TREE_DATA(p),global_aliasfind_struct.dsize) ) {
			return ;
		}
		global_aliasfind_struct.ret = 0 ;
		memcpy(global_aliasfind_struct.sn,p->tk.sn,SERIAL_NUMBER_SIZE) ;
		return ;
	case preorder:
	case endorder:
		break;
	}
}
/* Look in caches, 0=found and valid, 1=not or uncachable in the first place */
/* Gets serial number from name */
int Cache_Get_SerialNumber(const ASCII * name, BYTE * sn)
{
	int ret;
	ALIASFINDLOCK;

	global_aliasfind_struct.ret = 11  ;// not yet found
	global_aliasfind_struct.dsize = strlen(name) + 1 ;
	global_aliasfind_struct.name = name ;
	global_aliasfind_struct.sn = sn ;
	twalk(cache.permanent_tree, Aliasfindaction);
	ret = global_aliasfind_struct.ret ;
	ALIASFINDUNLOCK;
	if (ret) {
		LEVEL_DEBUG("Antialiasing %s unsuccesssful", SAFESTRING(name));
	} else {
		LEVEL_DEBUG("Antialiased %s as " SNformat, SAFESTRING(name), SNvar(sn));
	}

	return ret;
}

/* Look in caches, 0=found and valid, 1=not or uncachable in the first place */
static int Cache_Get_Common(void *data, size_t * dsize, time_t * duration, const struct tree_node *tn)
{
	int ret;
	time_t now = time(NULL);
	struct tree_opaque *opaque;
	//printf("Cache_Get_Common\n") ;
	LEVEL_DEBUG("Get from cache sn " SNformat " pointer=%p index=%d size=%d", SNvar(tn->tk.sn), tn->tk.p, tn->tk.extension, (int) dsize[0]);
	node_show(tn);
	//printf("\nTree (new):\n");
	new_tree();
	//printf("\nTree (old):\n");
	new_tree();
	CACHE_RLOCK;
	if ((opaque = tfind(tn, &cache.temporary_tree_new, tree_compare))
		|| ((cache.retired + duration[0] > now)
			&& (opaque = tfind(tn, &cache.temporary_tree_old, tree_compare)))
		) {
		//printf("CACHE GET 2 opaque=%p tn=%p\n",opaque,opaque->key);
		duration[0] = opaque->key->expires - now ;
		LEVEL_DEBUG("value found in cache. Remaining life: %d seconds.",duration[0]);
		if (duration[0] > 0) {
			// Compared with >= before, but fc_second(1) always cache for 2 seconds in that case.
			// Very noticable when reading time-data like "/26.80A742000000/date" for example.
			//printf("CACHE GET 3 buffer size=%lu stored size=%d\n",*dsize,opaque->key->dsize);
			if ( dsize[0] >= opaque->key->dsize) {
				//printf("CACHE GET 4\n");
				dsize[0] = opaque->key->dsize;
				//tree_show(opaque,leaf,0);
				//printf("CACHE GET 5 size=%lu\n",*dsize);
				if (dsize[0]) {
					memcpy(data, TREE_DATA(opaque->key), dsize[0]);
				}
				ret = 0;
				//printf("CACHE GOT\n");
				//twalk(cache.temporary_tree_new,tree_show) ;
			} else {
				ret = -EMSGSIZE;
			}
		} else {
			ret = -ETIMEDOUT;
		}
	} else {
		LEVEL_DEBUG("value not found in cache");
		ret = -ENOENT;
	}
	CACHE_RUNLOCK;
	return ret;
}

/* Look in caches, 0=found and valid, 1=not or uncachable in the first place */
static int Cache_Get_Store(void *data, size_t * dsize, time_t * duration, const struct tree_node *tn)
{
	struct tree_opaque *opaque;
	int ret;
	(void) duration; // ignored -- no timeout
	STORE_RLOCK;
	if ((opaque = tfind(tn, &cache.permanent_tree, tree_compare))) {
		if ( dsize[0] >= opaque->key->dsize) {
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
	
	LoadTK( pn->sn, pn->selected_filetype, pn->extension, &tn ) ;
	switch (pn->selected_filetype->change) {
		case fc_persistent:
			return Del_Stat(&cache_sto, Cache_Del_Store(&tn));
		default:
			return Del_Stat(&cache_ext, Cache_Del_Common(&tn));
	}
}

int Cache_Del_Mixed_Individual(const struct parsedname *pn)
{
	struct tree_node tn;
	time_t duration;
	//printf("Cache_Del\n") ;
	if (!pn) {
		return 1;				// do check here to avoid needless processing
	}
	if (pn->selected_filetype->ag==NON_AGGREGATE || pn->selected_filetype->ag->combined!=ag_mixed) {
		return 1 ;
	}
	duration = TimeOut(pn->selected_filetype->change);
	if (duration <= 0) {
		return 1;				/* in case timeout set to 0 */
	}
	
	LoadTK( pn->sn, pn->selected_filetype, 0, &tn) ;
	for ( tn.tk.extension = pn->selected_filetype->ag->elements-1 ; tn.tk.extension >= 0 ; --tn.tk.extension ) {
		switch (pn->selected_filetype->change) {
			case fc_persistent:
				Del_Stat(&cache_sto, Cache_Del_Store(&tn));
			default:
				Del_Stat(&cache_ext, Cache_Del_Common(&tn));
		}
	}
	return 0 ;
}

int Cache_Del_Mixed_Aggregate(const struct parsedname *pn)
{
	struct tree_node tn;
	time_t duration;
	//printf("Cache_Del\n") ;
	if (!pn) {
		return 1;				// do check here to avoid needless processing
	}
	if (pn->selected_filetype->ag==NON_AGGREGATE || pn->selected_filetype->ag->combined!=ag_mixed) {
		return 1 ;
	}
	duration = TimeOut(pn->selected_filetype->change);
	if (duration <= 0) {
		return 1;				/* in case timeout set to 0 */
	}
	
	LoadTK( pn->sn, pn->selected_filetype, EXTENSION_ALL, &tn) ;
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
	
	FS_LoadDirectoryOnly(&pn_directory, pn);
	LoadTK( pn_directory.sn, Directory_Marker, pn->selected_connection->index, &tn ) ;
	return Del_Stat(&cache_dir, Cache_Del_Common(&tn));
}

int Cache_Del_Simul(enum simul_type type, const struct parsedname *pn)
{
	struct tree_node tn;
	struct parsedname pn_directory;
	
	FS_LoadDirectoryOnly(&pn_directory, pn);
	LoadTK(pn_directory.sn, Simul_Marker[type], pn->selected_connection->index, &tn );
	return Del_Stat(&cache_dir, Cache_Del_Common(&tn));
}

int Cache_Del_Device(const struct parsedname *pn)
{
	struct tree_node tn;
	time_t duration = TimeOut(fc_presence);
	if (duration <= 0) {
		return 1;
	}

	LoadTK(pn->sn, Device_Marker, 0, &tn) ;
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

	LoadTK(pn->sn, ip->name, 0, &tn);
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
	LEVEL_DEBUG("Delete from cache sn " SNformat " in=%p index=%d", SNvar(tn->tk.sn), tn->tk.p, tn->tk.extension);
	CACHE_WLOCK;
	if ((opaque = tfind(tn, &cache.temporary_tree_new, tree_compare))
		|| ((cache.killed > now)
			&& (opaque = tfind(tn, &cache.temporary_tree_old, tree_compare)))
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
	if ((opaque = tfind(tn, &cache.permanent_tree, tree_compare))) {
		tn_found = opaque->key;
		tdelete(tn, &cache.permanent_tree, tree_compare);
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

static void LoadTK( const BYTE * sn, void * p, int extension, struct tree_node * tn )
{
	memset(&(tn->tk), 0, sizeof(struct tree_key));
	memcpy(tn->tk.sn, sn, SERIAL_NUMBER_SIZE);
	tn->tk.p = p;
	tn->tk.extension = extension;
}

#endif							/* OW_CACHE */
