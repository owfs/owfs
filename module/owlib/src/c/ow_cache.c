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
int AuxDirMarkerLoc ;
void * AuxDirectory_Marker = &AuxDirMarkerLoc ;
int MainDirMarkerLoc ;
void * MainDirectory_Marker = &MainDirMarkerLoc ;

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
	void *temporary_tree_new;			// current cache database
	void *temporary_tree_old;			// older cache database
	void *persistent_tree;				// persistent database
	void *temporary_alias_tree_new;		// current cache database
	void *temporary_alias_tree_old;		// older cache database
	void *persistent_alias_tree;		// persistent database
	size_t old_ram_size;				// cache size
	size_t new_ram_size;				// cache size
	time_t time_retired;				// start time of older
	time_t time_to_kill;				// deathtime of older
	time_t retired_lifespan;			// lifetime of older
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

struct alias_tree_node {
	size_t size;
	time_t expires;
	union {
		INDEX_OR_ERROR bus;
		BYTE sn[SERIAL_NUMBER_SIZE];
	} ;
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

#define ALIAS_TREE_DATA(atn)    ( (ASCII *)(atn) + sizeof(struct alias_tree_node) )
#define CONST_ALIAS_TREE_DATA(atn)    ( (const ASCII *)(atn) + sizeof(struct alias_tree_node) )

enum cache_task_return { ctr_ok, ctr_not_found, ctr_expired, ctr_size_mismatch, } ;

static void FlipTree( void ) ;

static int IsThisPersistent( const struct parsedname * pn ) ;

static GOOD_OR_BAD Cache_Add(const void *data, const size_t datasize, const struct parsedname *pn);
static GOOD_OR_BAD Cache_Add_Common(struct tree_node *tn);
static GOOD_OR_BAD Cache_Add_Persistent(struct tree_node *tn);

static enum cache_task_return Cache_Get_Common(void *data, size_t * dsize, time_t * duration, const struct tree_node *tn);
static enum cache_task_return Cache_Get_Common_Dir(struct dirblob *db, time_t * duration, const struct tree_node *tn);
static enum cache_task_return Cache_Get_Persistent(void *data, size_t * dsize, time_t * duration, const struct tree_node *tn);

static GOOD_OR_BAD Cache_Get_Simultaneous(enum simul_type type, struct one_wire_query *owq) ;
static GOOD_OR_BAD Cache_Get_Internal(void *data, size_t * dsize, const struct internal_prop *ip, const struct parsedname *pn);
static GOOD_OR_BAD Cache_Get_Strict(void *data, size_t dsize, const struct parsedname *pn);

static void Cache_Del(const struct parsedname *pn) ;
static GOOD_OR_BAD Cache_Del_Common(const struct tree_node *tn);
static GOOD_OR_BAD Cache_Del_Persistent(const struct tree_node *tn);

static void Cache_Add_Alias_Common(struct alias_tree_node *atn);
static INDEX_OR_ERROR Cache_Get_Alias_Common( struct alias_tree_node * atn) ;

static void Cache_Add_Alias_SN(const ASCII * alias_name, const BYTE * sn) ;
static void Cache_Del_Alias_SN(const ASCII * alias_name) ;

static void Cache_Add_Alias_Persistent(struct alias_tree_node *atn);
static enum cache_task_return Cache_Get_Alias_Persistent( BYTE * sn, struct alias_tree_node * atn);
static void Cache_Del_Alias_Persistent( struct alias_tree_node * atn) ;

static GOOD_OR_BAD Add_Stat(struct cache_stats *scache, GOOD_OR_BAD result);
static GOOD_OR_BAD Get_Stat(struct cache_stats *scache, const enum cache_task_return result);
static void Del_Stat(struct cache_stats *scache, const int result);

static int tree_compare(const void *a, const void *b);
static time_t TimeOut(const enum fc_change change);
static void Aliaslistaction(const void *node, const VISIT which, const int depth) ;
static void LoadTK( const BYTE * sn, void * p, int extension, struct tree_node * tn ) ;

/* used for the sort/search b-tree routines */
/* big voodoo pointer fuss to just do a standard memory compare of the "key" */
static int tree_compare(const void *a, const void *b)
{
	return memcmp(&((const struct tree_node *) a)->tk, &((const struct tree_node *) b)->tk, sizeof(struct tree_key));
}

/* used for the sort/search b-tree routines */
/* big voodoo pointer fuss to just do a standard memory compare of the "key" */
static int alias_tree_compare(const void *a, const void *b)
{
	int da = ((const struct alias_tree_node *) a)->size ;
	int d = da - ((const struct alias_tree_node *) b)->size ;

	if ( d != 0 ) {
		return d ;
	}
	return memcmp( CONST_ALIAS_TREE_DATA((const struct alias_tree_node *) a), CONST_ALIAS_TREE_DATA((const struct alias_tree_node *) b), da);
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
	case fc_simultaneous_temperature:
	case fc_simultaneous_voltage:
		return Globals.timeout_volatile;
	case fc_stable:
	case fc_read_stable:
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

static int IsThisPersistent( const struct parsedname * pn )
{
	return ( (pn->selected_filetype->change==fc_persistent) || get_busmode(pn->selected_connection) ) ;
}

/* DB cache creation code */
/* Note: done in single-threaded mode so locking not yet needed */
void Cache_Open(void)
{
	memset(&cache, 0, sizeof(struct cache_data));

	cache.retired_lifespan = TimeOut(fc_stable);
	if (cache.retired_lifespan > 3600) {
		cache.retired_lifespan = 3600;	/* 1 hour tops */
	}

	// Flip once (at start) to set up old tree.
	FlipTree() ;
}

/* Note: done in a simgle single thread mode so locking not needed */
void Cache_Close(void)
{
	Cache_Clear() ;
	SAFETDESTROY( cache.persistent_tree, owfree_func);
	SAFETDESTROY( cache.persistent_alias_tree, owfree_func);
}

/* Moves new to old tree, initializes new tree, and clears former old tree location */
static void FlipTree( void )
{
	void * flip = cache.temporary_tree_old; // old old saved for later clearing
	void * flip_alias = cache.temporary_alias_tree_old; // old old saved for later clearing

	/* Flip caches! old = new. New truncated, reset time and counters and flag */
	LEVEL_DEBUG("Flipping cache tree (purging timed-out data)");

	// move "new" pointers to "old"
	cache.temporary_tree_old = cache.temporary_tree_new;
	cache.old_ram_size = cache.new_ram_size;
	cache.temporary_alias_tree_old = cache.temporary_alias_tree_new;

	// New cache setup
	cache.temporary_tree_new = NULL;
	cache.temporary_alias_tree_new = NULL;
	cache.new_ram_size = 0;
	cache.added = 0;

	// set up "old" cache times
	cache.time_retired = NOW_TIME;
	cache.time_to_kill = cache.time_retired + cache.retired_lifespan;

	// delete really old tree
	LEVEL_DEBUG("flip cache. tdestroy() will be called.");
	SAFETDESTROY( flip, owfree_func);
	SAFETDESTROY( flip_alias, owfree_func);
	STATLOCK;
	++cache_flips;			/* statistics */
	memcpy(&old_avg, &new_avg, sizeof(struct average));
	AVERAGE_CLEAR(&new_avg);
	STATUNLOCK;
}

/* Clear the cache (a change was made that might give stale information) */
void Cache_Clear(void)
{
	CACHE_WLOCK;
	FlipTree() ;
	FlipTree() ;
	CACHE_WUNLOCK;
}

/* Wrapper to perform a cache function and add statistics */
static GOOD_OR_BAD Add_Stat(struct cache_stats *scache, GOOD_OR_BAD result)
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
			LEVEL_DEBUG("Adding data for %s", SAFESTRING(pn->path) );
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
			LEVEL_DEBUG("Adding data for %s", SAFESTRING(pn->path) );
			return Cache_Add(OWQ_buffer(owq), OWQ_length(owq), pn);
		case ft_integer:
		case ft_unsigned:
		case ft_yesno:
		case ft_date:
		case ft_float:
		case ft_pressure:
		case ft_temperature:
		case ft_tempgap:
			LEVEL_DEBUG("Adding data for %s", SAFESTRING(pn->path) );
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
	int persistent ;

	if (!pn || IsAlarmDir(pn)) {
		return gbGOOD;				// do check here to avoid needless processing
	}

	// Special handling of Mock
	persistent = IsThisPersistent(pn) ;
	if ( persistent ) {
		duration = 1 ;
	} else {
		duration = TimeOut(pn->selected_filetype->change);
		if (duration <= 0) {
			return gbGOOD;				/* in case timeout set to 0 */
		}
	}

	// allocate space for the node and data
	tn = (struct tree_node *) owmalloc(sizeof(struct tree_node) + datasize);
	if (!tn) {
		return gbBAD;
	}

	LEVEL_DEBUG(SNformat " size=%d", SNvar(pn->sn), (int) datasize);

	// populate the node structure with data
	LoadTK( pn->sn, pn->selected_filetype, pn->extension, tn );
	tn->expires = duration + NOW_TIME;
	tn->dsize = datasize;
	if (datasize) {
		memcpy(TREE_DATA(tn), data, datasize);
	}
	return persistent ?
		Add_Stat(&cache_pst, Cache_Add_Persistent(tn)) :
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

	if (pn==NO_PARSEDNAME || pn->selected_connection==NO_CONNECTION) {
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

	if ( DirblobElements(db) < 1 ) {
		// only cache long directories.
		// zero (or one?) entry is possibly an error and needs to be repeated more quickly
		LEVEL_DEBUG("Won\'t cache empty directory");
		Cache_Del_Dir( pn ) ;
		return gbGOOD ;
	}
	
	// allocate space for the node and data
	tn = (struct tree_node *) owmalloc(sizeof(struct tree_node) + size);
	if (!tn) {
		return gbBAD;
	}
	
	LEVEL_DEBUG("Adding directory for " SNformat " elements=%d", SNvar(pn->sn), DirblobElements(db));
	
	// populate node with directory name and dirblob
	FS_LoadDirectoryOnly(&pn_directory, pn);
	LoadTK( pn_directory.sn, Directory_Marker, pn->selected_connection->index, tn );
	tn->expires = duration + NOW_TIME;
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
	// Note: pn already points to directory
	time_t duration = TimeOut(fc_volatile);
	struct tree_node *tn;

	if (pn==NO_PARSEDNAME || pn->selected_connection==NO_CONNECTION) {
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
	LEVEL_DEBUG("Adding for conversion time for "SNformat, SNvar(pn->sn));
	tn = (struct tree_node *) owmalloc(sizeof(struct tree_node));
	if (!tn) {
		return gbBAD;
	}
	
	LEVEL_DEBUG(SNformat, SNvar(pn->sn));
	
	// populate node with directory name and dirblob
	LoadTK( pn->sn, Simul_Marker[type], pn->selected_connection->index, tn) ;
	LEVEL_DEBUG("Simultaneous add type=%d",type);
	tn->expires = duration + NOW_TIME;
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

	LEVEL_DEBUG("Adding device location " SNformat " bus=%d", SNvar(sn), (int) bus_nr);
	LoadTK(sn, Device_Marker, 0, tn );
	tn->expires = duration + NOW_TIME;
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
GOOD_OR_BAD Cache_Add_SlaveSpecific(const void *data, const size_t datasize, const struct internal_prop *ip, const struct parsedname *pn)
{
	struct tree_node *tn;
	time_t duration;
	//printf("Cache_Add_SlaveSpecific\n");
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

	LEVEL_DEBUG("Adding internal data for "SNformat " size=%d", SNvar(pn->sn), (int) datasize);
	LoadTK( pn->sn, ip->name, EXTENSION_INTERNAL, tn );
	tn->expires = duration + NOW_TIME;
	tn->dsize = datasize;
	if (datasize) {
		memcpy(TREE_DATA(tn), data, datasize);
	}
	//printf("ADD INTERNAL name= %s size=%d \n",tn->tk.p.nm,tn->dsize);
	//printf("  ADD INTERNAL data[0]=%d size=%d \n",((BYTE *)data)[0],datasize);
	switch (ip->change) {
	case fc_persistent:
		return Add_Stat(&cache_pst, Cache_Add_Persistent(tn));
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

	if ( size == 0 ) {
		return gbGOOD ;
	}

	tn = (struct tree_node *) owmalloc(sizeof(struct tree_node) + size + 1 );
	if (!tn) {
		return gbBAD;
	}

	LEVEL_DEBUG("Adding alias for " SNformat " = %s", SNvar(sn), name);
	LoadTK( sn, Alias_Marker, 0, tn );
	tn->expires = NOW_TIME;
	tn->dsize = size;
	memcpy((ASCII *)TREE_DATA(tn), name, size+1 ); // includes NULL
	Cache_Add_Alias_SN( name, sn ) ;
	return Add_Stat(&cache_pst, Cache_Add_Persistent(tn));
}

/* Add an item to the cache */
/* retire the cache (flip) if too old, and start a new one (keep the old one for a while) */
/* return 0 if good, 1 if not */
static GOOD_OR_BAD Cache_Add_Common(struct tree_node *tn)
{
	struct tree_opaque *opaque;
	enum { no_add, yes_add, just_update } state = no_add;

	node_show(tn);
	LEVEL_DEBUG("Add to cache sn " SNformat " pointer=%p index=%d size=%d", SNvar(tn->tk.sn), tn->tk.p, tn->tk.extension, tn->dsize);
	CACHE_WLOCK;
	if (cache.time_to_kill < NOW_TIME) {	// old database has timed out
		FlipTree() ;
	}
	if (Globals.cache_size && (cache.old_ram_size + cache.new_ram_size > Globals.cache_size)) {
		// failed size test
		owfree(tn);
	} else if ((opaque = tsearch(tn, &cache.temporary_tree_new, tree_compare))) {
		//printf("Cache_Add_Common to %p\n",opaque);
		if (tn != opaque->key) {
			cache.new_ram_size += sizeof(tn) - sizeof(opaque->key);
			owfree(opaque->key);
			opaque->key = tn;
			state = just_update;
		} else {
			state = yes_add;
			cache.new_ram_size += sizeof(tn);
		}
	} else {					// nothing found or added?!? free our memory segment
		owfree(tn);
	}
	CACHE_WUNLOCK;
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
static GOOD_OR_BAD Cache_Add_Persistent(struct tree_node *tn)
{
	struct tree_opaque *opaque;
	enum { no_add, yes_add, just_update } state = no_add;
	LEVEL_DEBUG("Adding data to permanent store");

	PERSISTENT_WLOCK;
	opaque = tsearch(tn, &cache.persistent_tree, tree_compare) ;
	if ( opaque != NULL ) {
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
	PERSISTENT_WUNLOCK;

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

static GOOD_OR_BAD Get_Stat(struct cache_stats *scache, const enum cache_task_return result)
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
	int persistent ;

	// do check here to avoid needless processing
	if (IsUncachedDir(pn) || IsAlarmDir(pn)) {
		return gbBAD;
	}

	// Special handling of Mock
	persistent = IsThisPersistent(pn) ;
	if ( persistent ) {
		duration = 1 ;
	} else {
		duration = TimeOut(pn->selected_filetype->change);
		if (duration <= 0) {
			return gbBAD;				/* in case timeout set to 0 */
		}
	}


	LEVEL_DEBUG(SNformat " size=%d IsUncachedDir=%d", SNvar(pn->sn), (int) dsize[0], IsUncachedDir(pn));
	LoadTK( pn->sn, pn->selected_filetype, pn->extension, &tn );
	return persistent ?
		Get_Stat(&cache_pst, Cache_Get_Persistent(data, dsize, &duration, &tn)) :
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

	LEVEL_DEBUG("Looking for directory "SNformat, SNvar(pn->sn));
	FS_LoadDirectoryOnly(&pn_directory, pn);
	LoadTK( pn_directory.sn, Directory_Marker, pn->selected_connection->index, &tn) ;
	return Get_Stat(&cache_dir, Cache_Get_Common_Dir(db, &duration, &tn));
}

/* Look in caches, 0=found and valid, 1=not or uncachable in the first place */
static enum cache_task_return Cache_Get_Common_Dir(struct dirblob *db, time_t * duration, const struct tree_node *tn)
{
	enum cache_task_return ctr_ret;
	time_t now = NOW_TIME;
	size_t size;
	struct tree_opaque *opaque;
	LEVEL_DEBUG("Get from cache sn " SNformat " pointer=%p extension=%d", SNvar(tn->tk.sn), tn->tk.p, tn->tk.extension);
	CACHE_RLOCK;
	opaque = tfind(tn, &cache.temporary_tree_new, tree_compare) ;
	if ( opaque == NULL ) {
		// not found in new tree
		if ( cache.time_retired + duration[0] > now ) {
			// old tree could be new enough
			opaque = tfind(tn, &cache.temporary_tree_old, tree_compare) ;
		}
	}
	if ( opaque != NULL ) {
		duration[0] = opaque->key->expires - now ;
		if (duration[0] >= 0) {
			LEVEL_DEBUG("Dir found in cache");
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
			LEVEL_DEBUG("Dir expired in cache");
			ctr_ret = ctr_expired;
		}
	} else {
		LEVEL_DEBUG("Dir not found in cache");
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

	LEVEL_DEBUG("Looking for device "SNformat, SNvar(pn->sn));
	LoadTK( pn->sn, Device_Marker, 0, &tn ) ;
	return Get_Stat(&cache_dev, Cache_Get_Common(bus_nr, &size, &duration, &tn));
}

/* Does cache get, but doesn't allow play in data size */
GOOD_OR_BAD Cache_Get_SlaveSpecific(void *data, size_t dsize, const struct internal_prop *ip, const struct parsedname *pn)
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
			return Get_Stat(&cache_pst, Cache_Get_Persistent(data, dsize, &duration, &tn));
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

	LEVEL_DEBUG("Looking for conversion time "SNformat, SNvar(pn->sn));
	
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
/* space allocated, needs to be owfree-d */
/* returns null-terminated string */
ASCII * Cache_Get_Alias(const BYTE * sn)
{
	struct tree_node tn;
	struct tree_opaque *opaque;
	ASCII * alias_name = NULL ;
	

	LoadTK(sn, Alias_Marker, 0, &tn ) ;

	PERSISTENT_RLOCK;
	opaque = tfind(&tn, &cache.persistent_tree, tree_compare) ;
	if ( opaque != NULL ) {
		alias_name = owmalloc( opaque->key->dsize + 1 ) ;
		if ( alias_name != NULL ) {
			memcpy( alias_name, (ASCII *)TREE_DATA(opaque->key), opaque->key->dsize+1 ) ;
			LEVEL_DEBUG("Retrieving " SNformat " alias=%s", SNvar(sn), alias_name );
		}
	}
	PERSISTENT_RUNLOCK;
	return alias_name ;
}

/* Look in caches, 0=found and valid, 1=not or uncachable in the first place */
/* duration is time left */
static enum cache_task_return Cache_Get_Common(void *data, size_t * dsize, time_t * duration, const struct tree_node *tn)
{
	enum cache_task_return ctr_ret;
	time_t now = NOW_TIME;
	struct tree_opaque *opaque;
	
	LEVEL_DEBUG("Search in cache sn " SNformat " pointer=%p index=%d size=%d", SNvar(tn->tk.sn), tn->tk.p, tn->tk.extension, (int) dsize[0]);
	//node_show(tn);
	//new_tree();
	CACHE_RLOCK;
	opaque = tfind(tn, &cache.temporary_tree_new, tree_compare) ;
	if ( opaque == NULL ) {
		// not found in new tree
		if ( cache.time_retired + duration[0] > now ) {
			// retired time isn't too old for this data item
			opaque = tfind(tn, &cache.temporary_tree_old, tree_compare) ;
		}
	}
	if ( opaque != NULL ) {
		// modify duration to time left (can be negative if expired)
		duration[0] = opaque->key->expires - now ;
		if (duration[0] > 0) {
			LEVEL_DEBUG("Value found in cache. Remaining life: %d seconds.",duration[0]);
			// Compared with >= before, but fc_second(1) always cache for 2 seconds in that case.
			// Very noticable when reading time-data like "/26.80A742000000/date" for example.
			if ( dsize[0] >= opaque->key->dsize) {
				// lower data size if stored value is shorter
				dsize[0] = opaque->key->dsize;
				//tree_show(opaque,leaf,0);
				if (dsize[0] > 0) {
					memcpy(data, TREE_DATA(opaque->key), dsize[0]);
				}
				ctr_ret = ctr_ok;
				//twalk(cache.temporary_tree_new,tree_show) ;
			} else {
				ctr_ret = ctr_size_mismatch;
			}
		} else {
			LEVEL_DEBUG("Value found in cache, but expired by %d seconds.",-duration[0]);
			ctr_ret = ctr_expired;
		}
	} else {
		LEVEL_DEBUG("Value not found in cache");
		ctr_ret = ctr_not_found;
	}
	CACHE_RUNLOCK;
	return ctr_ret;
}

/* Look in caches, 0=found and valid, 1=not or uncachable in the first place */
static enum cache_task_return Cache_Get_Persistent(void *data, size_t * dsize, time_t * duration, const struct tree_node *tn)
{
	struct tree_opaque *opaque;
	enum cache_task_return ctr_ret;
	(void) duration; // ignored -- no timeout
	PERSISTENT_RLOCK;
	opaque = tfind(tn, &cache.persistent_tree, tree_compare) ;
	if ( opaque != NULL ) {
		if ( dsize[0] >= opaque->key->dsize) {
			dsize[0] = opaque->key->dsize;
			if (dsize[0] > 0) {
				memcpy(data, TREE_DATA(opaque->key), dsize[0]);
			}
			ctr_ret = ctr_ok;
		} else {
			ctr_ret = ctr_size_mismatch;
		}
	} else {
		ctr_ret = ctr_not_found;
	}
	PERSISTENT_RUNLOCK;
	return ctr_ret;
}

static void Del_Stat(struct cache_stats *scache, const int result)
{
	if ( GOOD( result)) {
		STAT_ADD1(scache->deletes);
	}
}

void OWQ_Cache_Del(struct one_wire_query *owq)
{
	Cache_Del(PN(owq));
}

void OWQ_Cache_Del_ALL(struct one_wire_query *owq)
{
	struct parsedname * pn = PN(owq) ; // convenience
	int extension = pn->extension ; // store extension
	
	pn->extension = EXTENSION_ALL ; // temporary assignment
	Cache_Del(pn);
	pn->extension = extension ; // restore extension
}

void OWQ_Cache_Del_BYTE(struct one_wire_query *owq)
{
	struct parsedname * pn = PN(owq) ; // convenience
	int extension = pn->extension ; // store extension
	
	pn->extension = EXTENSION_BYTE ; // temporary assignment
	Cache_Del(pn);
	pn->extension = extension ; // restore extension
}

void OWQ_Cache_Del_parts(struct one_wire_query *owq)
{
	struct parsedname * pn = PN(owq) ; // convenience
	
	if ( pn->selected_filetype->ag != NON_AGGREGATE ) {
		int extension = pn->extension ; // store extension
		int extension_index ;
		for ( extension_index = pn->selected_filetype->ag->elements - 1 ; extension_index >= 0  ; -- extension_index ) {
			pn->extension = extension_index ; // temporary assignment
			Cache_Del(pn);
		}
		pn->extension = extension ; // restore extension
	} else {
		Cache_Del(pn);
	}
}

// Delete a serial number's alias
// Safe to call if no alias exists
// Also deletes from linked alias_sn file
void Cache_Del_Alias(const BYTE * sn)
{
	ASCII * alias_name ;
	struct tree_node *tn;
	size_t size ;

	alias_name = Cache_Get_Alias( sn ) ;
	if ( alias_name == NULL ) {
		// doesn't exist
		// (or a memory error -- unlikely)
		return ;
	}

	LEVEL_DEBUG("Deleting alias %s from "SNformat, alias_name, SNvar(sn)) ;
	size = strlen( alias_name ) ;
	tn = (struct tree_node *) owmalloc(sizeof(struct tree_node) + size + 1 );
	if ( tn != NULL ) {
		tn->expires = NOW_TIME;
		tn->dsize = size;
		memcpy((ASCII *)TREE_DATA(tn), alias_name, size+1); // includes NULL
		LoadTK( sn, Alias_Marker, 0, tn ) ;
		Del_Stat(&cache_pst, Cache_Del_Persistent(tn));
		Cache_Del_Alias_SN( alias_name ) ;
	}
	owfree( alias_name ) ;
}

static void Cache_Del(const struct parsedname *pn)
{
	struct tree_node tn;
	time_t duration;
	//printf("Cache_Del\n") ;
	if (!pn) {
		return;	// do check here to avoid needless processing
	}
	
	duration = TimeOut(pn->selected_filetype->change);
	if (duration <= 0) {
		return;				/* in case timeout set to 0 */
	}
	
	LoadTK( pn->sn, pn->selected_filetype, pn->extension, &tn ) ;
	switch (pn->selected_filetype->change) {
		case fc_persistent:
			Del_Stat(&cache_pst, Cache_Del_Persistent(&tn));
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
				Del_Stat(&cache_pst, Cache_Del_Persistent(&tn));
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
			Del_Stat(&cache_pst, Cache_Del_Persistent(&tn));
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
		Del_Stat(&cache_pst, Cache_Del_Persistent(&tn));
		break;
	default:
		Del_Stat(&cache_int, Cache_Del_Common(&tn));
		break;
	}
}

static GOOD_OR_BAD Cache_Del_Common(const struct tree_node *tn)
{
	struct tree_opaque *opaque;
	time_t now = NOW_TIME;
	GOOD_OR_BAD ret = gbBAD;
	LEVEL_DEBUG("Delete from cache sn " SNformat " in=%p index=%d", SNvar(tn->tk.sn), tn->tk.p, tn->tk.extension);

	CACHE_WLOCK;
	opaque = tfind(tn, &cache.temporary_tree_new, tree_compare) ;
	if ( opaque == NULL ) {
		// not in new tree
		if ( cache.time_to_kill > now ) {
			// old tree still alive
			opaque = tfind(tn, &cache.temporary_tree_old, tree_compare) ;
		}
	}
	if ( opaque != NULL ) {
		opaque->key->expires = now - 1;
		ret = gbGOOD;
	}
	CACHE_WUNLOCK;

	return ret;
}

static GOOD_OR_BAD Cache_Del_Persistent(const struct tree_node *tn)
{
	struct tree_opaque *opaque;
	struct tree_node *tn_found = NULL;

	PERSISTENT_WLOCK;
	opaque = tfind(tn, &cache.persistent_tree, tree_compare) ;
	if ( opaque != NULL ) {
		tn_found = opaque->key;
		tdelete(tn, &cache.persistent_tree, tree_compare);
	}
	PERSISTENT_WUNLOCK;

	if ( tn_found == NULL ) {
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

// Alias list from persistent cache
// formatted as an alias file:
// NNNNNNNNNNNN=alias_name\n
//
// Need to protect a global variable (aliaslist_cb) since twalk has no way of sending user data.

struct memblob * aliaslist_mb ;

static void Aliaslistaction(const void *node, const VISIT which, const int depth)
{
	const struct tree_node *p = *(struct tree_node * const *) node;
	(void) depth;
	char SN_address[SERIAL_NUMBER_SIZE*2] ;
	
	switch (which) {
	case leaf:
	case postorder:
		if ( p->tk.p != Alias_Marker ) {
			return ;
		}
		// Add sn address
		bytes2string(SN_address, p->tk.sn, SERIAL_NUMBER_SIZE);
		MemblobAdd( (BYTE *) SN_address, SERIAL_NUMBER_SIZE*2, aliaslist_mb ) ;
		// Add '='
		MemblobAdd( (BYTE *) "=", 1, aliaslist_mb ) ;
		// Add alias name
		MemblobAdd( (const BYTE *)CONST_TREE_DATA(p), p->dsize, aliaslist_mb ) ;
		// Add <CR>
		MemblobAdd( (BYTE *) "\x0D\x0A", 2, aliaslist_mb ) ;
		return ;
	case preorder:
	case endorder:
		break;
	}
}

void Aliaslist( struct memblob * mb  )
{
	PERSISTENT_RLOCK ;
	ALIASLISTLOCK ;
	aliaslist_mb = mb ;
	twalk(cache.persistent_tree, Aliaslistaction);
	ALIASLISTUNLOCK ;
	PERSISTENT_RUNLOCK ;
}

/* Add an alias to the temporary database of name->bus */
/* alias_name is a null-terminated string */
void Cache_Add_Alias_Bus(const ASCII * alias_name, INDEX_OR_ERROR bus)
{
	// allocate space for the node and data
	size_t datasize = strlen(alias_name) ;
	struct alias_tree_node *atn = (struct alias_tree_node *) owmalloc(sizeof(struct alias_tree_node) + datasize + 1 );
	time_t duration = TimeOut(fc_presence);

	if (atn==NULL) {
		return ;
	}

	if (datasize==0) {
		owfree(atn) ;
		return ;
	}

	// populate the node structure with data
	atn->expires = duration + NOW_TIME;
	atn->size = datasize ;
	atn->bus = bus ;
	memcpy( ALIAS_TREE_DATA(atn), alias_name, datasize + 1 ) ;
	
	Cache_Add_Alias_Common( atn ) ;
}

/* Add an item to the alias cache */
/* retire the cache (flip) if too old, and start a new one (keep the old one for a while) */
/* return 0 if good, 1 if not */
static void Cache_Add_Alias_Common(struct alias_tree_node *atn)
{
	struct tree_opaque *opaque;

	CACHE_WLOCK;
	if (cache.time_to_kill < NOW_TIME) {	// old database has timed out
		FlipTree() ;
	}
	if (Globals.cache_size && (cache.old_ram_size + cache.new_ram_size > Globals.cache_size)) {
		// failed size test
		owfree(atn);
	} else if ((opaque = tsearch(atn, &cache.temporary_alias_tree_new, alias_tree_compare))) {
		if ( (void *)atn != (void *) (opaque->key) ) {
			cache.new_ram_size += sizeof(atn) - sizeof(opaque->key);
			owfree(opaque->key);
			opaque->key = (void *) atn;
		} else {
			cache.new_ram_size += sizeof(atn);
		}
	} else {					// nothing found or added?!? free our memory segment
		owfree(atn);
	}
	CACHE_WUNLOCK;
}

/* Add an alias/sn to the persistent database of name->sn */
/* Alias name must be a null-terminated string */
static void Cache_Add_Alias_SN(const ASCII * alias_name, const BYTE * sn)
{
	// allocate space for the node and data
	size_t datasize = strlen(alias_name) ;
	struct alias_tree_node *atn = (struct alias_tree_node *) owmalloc(sizeof(struct alias_tree_node) + datasize + 1);

	if (atn==NULL) {
		return ;
	}

	if (datasize==0) {
		owfree(atn) ;
		return ;
	}

	// populate the node structure with data
	atn->expires = NOW_TIME;
	atn->size = datasize;
	memcpy( atn->sn, sn, SERIAL_NUMBER_SIZE ) ;
	memcpy( ALIAS_TREE_DATA(atn), alias_name, datasize+1 ) ;
	
	Cache_Add_Alias_Persistent( atn ) ;
}

static void Cache_Add_Alias_Persistent(struct alias_tree_node *atn)
{
	struct tree_opaque *opaque;

	PERSISTENT_WLOCK;
	opaque = tsearch(atn, &cache.persistent_alias_tree, alias_tree_compare) ;
	if ( opaque != NULL ) {
		if ( (void *) atn != (void *) (opaque->key) ) {
			owfree(opaque->key);
			opaque->key = (void *) atn;
		}
	} else {					// nothing found or added?!? free our memory segment
		owfree(atn);
	}
	PERSISTENT_WUNLOCK;
}

/* Find bus from alias name */
/* Alias name must be a null-terminated string */
INDEX_OR_ERROR Cache_Get_Alias_Bus(const ASCII * alias_name)
{
	// allocate space for the node and data
	size_t datasize = strlen(alias_name) ;
	struct alias_tree_node *atn = (struct alias_tree_node *) owmalloc(sizeof(struct alias_tree_node) + datasize + 1);

	if (atn==NULL) {
		return INDEX_BAD ;
	}

	if (datasize==0) {
		owfree(atn) ;
		return INDEX_BAD ;
	}

	// populate the node structure with data
	atn->size = datasize;
	memcpy( ALIAS_TREE_DATA(atn), alias_name, datasize+1 ) ;
	
	return Cache_Get_Alias_Common( atn ) ;
}

static INDEX_OR_ERROR Cache_Get_Alias_Common( struct alias_tree_node * atn)
{
	INDEX_OR_ERROR bus = INDEX_BAD;
	time_t now = NOW_TIME;
	struct tree_opaque *opaque;
	
	CACHE_RLOCK;
	opaque = tfind(atn, &cache.temporary_alias_tree_new, alias_tree_compare) ;
	if ( opaque == NULL ) {
		// try old tree
		opaque = tfind(atn, &cache.temporary_alias_tree_old, alias_tree_compare) ;
	}
	if ( opaque != NULL ) {
		// test expiration
		if ( ((struct alias_tree_node *)(opaque->key))->expires > now) {
			bus = ((struct alias_tree_node *)(opaque->key))->bus ;
			LEVEL_DEBUG("Found %s on bus.%d\n",ALIAS_TREE_DATA(atn),bus) ;
		}
	}
	CACHE_RUNLOCK;
	LEVEL_DEBUG("Finding %s unsuccessful\n",ALIAS_TREE_DATA(atn)) ;
	owfree(atn) ;
	return bus;
}

/* sn must point to an 8 byte buffer */
/* Alias name must be a null-terminated string */
GOOD_OR_BAD Cache_Get_Alias_SN(const ASCII * alias_name, BYTE * sn )
{
	// allocate space for the node and data
	size_t datasize = strlen(alias_name) ;
	struct alias_tree_node *atn ;

	if (datasize==0) {
		return gbBAD ;
	}

	atn = (struct alias_tree_node *) owmalloc(sizeof(struct alias_tree_node) + datasize+1);
	if (atn==NULL) {
		return gbBAD ;
	}

	// populate the node structure with data
	atn->size = datasize;
	memcpy( ALIAS_TREE_DATA(atn), alias_name, datasize+1 ) ;
	
	return Cache_Get_Alias_Persistent( sn, atn )==ctr_ok ? gbGOOD : gbBAD  ;
}

/* look in persistent alias->sn tree */
static enum cache_task_return Cache_Get_Alias_Persistent( BYTE * sn, struct alias_tree_node * atn)
{
	struct tree_opaque *opaque;
	GOOD_OR_BAD ret = gbBAD ;

	PERSISTENT_RLOCK;
	opaque = tfind(atn, &cache.persistent_alias_tree, alias_tree_compare) ;
	if ( opaque != NULL ) {
		memcpy( sn, ((struct alias_tree_node *)(opaque->key))->sn, SERIAL_NUMBER_SIZE ) ;
		LEVEL_DEBUG("Lookup of %s gives "SNformat, CONST_ALIAS_TREE_DATA(atn), SNvar(sn) ) ;
		ret = gbGOOD ;
	} else {
		LEVEL_DEBUG("Lookup of %s unsuccessful",CONST_ALIAS_TREE_DATA(atn)) ;
	}
	PERSISTENT_RUNLOCK;
	owfree(atn) ;
	return ret;
}

/* Alias name must be a null-terminated string */
static void Cache_Del_Alias_SN(const ASCII * alias_name)
{
	// allocate space for the node and data
	size_t datasize = strlen(alias_name) ;
	struct alias_tree_node *atn = (struct alias_tree_node *) owmalloc(sizeof(struct alias_tree_node) + datasize + 1);

	if (atn==NULL) {
		return ;
	}

	// populate the node structure with data
	atn->expires = NOW_TIME;
	atn->size = datasize;
	memcpy( ALIAS_TREE_DATA(atn), alias_name, datasize+1 ) ;
	
	Cache_Del_Alias_Persistent( atn ) ;
}

/* look in persistent alias->sn tree */
static void Cache_Del_Alias_Persistent( struct alias_tree_node * atn)
{
	struct tree_opaque *opaque;
	struct alias_tree_node *atn_found = NULL;

	PERSISTENT_RLOCK;
	opaque = tfind(atn, &cache.persistent_alias_tree, alias_tree_compare) ;
	if ( opaque != NULL ) {
		atn_found = (struct alias_tree_node *) (opaque->key);
	}
	PERSISTENT_RUNLOCK;
	owfree(atn_found) ;
}

/* Delete bus from alias name */
/* Alias name must be a null-terminated string */
void Cache_Del_Alias_Bus(const ASCII * alias_name)
{
	// Cheat -- just change to a bad bus value
	LEVEL_DEBUG("Hide %s\n",alias_name) ;
	Cache_Add_Alias_Bus( alias_name, INDEX_BAD ) ;
}


#endif							/* OW_CACHE */
