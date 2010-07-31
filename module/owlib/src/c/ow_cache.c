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

enum cache_task_return { ctr_ok, ctr_not_found, ctr_expired, ctr_size_mismatch, } ;

static void * GetFlippedTree( void ) ;
static void DeleteFlippedTree( void * retired_tree ) ;

static int Cache_Type_Store( const struct parsedname * pn ) ;

static GOOD_OR_BAD Cache_Add(const void *data, const size_t datasize, const struct parsedname *pn);
static GOOD_OR_BAD Cache_Add_Common(struct tree_node *tn);
static GOOD_OR_BAD Cache_Add_Store(struct tree_node *tn);

static enum cache_task_return Cache_Get_Common(void *data, size_t * dsize, time_t * duration, const struct tree_node *tn);
static enum cache_task_return Cache_Get_Common_Dir(struct dirblob *db, time_t * duration, const struct tree_node *tn);
static enum cache_task_return Cache_Get_Store(void *data, size_t * dsize, time_t * duration, const struct tree_node *tn);

static GOOD_OR_BAD Cache_Get_Simultaneous(enum simul_type type, struct one_wire_query *owq) ;
static GOOD_OR_BAD Cache_Get_Internal(void *data, size_t * dsize, const struct internal_prop *ip, const struct parsedname *pn);
static GOOD_OR_BAD Cache_Get_Strict(void *data, size_t dsize, const struct parsedname *pn);

static GOOD_OR_BAD Cache_Del_Common(const struct tree_node *tn);
static GOOD_OR_BAD Cache_Del_Store(const struct tree_node *tn);

static GOOD_OR_BAD Add_Stat(struct cache *scache, GOOD_OR_BAD result);
static GOOD_OR_BAD Get_Stat(struct cache *scache, const enum cache_task_return result);
static void Del_Stat(struct cache *scache, const int result);
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
static GOOD_OR_BAD Add_Stat(struct cache *scache, GOOD_OR_BAD result)
{
	if ( GOOD(result) ) {
		STAT_ADD1(scache->adds);
	}
	return result;
}

/* Higher level add of a one-wire-query object */
GOOD_OR_BAD OWQ_Cache_Add(const struct one_wire_query *owq)
{
	const struct parsedname *pn = PN(owq);
	if (pn->extension == EXTENSION_ALL) {
		switch (pn->selected_filetype->format) {
		case ft_ascii:
		case ft_vascii:
		case ft_alias:
		case ft_binary:
			return gbBAD;			// cache of string arrays not supported
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
			return gbBAD;
		}
	} else {
		switch (pn->selected_filetype->format) {
		case ft_ascii:
		case ft_vascii:
		case ft_alias:
		case ft_binary:
			if (OWQ_offset(owq) > 0) {
				return gbBAD;
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
			return gbBAD;
		}
	}
}

/* Add an item to the cache */
/* return 0 if good, 1 if not */
static GOOD_OR_BAD Cache_Add(const void *data, const size_t datasize, const struct parsedname *pn)
{
	struct tree_node *tn;
	time_t duration;

	if (!pn || IsAlarmDir(pn)) {
		return gbGOOD;				// do check here to avoid needless processing
	}

	duration = TimeOut(pn->selected_filetype->change);
	if (duration <= 0) {
		return gbGOOD;				/* in case timeout set to 0 */
	}

	// allocate space for the node and data
	tn = (struct tree_node *) owmalloc(sizeof(struct tree_node) + datasize);
	if (!tn) {
		return gbBAD;
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
GOOD_OR_BAD Cache_Add_Dir(const struct dirblob *db, const struct parsedname *pn)
{
	time_t duration = TimeOut(fc_directory);
	struct tree_node *tn;
	size_t size = DirblobElements(db) * SERIAL_NUMBER_SIZE;
	struct parsedname pn_directory;

	if (pn==NULL || pn->selected_connection==NULL) {
		return gbGOOD;				// do check here to avoid needless processing
	}
	
	switch ( pn->selected_connection->busmode ) {
		case bus_fake:
		case bus_tester:
		case bus_mock:
		case bus_w1:
		case bus_bad:
		case bus_unknown:
			return gbGOOD ;
		default:
			break ;
	}
	
	if (duration <= 0) {
		return 0;				/* in case timeout set to 0 */
	}
	
	// allocate space for the node and data
	tn = (struct tree_node *) owmalloc(sizeof(struct tree_node) + size);
	if (!tn) {
		return gbBAD;
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
GOOD_OR_BAD Cache_Add_Simul(const enum simul_type type, const struct parsedname *pn)
{
	time_t duration = TimeOut(fc_volatile);
	struct tree_node *tn;
	struct parsedname pn_directory;

	if (pn==NULL || pn->selected_connection==NULL) {
		return gbGOOD;				// do check here to avoid needless processing
	}
	
	switch ( pn->selected_connection->busmode ) {
		case bus_fake:
		case bus_tester:
		case bus_mock:
		case bus_bad:
		case bus_unknown:
			return gbGOOD ;
		default:
			break ;
	}
	
	if (duration <= 0) {
		return gbGOOD;				/* in case timeout set to 0 */
	}
	
	// allocate space for the node and data
	tn = (struct tree_node *) owmalloc(sizeof(struct tree_node));
	if (!tn) {
		return gbBAD;
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
GOOD_OR_BAD Cache_Add_Device(const int bus_nr, const BYTE * sn)
{
	time_t duration = TimeOut(fc_presence);
	struct tree_node *tn;

	if (duration <= 0) {
		return gbGOOD;				/* in case timeout set to 0 */
	}

	if ( sn[0] == 0 ) { //bad serial number
		return gbGOOD ;
	}

	tn = (struct tree_node *) owmalloc(sizeof(struct tree_node) + sizeof(int));
	if (!tn) {
		return gbBAD;
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
GOOD_OR_BAD Cache_Add_Internal(const void *data, const size_t datasize, const struct internal_prop *ip, const struct parsedname *pn)
{
	struct tree_node *tn;
	time_t duration;
	//printf("Cache_Add_Internal\n");
	if (!pn) {
		return gbGOOD;				// do check here to avoid needless processing
	}

	duration = TimeOut(ip->change);
	if (duration <= 0) {
		return gbGOOD;				/* in case timeout set to 0 */
	}

	tn = (struct tree_node *) owmalloc(sizeof(struct tree_node) + datasize);
	if (!tn) {
		return gbBAD;
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
GOOD_OR_BAD Cache_Add_Alias(const ASCII *name, const BYTE * sn)
{
	struct tree_node *tn;
	size_t size = strlen(name) ;

	tn = (struct tree_node *) owmalloc(sizeof(struct tree_node) + size + 1 );
	if (!tn) {
		return gbBAD;
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
static GOOD_OR_BAD Cache_Add_Common(struct tree_node *tn)
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
		case yes_add: // add new entry
			STATLOCK;
			AVERAGE_IN(&new_avg);
			++cache_adds;			/* statistics */
			STATUNLOCK;
			return gbGOOD;
		case just_update: // update the time mark and data
			STATLOCK;
			AVERAGE_MARK(&new_avg);
			++cache_adds;			/* statistics */
			STATUNLOCK;
			return gbGOOD;
		default: // unable to add
			return gbBAD;
	}
}

/* Add an item to the cache */
/* retire the cache (flip) if too old, and start a new one (keep the old one for a while) */
/* return 0 if good, 1 if not */
static GOOD_OR_BAD Cache_Add_Store(struct tree_node *tn)
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
		return gbGOOD;
	case just_update:
		STATLOCK;
		AVERAGE_MARK(&store_avg);
		STATUNLOCK;
		return gbGOOD;
	default:
		return gbBAD;
	}
}

static GOOD_OR_BAD Get_Stat(struct cache *scache, const enum cache_task_return result)
{
	GOOD_OR_BAD gbret = gbBAD ; // default
	
	STATLOCK;
	++scache->tries;
	switch ( result ) {
		case ctr_expired:
			++scache->expires;
			break ;
		case ctr_ok:
			++scache->hits;
			gbret = gbGOOD ;
			break ;
		default:
			break ;
	}	
	STATUNLOCK;
	return gbret ;
}

/* Does cache get, but doesn't allow play in data size */
static GOOD_OR_BAD Cache_Get_Strict(void *data, size_t dsize, const struct parsedname *pn)
{
	size_t size = dsize;
	RETURN_BAD_IF_BAD( Cache_Get(data, &size, pn) ) ;
	return ( dsize == size) ? gbGOOD : gbBAD ;
}


GOOD_OR_BAD OWQ_Cache_Get(struct one_wire_query *owq)
{
	struct parsedname *pn = PN(owq);

	// do check here to avoid needless processing
	if (IsUncachedDir(pn) || IsAlarmDir(pn)) {
		return gbBAD;
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
			return gbBAD;			// string arrays not supported
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
			return gbBAD;
		}
	} else {
		switch (pn->selected_filetype->format) {
		case ft_ascii:
		case ft_vascii:
		case ft_alias:
		case ft_binary:
			if (OWQ_offset(owq) > 0) {
				return gbBAD;
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
			return gbBAD;
		}
	}
}

/* Look in caches, 0=found and valid, 1=not or uncachable in the first place */
GOOD_OR_BAD Cache_Get(void *data, size_t * dsize, const struct parsedname *pn)
{
	time_t duration;
	struct tree_node tn;
	//printf("Cache_Get\n") ;
	// do check here to avoid needless processing
	if (IsUncachedDir(pn) || IsAlarmDir(pn)) {
		return gbBAD;
	}

	duration = TimeOut(pn->selected_filetype->change);
	if (duration <= 0) {
		return gbBAD;
	}

	LEVEL_DEBUG(SNformat " size=%d IsUncachedDir=%d", SNvar(pn->sn), (int) dsize[0], IsUncachedDir(pn));
	LoadTK( pn->sn, pn->selected_filetype, pn->extension, &tn );
	return Cache_Type_Store(pn) ?
		Get_Stat(&cache_sto, Cache_Get_Store(data, dsize, &duration, &tn)) :
		Get_Stat(&cache_ext, Cache_Get_Common(data, dsize, &duration, &tn));
}

/* Look in caches, 0=found and valid, 1=not or uncachable in the first place */
GOOD_OR_BAD Cache_Get_Dir(struct dirblob *db, const struct parsedname *pn)
{
	time_t duration = TimeOut(fc_directory);
	struct tree_node tn;
	struct parsedname pn_directory;
	DirblobInit(db);
	if (duration <= 0) {
		return gbBAD;
	}

	LEVEL_DEBUG(SNformat, SNvar(pn->sn));
	FS_LoadDirectoryOnly(&pn_directory, pn);
	LoadTK( pn_directory.sn, Directory_Marker, pn->selected_connection->index, &tn) ;
	return Get_Stat(&cache_dir, Cache_Get_Common_Dir(db, &duration, &tn));
}

/* Look in caches, 0=found and valid, 1=not or uncachable in the first place */
static enum cache_task_return Cache_Get_Common_Dir(struct dirblob *db, time_t * duration, const struct tree_node *tn)
{
	enum cache_task_return ctr_ret;
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
				ctr_ret = ctr_ok;
			} else {
				ctr_ret = ctr_size_mismatch;
			}
		} else {
			//char b[26];
			//printf("GOT DEAD now:%s",ctime_r(&now,b)) ;
			//printf("        then:%s",ctime_r(&opaque->key->expires,b)) ;
			LEVEL_DEBUG("Expired in cache");
			ctr_ret = ctr_expired;
		}
	} else {
		LEVEL_DEBUG("dir not found in cache");
		ctr_ret = ctr_not_found;
	}
	CACHE_RUNLOCK;
	return ctr_ret;
}

/* Look in caches, 0=found and valid, 1=not or uncachable in the first place */
GOOD_OR_BAD Cache_Get_Device(void *bus_nr, const struct parsedname *pn)
{
	time_t duration = TimeOut(fc_presence);
	size_t size = sizeof(int);
	struct tree_node tn;
	if (duration <= 0) {
		return gbBAD;
	}

	LEVEL_DEBUG(SNformat, SNvar(pn->sn));
	LoadTK( pn->sn, Device_Marker, 0, &tn ) ;
	return Get_Stat(&cache_dev, Cache_Get_Common(bus_nr, &size, &duration, &tn));
}

/* Does cache get, but doesn't allow play in data size */
GOOD_OR_BAD Cache_Get_Internal_Strict(void *data, size_t dsize, const struct internal_prop *ip, const struct parsedname *pn)
{
	size_t size = dsize;
	RETURN_BAD_IF_BAD( Cache_Get_Internal(data, &size, ip, pn) ) ;
	
	return ( dsize == size) ? gbGOOD : gbBAD ;
}

/* Look in caches, 0=found and valid, 1=not or uncachable in the first place */
static GOOD_OR_BAD Cache_Get_Internal(void *data, size_t * dsize, const struct internal_prop *ip, const struct parsedname *pn)
{
	struct tree_node tn;
	time_t duration;
	//printf("Cache_Get_Internal");
	if (!pn) {
		return gbBAD;				// do check here to avoid needless processing
	}
	
	duration = TimeOut(ip->change);
	if (duration <= 0) {
		return gbBAD;				/* in case timeout set to 0 */
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
GOOD_OR_BAD Cache_Get_Simul_Time(enum simul_type type, time_t * dwell_time, const struct parsedname * pn)
{
	// valid cached primary data -- see if a simultaneous conversion should be used instead
	struct tree_node tn;
	time_t duration ;
	time_t time_left ;
	size_t dsize_simul = 0 ;
	struct parsedname pn_directory ;

	duration = TimeOut(ipSimul[type].change); // time allocated this conversion
	if ( duration <= 0) {
		// uncachable
		return gbBAD;
	}
	
	FS_LoadDirectoryOnly(&pn_directory, pn);
	LoadTK(pn_directory.sn, Simul_Marker[type], pn->selected_connection->index, &tn ) ;
	if ( Get_Stat(&cache_int, Cache_Get_Common(NULL, &dsize_simul, &time_left, &tn)) ) {
		return gbBAD ;
	}
	// duration_simul is time left
	// duration is time allocated
	// back-compute dwell time
	dwell_time[0] = duration - time_left ;
	return gbGOOD ;
}

/* Test for a simultaneous property
 * return true if simultaneous is the prefered method
 * bad if no simultaneous, or it's not the best
*/
static GOOD_OR_BAD Cache_Get_Simultaneous(enum simul_type type, struct one_wire_query *owq)
{
	struct tree_node tn;
	time_t duration ;
	time_t time_left ;
	time_t dwell_time_simul ;
	struct parsedname * pn = PN(owq) ;
	size_t dsize = sizeof(union value_object) ;
	
	duration = TimeOut(pn->selected_filetype->change);
	if (duration <= 0) {
		// probably "uncached" requested
		return gbBAD;
	}
	
	LoadTK( pn->sn, pn->selected_filetype, pn->extension, &tn ) ;
	
	if ( Get_Stat(&cache_ext, Cache_Get_Common( &OWQ_val(owq), &dsize, &time_left, &tn)) == 0 ) {
		// valid cached primary data -- see if a simultaneous conversion should be used instead
		time_t dwell_time_data = duration - time_left ;
		
		if ( BAD( Cache_Get_Simul_Time( type, &dwell_time_simul, pn)) ) {
			// Simul not found or timed out
			LEVEL_DEBUG("Simultaneous conversion not found.") ;
			OWQ_SIMUL_CLR(owq) ;
			return gbGOOD ;
		}
		if ( dwell_time_simul < dwell_time_data ) {
			LEVEL_DEBUG("Simultaneous conversion is newer than previous reading.") ;
			OWQ_SIMUL_SET(owq) ;
			return gbBAD ; // Simul is newer
		}
		// Cached data is newer, so use it
		OWQ_SIMUL_CLR(owq) ;
		return gbGOOD ;
	}
	// fall through -- no cached primary data
	if ( BAD( Cache_Get_Simul_Time( type, &dwell_time_simul, pn)) ) {
		// no simultaneous either
		OWQ_SIMUL_CLR(owq) ;
		return gbBAD ;
	}
	OWQ_SIMUL_SET(owq) ;
	return gbBAD ; // Simul is newer
}

/* Look in caches, 0=found and valid, 1=not or uncachable in the first place */
/* space already allocated in buffer */
GOOD_OR_BAD Cache_Get_Alias(ASCII * name, size_t length, const BYTE * sn)
{
	struct tree_node tn;
	struct tree_opaque *opaque;
	GOOD_OR_BAD ret = gbBAD ;

	LoadTK(sn, Alias_Marker, 0, &tn ) ;

	STORE_RLOCK;
	if ((opaque = tfind(&tn, &cache.permanent_tree, tree_compare))) {
		if ( opaque->key->dsize < length ) {
			strncpy(name,(ASCII *)TREE_DATA(opaque->key),length);
			ret = gbGOOD ;
			LEVEL_DEBUG("Retrieving " SNformat " alias=%s", SNvar(sn), SAFESTRING(name) );
		}
	}
	STORE_RUNLOCK;
	return ret ;
}

struct {
	const ASCII *name;
	size_t dsize ;
	BYTE * sn;
	GOOD_OR_BAD ret ;
} global_aliasfind_struct;

static void Aliasfindaction(const void *node, const VISIT which, const int depth)
{
	const struct tree_node *p = *(struct tree_node * const *) node;
	(void) depth;
	if (  GOOD( global_aliasfind_struct.ret) ) {
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
		global_aliasfind_struct.ret = gbGOOD ;
		memcpy(global_aliasfind_struct.sn,p->tk.sn,SERIAL_NUMBER_SIZE) ;
		return ;
	case preorder:
	case endorder:
		break;
	}
}
/* Look in caches, 0=found and valid, 1=not or uncachable in the first place */
/* Gets serial number from name */
GOOD_OR_BAD Cache_Get_SerialNumber(const ASCII * name, BYTE * sn)
{
	GOOD_OR_BAD ret;

	ALIASFINDLOCK;
	global_aliasfind_struct.ret = gbBAD  ; // not yet found
	global_aliasfind_struct.dsize = strlen(name) + 1 ;
	global_aliasfind_struct.name = name ;
	global_aliasfind_struct.sn = sn ;
	twalk(cache.permanent_tree, Aliasfindaction);
	ret = global_aliasfind_struct.ret ;
	ALIASFINDUNLOCK;

	if ( BAD(ret)) {
		LEVEL_DEBUG("Antialiasing %s unsuccesssful", SAFESTRING(name));
		return gbBAD ;
	} else {
		LEVEL_DEBUG("Antialiased %s as " SNformat, SAFESTRING(name), SNvar(sn));
		return gbGOOD ;
	}
}

/* Look in caches, 0=found and valid, 1=not or uncachable in the first place */
/* duration is time left */
static enum cache_task_return Cache_Get_Common(void *data, size_t * dsize, time_t * duration, const struct tree_node *tn)
{
	enum cache_task_return ctr_ret;
	time_t now = time(NULL);
	struct tree_opaque *opaque;
	
	LEVEL_DEBUG("Get from cache sn " SNformat " pointer=%p index=%d size=%d", SNvar(tn->tk.sn), tn->tk.p, tn->tk.extension, (int) dsize[0]);
	//node_show(tn);
	//new_tree();
	CACHE_RLOCK;
	if ((opaque = tfind(tn, &cache.temporary_tree_new, tree_compare))
		|| ((cache.retired + duration[0] > now)
			&& (opaque = tfind(tn, &cache.temporary_tree_old, tree_compare)))
		) {
		duration[0] = opaque->key->expires - now ;
		LEVEL_DEBUG("value found in cache. Remaining life: %d seconds.",duration[0]);
		if (duration[0] > 0) {
			// Compared with >= before, but fc_second(1) always cache for 2 seconds in that case.
			// Very noticable when reading time-data like "/26.80A742000000/date" for example.
			if ( dsize[0] >= opaque->key->dsize) {
				dsize[0] = opaque->key->dsize;
				//tree_show(opaque,leaf,0);
				if (dsize[0]) {
					memcpy(data, TREE_DATA(opaque->key), dsize[0]);
				}
				ctr_ret = ctr_ok;
				//twalk(cache.temporary_tree_new,tree_show) ;
			} else {
				ctr_ret = ctr_size_mismatch;
			}
		} else {
			ctr_ret = ctr_expired;
		}
	} else {
		LEVEL_DEBUG("value not found in cache");
		ctr_ret = ctr_not_found;
	}
	CACHE_RUNLOCK;
	return ctr_ret;
}

/* Look in caches, 0=found and valid, 1=not or uncachable in the first place */
static enum cache_task_return Cache_Get_Store(void *data, size_t * dsize, time_t * duration, const struct tree_node *tn)
{
	struct tree_opaque *opaque;
	enum cache_task_return ctr_ret;
	(void) duration; // ignored -- no timeout
	STORE_RLOCK;
	if ((opaque = tfind(tn, &cache.permanent_tree, tree_compare))) {
		if ( dsize[0] >= opaque->key->dsize) {
			dsize[0] = opaque->key->dsize;
			if (dsize[0]) {
				memcpy(data, TREE_DATA(opaque->key), dsize[0]);
			}
			ctr_ret = ctr_ok;
		} else {
			ctr_ret = ctr_size_mismatch;
		}
	} else {
		ctr_ret = ctr_not_found;
	}
	STORE_RUNLOCK;
	return ctr_ret;
}

static void Del_Stat(struct cache *scache, const int result)
{
	if ( GOOD( result)) {
		STAT_ADD1(scache->deletes);
	}
}

void OWQ_Cache_Del(const struct one_wire_query *owq)
{
	Cache_Del(PN(owq));
}

void Cache_Del(const struct parsedname *pn)
{
	struct tree_node tn;
	time_t duration;
	//printf("Cache_Del\n") ;
	if (!pn) {
		return;				// do check here to avoid needless processing
	}
	
	duration = TimeOut(pn->selected_filetype->change);
	if (duration <= 0) {
		return;				/* in case timeout set to 0 */
	}
	
	LoadTK( pn->sn, pn->selected_filetype, pn->extension, &tn ) ;
	switch (pn->selected_filetype->change) {
		case fc_persistent:
			Del_Stat(&cache_sto, Cache_Del_Store(&tn));
			break ;
		default:
			Del_Stat(&cache_ext, Cache_Del_Common(&tn));
			break ;
	}
}

void Cache_Del_Mixed_Individual(const struct parsedname *pn)
{
	struct tree_node tn;
	time_t duration;
	//printf("Cache_Del\n") ;
	if (!pn) {
		return;				// do check here to avoid needless processing
	}
	if (pn->selected_filetype->ag==NON_AGGREGATE || pn->selected_filetype->ag->combined!=ag_mixed) {
		return ;
	}
	duration = TimeOut(pn->selected_filetype->change);
	if (duration <= 0) {
		return;				/* in case timeout set to 0 */
	}
	
	LoadTK( pn->sn, pn->selected_filetype, 0, &tn) ;
	for ( tn.tk.extension = pn->selected_filetype->ag->elements-1 ; tn.tk.extension >= 0 ; --tn.tk.extension ) {
		switch (pn->selected_filetype->change) {
			case fc_persistent:
				Del_Stat(&cache_sto, Cache_Del_Store(&tn));
				break ;
			default:
				Del_Stat(&cache_ext, Cache_Del_Common(&tn));
				break ;
		}
	}
}

void Cache_Del_Mixed_Aggregate(const struct parsedname *pn)
{
	struct tree_node tn;
	time_t duration;
	//printf("Cache_Del\n") ;
	if (!pn) {
		return;				// do check here to avoid needless processing
	}
	if (pn->selected_filetype->ag==NON_AGGREGATE || pn->selected_filetype->ag->combined!=ag_mixed) {
		return ;
	}
	duration = TimeOut(pn->selected_filetype->change);
	if (duration <= 0) {
		return;				/* in case timeout set to 0 */
	}
	
	LoadTK( pn->sn, pn->selected_filetype, EXTENSION_ALL, &tn) ;
	switch (pn->selected_filetype->change) {
		case fc_persistent:
			Del_Stat(&cache_sto, Cache_Del_Store(&tn));
			break ;
		default:
			Del_Stat(&cache_ext, Cache_Del_Common(&tn));
			break ;
	}
}

void Cache_Del_Dir(const struct parsedname *pn)
{
	struct tree_node tn;
	struct parsedname pn_directory;
	
	FS_LoadDirectoryOnly(&pn_directory, pn);
	LoadTK( pn_directory.sn, Directory_Marker, pn->selected_connection->index, &tn ) ;
	Del_Stat(&cache_dir, Cache_Del_Common(&tn));
}

void Cache_Del_Simul(enum simul_type type, const struct parsedname *pn)
{
	struct tree_node tn;
	struct parsedname pn_directory;
	
	FS_LoadDirectoryOnly(&pn_directory, pn);
	LoadTK(pn_directory.sn, Simul_Marker[type], pn->selected_connection->index, &tn );
	Del_Stat(&cache_dir, Cache_Del_Common(&tn));
}

void Cache_Del_Device(const struct parsedname *pn)
{
	struct tree_node tn;
	time_t duration = TimeOut(fc_presence);
	if (duration <= 0) {
		return;
	}

	LoadTK(pn->sn, Device_Marker, 0, &tn) ;
	Del_Stat(&cache_dev, Cache_Del_Common(&tn));
}

void Cache_Del_Internal(const struct internal_prop *ip, const struct parsedname *pn)
{
	struct tree_node tn;
	time_t duration;
	//printf("Cache_Del_Internal\n") ;
	if (!pn) {
		return;				// do check here to avoid needless processing
	}

	duration = TimeOut(ip->change);
	if (duration <= 0) {
		return;				/* in case timeout set to 0 */
	}

	LoadTK(pn->sn, ip->name, 0, &tn);
	switch (ip->change) {
	case fc_persistent:
		Del_Stat(&cache_sto, Cache_Del_Store(&tn));
		break;
	default:
		Del_Stat(&cache_int, Cache_Del_Common(&tn));
		break;
	}
}

static GOOD_OR_BAD Cache_Del_Common(const struct tree_node *tn)
{
	struct tree_opaque *opaque;
	time_t now = time(NULL);
	GOOD_OR_BAD ret = gbBAD;
	LEVEL_DEBUG("Delete from cache sn " SNformat " in=%p index=%d", SNvar(tn->tk.sn), tn->tk.p, tn->tk.extension);
	CACHE_WLOCK;
	if ((opaque = tfind(tn, &cache.temporary_tree_new, tree_compare))
		|| ((cache.killed > now)
			&& (opaque = tfind(tn, &cache.temporary_tree_old, tree_compare)))
		) {
		opaque->key->expires = now - 1;
		ret = gbGOOD;
	}
	CACHE_WUNLOCK;
	return ret;
}

static GOOD_OR_BAD Cache_Del_Store(const struct tree_node *tn)
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
		return gbBAD;
	}

	owfree(tn_found);
	STATLOCK;
	AVERAGE_OUT(&store_avg);
	STATUNLOCK;
	return gbGOOD;
}

static void LoadTK( const BYTE * sn, void * p, int extension, struct tree_node * tn )
{
	memset(&(tn->tk), 0, sizeof(struct tree_key));
	memcpy(tn->tk.sn, sn, SERIAL_NUMBER_SIZE);
	tn->tk.p = p;
	tn->tk.extension = extension;
}

#endif							/* OW_CACHE */
