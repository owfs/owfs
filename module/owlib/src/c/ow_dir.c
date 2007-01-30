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

#include <config.h>
#include "owfs_config.h"
#include "ow_devices.h"
#include "ow_counters.h"
#include "ow_connection.h"
#include "ow_dirblob.h"

static int FS_dir_both(void (*dirfunc) (void *, const struct parsedname *),
					   void *v, const struct parsedname *pn,
					   uint32_t * flags);
static int
FS_dir_seek(void (*dirfunc) (void *, const struct parsedname * const),
			void *v, struct connection_in *in, const struct parsedname *pn,
			uint32_t * flags);
static int
FS_devdir(void (*dirfunc) (void *, const struct parsedname * const),
		  void *v, struct parsedname *pn2);
static int
FS_alarmdir(void (*dirfunc) (void *, const struct parsedname * const),
			void *v, struct parsedname *pn2);
static int
FS_typedir(void (*dirfunc) (void *, const struct parsedname * const),
		   void *v, struct parsedname *pn2);
static int
FS_realdir(void (*dirfunc) (void *, const struct parsedname * const),
		   void *v, struct parsedname *pn2, uint32_t * flags);
static int
FS_cache2real(void (*dirfunc) (void *, const struct parsedname * const),
			  void *v, struct parsedname *pn2, uint32_t * flags);
static int FS_busdir(void (*dirfunc) (void *, const struct parsedname *),
					 void *v, const struct parsedname *pn);

/* Calls dirfunc() for each element in directory */
/* void * data is arbitrary user data passed along -- e.g. output file descriptor */
/* pn -- input:
    pn->dev == NULL -- root directory, give list of all devices
    pn->dev non-null, -- device directory, give all properties
    branch aware
    cache aware

   pn -- output (with each call to dirfunc)
    ROOT
    pn->dev set
    pn->sn set appropriately
    pn->ft not set

    DEVICE
    pn->dev and pn->sn still set
    pn->ft loops through
*/
/* path is the path which "pn" parses */
/* FS_dir produces the "invariant" portion of the directory, passing on to
   FS_dir_seek the variable part */
int FS_dir(void (*dirfunc) (void *, const struct parsedname *),
		   void *v, const struct parsedname *pn)
{
	uint32_t flags;

	return FS_dir_both(dirfunc, v, pn, &flags);
}

/* path is the path which "pn" parses */
/* FS_dir_remote is the entry into FS_dir_seek from ServerDir */
/* More checking is done, and the flags are returned */
int FS_dir_remote(void (*dirfunc) (void *, const struct parsedname *),
				  void *v, const struct parsedname *pn, uint32_t * flags)
{
	return FS_dir_both(dirfunc, v, pn, flags);
}


static int FS_dir_both(void (*dirfunc) (void *, const struct parsedname *),
					   void *v, const struct parsedname *pn,
					   uint32_t * flags)
{
	int ret = 0;
	struct parsedname pn2;

	/* initialize flags */
	flags[0] = 0;
	if (pn == NULL || pn->in == NULL)
		return -ENODEV;
	LEVEL_CALL("DIRECTORY path=%s\n", SAFESTRING(pn->path));

	STATLOCK;
	AVERAGE_IN(&dir_avg)
		AVERAGE_IN(&all_avg)
		STATUNLOCK;
	FSTATLOCK;
	dir_time = time(NULL);		// protected by mutex
	FSTATUNLOCK;

	/* Make a copy (shallow) of pn to modify for directory entries */
	memcpy(&pn2, pn, sizeof(struct parsedname));	/*shallow copy */

	if (pn->ft) {
		ret = -ENOTDIR;
	} else if (pn->dev) {		/* device directory */
		if (pn->type == pn_structure) {
			/* Device structure is always known for ordinary devices, so don't
			 * bother calling ServerDir() */
			ret = FS_devdir(dirfunc, v, &pn2);
		} else if (SpecifiedBus(pn) && BusIsServer(pn->in)) {
			ret = ServerDir(dirfunc, v, &pn2, flags);
		} else {
			ret = FS_devdir(dirfunc, v, &pn2);
		}
	} else if (IsAlarmDir(pn)) {	/* root or branch directory -- alarm state */
		//printf("ALARM\n");
		ret = SpecifiedBus(pn) ? FS_alarmdir(dirfunc, v, &pn2)
			: FS_dir_seek(dirfunc, v, pn2.in, &pn2, flags);
	} else if (NotRealDir(pn)) {	/* stat, sys or set dir */
		/* there are some files with variable sizes, and /system/adapter have variable
		 * number of entries and we have to call ServerDir() */
		ret = (SpecifiedBus(pn) && BusIsServer(pn->in))
			? ServerDir(dirfunc, v, &pn2, flags)
			: FS_typedir(dirfunc, v, &pn2);
	} else {					/* Directory of some kind */
		if (pn->pathlength == 0) {	/* root directory */
			if ((Global.opt != opt_server) && !SpecifiedBus(pn) && NotUncachedDir(pn)) {	/* structure only in true root */
				pn2.type = pn_structure;
				dirfunc(v, &pn2);
				pn2.type = pn_real;
			}
			if (!SpecifiedBus(pn) && ShouldReturnBusList(pn)) {
				/* restore state */
				pn2.type = pn_real;
				FS_busdir(dirfunc, v, pn);
				if (NotUncachedDir(pn)) {
					if (IsLocalCacheEnabled(pn)) {	/* cached */
						pn2.state = pn->state | pn_uncached;
						dirfunc(v, &pn2);
						pn2.state = pn->state;
					}
					pn2.type = pn_settings;
					dirfunc(v, &pn2);
					pn2.type = pn_system;
					dirfunc(v, &pn2);
					pn2.type = pn_statistics;
					dirfunc(v, &pn2);
				}
				pn2.type = pn->type;
			}
		}
		/* Now get the actual devices */
		ret = SpecifiedBus(pn) ? FS_cache2real(dirfunc, v, &pn2, flags)
			: FS_dir_seek(dirfunc, v, pn2.in, &pn2, flags);
	}
	if (Global.opt != opt_server) {
		if (NotAlarmDir(pn)) {
			/* don't show alarm directory in alarm directory */
			/* alarm directory */
			if (flags[0] & DEV_alarm) {
				pn2.state = (pn_alarm | (pn->state & pn_text));
				dirfunc(v, &pn2);
				pn2.state = pn->state;
			}
		}
		/* simultaneous directory */
		if (flags[0] & (DEV_temp | DEV_volt)) {
			pn2.dev = DeviceSimultaneous;
			dirfunc(v, &pn2);
		}
	}
	STATLOCK;
	AVERAGE_OUT(&dir_avg)
		AVERAGE_OUT(&all_avg)
		STATUNLOCK;
	//printf("FS_dir out ret=%d\n", ret);
	return ret;
}

/* path is the path which "pn" parses */
/* FS_dir_seek produces the data that can vary: device lists, etc. */
#if OW_MT
struct dir_seek_struct {
	struct connection_in *in;
	const struct parsedname *pn;
	void (*dirfunc) (void *, const struct parsedname *);
	void *v;
	uint32_t *flags;
	int ret;
};

/* Embedded function */
static void *FS_dir_seek_callback(void *v)
{
	struct dir_seek_struct *dss = v;
	dss->ret =
		FS_dir_seek(dss->dirfunc, dss->v, dss->in, dss->pn, dss->flags);
	pthread_exit(NULL);
	return NULL;
}
static int FS_dir_seek(void (*dirfunc) (void *, const struct parsedname *),
					   void *v, struct connection_in *in,
					   const struct parsedname *pn, uint32_t * flags)
{
	int ret = 0;
	struct dir_seek_struct dss = { in->next, pn, dirfunc, v, flags, 0 };
	struct parsedname pn2;
	pthread_t thread;
	int threadbad = 1;

	if (!KnownBus(pn)) {
		threadbad = in->next == NULL
			|| pthread_create(&thread, NULL, FS_dir_seek_callback,
							  (void *) (&dss));
	}

	memcpy(&pn2, pn, sizeof(struct parsedname));	// shallow copy
	pn2.in = in;

	if (TestConnection(&pn2)) {	// reconnect ok?
		ret = -ECONNABORTED;
	} else if (KnownBus(pn) && BusIsServer(in)) {	/* is this a remote bus? */
		//printf("FS_dir_seek: Call ServerDir %s\n", pn->path);
		ret = ServerDir(dirfunc, v, &pn2, flags);
	} else {					/* local bus */
		if (IsAlarmDir(pn)) {	/* root or branch directory -- alarm state */
			//printf("FS_dir_seek: Call FS_alarmdir %s\n", pn->path);
			ret = FS_alarmdir(dirfunc, v, &pn2);
		} else {
			//printf("FS_dir_seek: call FS_cache2real bus %d\n", pn->in->index);
			ret = FS_cache2real(dirfunc, v, &pn2, flags);
		}
	}
	//printf("FS_dir_seek4 pid=%ld adapter=%d ret=%d\n",pthread_self(), pn->in->index,ret);
	/* See if next bus was also queried */
	if (threadbad == 0) {		/* was a thread created? */
		void *vo;
		if (pthread_join(thread, &vo))
			return ret;			/* wait for it (or return only this result) */
		if (dss.ret >= 0)
			return dss.ret;		/* is it an error return? Then return this one */
	}
	return ret;
}

#else							/* OW_MT */

/* path is the path which "pn" parses */
/* FS_dir_seek produces the data that can vary: device lists, etc. */
static int FS_dir_seek(void (*dirfunc) (void *, const struct parsedname *),
					   void *v, struct connection_in *in,
					   const struct parsedname *pn, uint32_t * flags)
{
	int ret = 0;
	struct parsedname pn2;

	memcpy(&pn2, pn, sizeof(struct parsedname));	//shallow copy
	pn2.in = in;
	if (TestConnection(&pn2)) {	// reconnect ok?
		ret = -ECONNABORTED;
	} else if (KnownBus(pn) && BusIsServer(in)) {	/* is this a remote bus? */
		//printf("FS_dir_seek: Call ServerDir %s\n", pn->path);
		ret = ServerDir(dirfunc, v, &pn2, flags);
	} else {					/* local bus */
		if (IsAlarmDir(&pn2)) {	/* root or branch directory -- alarm state */
			//printf("FS_dir_seek: Call FS_alarmdir %s\n", pn->path);
			ret = FS_alarmdir(dirfunc, v, &pn2);
		} else {
			//printf("FS_dir_seek: call FS_cache2real bus %d\n", pn->in->index);
			ret = FS_cache2real(dirfunc, v, &pn2, flags);
		}
	}
	//printf("FS_dir_seek4 pid=%ld adapter=%d ret=%d\n",pthread_self(), pn->in->index,ret);
	if (in->next && ret <= 0)
		return FS_dir_seek(dirfunc, v, in->next, pn, flags);
	return ret;
}
#endif							/* OW_MT */

/* Device directory -- all from memory */
static int FS_devdir(void (*dirfunc) (void *, const struct parsedname *),
					 void *v, struct parsedname *pn2)
{
	struct filetype *lastft = &(pn2->dev->ft[pn2->dev->nft]);	/* last filetype struct */
	struct filetype *firstft;	/* first filetype struct */
	char s[33];
	size_t len;

	STAT_ADD1(dir_dev.calls);
	if (pn2->subdir) {			/* indevice subdir, name prepends */
//printf("DIR device subdirectory\n");
		len = snprintf(s, 32, "%s/", pn2->subdir->name);
		firstft = pn2->subdir + 1;
	} else {
//printf("DIR device directory\n");
		len = 0;
		firstft = pn2->dev->ft;
	}
	for (pn2->ft = firstft; pn2->ft < lastft; ++pn2->ft) {	/* loop through filetypes */
		if (len) {				/* subdir */
			/* test start of name for directory name */
			if (strncmp(pn2->ft->name, s, len))
				break;
		} else {				/* primary device directory */
			if (strchr(pn2->ft->name, '/'))
				continue;
		}
		if (pn2->ft->ag) {
			for (pn2->extension =
				 (pn2->ft->format == ft_bitfield) ? -2 : -1;
				 pn2->extension < pn2->ft->ag->elements;
				 ++pn2->extension) {
				dirfunc(v, pn2);
				STAT_ADD1(dir_dev.entries);
			}
		} else {
			pn2->extension = 0;
			dirfunc(v, pn2);
			STAT_ADD1(dir_dev.entries);
		}
	}
	return 0;
}

/* Note -- alarm directory is smaller, no adapters or stats or uncached */
static int FS_alarmdir(void (*dirfunc) (void *, const struct parsedname *),
					   void *v, struct parsedname *pn2)
{
	int ret;
	uint32_t flags;
	struct device_search ds;	// holds search state

	/* cache from Server if this is a remote bus */
	if (BusIsServer(pn2->in))
		return ServerDir(dirfunc, v, pn2, &flags);

	/* STATISCTICS */
	STAT_ADD1(dir_main.calls);
//printf("DIR alarm directory\n");

	BUSLOCK(pn2);
	pn2->ft = NULL;				/* just in case not properly set */
	ret = BUS_first_alarm(&ds, pn2);
	if (ret) {
		BUSUNLOCK(pn2);
		if (ret == -ENODEV)
			return 0;			/* no more alarms is ok */
		return ret;
	}
	while (ret == 0) {
		STAT_ADD1(dir_main.entries);
		memcpy(pn2->sn, ds.sn, 8);
		/* Search for known 1-wire device -- keyed to device name (family code in HEX) */
		FS_devicefindhex(ds.sn[0], pn2);	// lookup ID and set pn2.dev
		DIRLOCK;
		dirfunc(v, pn2);
		DIRUNLOCK;
		pn2->dev = NULL;		/* clear for the rest of directory listing */
		ret = BUS_next(&ds, pn2);
//printf("ALARM sn: "SNformat" ret=%d\n",SNvar(sn),ret);
	}
	BUSUNLOCK(pn2);
	if (ret == -ENODEV)
		return 0;				/* no more alarms is ok */
	return ret;
}

/* A directory of devices -- either main or branch */
/* not within a device, nor alarm state */
/* Also, adapters and stats handled elsewhere */
/* Scan the directory from the BUS and add to cache */
static int FS_realdir(void (*dirfunc) (void *, const struct parsedname *),
					  void *v, struct parsedname *pn2, uint32_t * flags)
{
	struct device_search ds;
	size_t devices = 0;
	struct dirblob db;
	int ret;

	/* cache from Server if this is a remote bus */
	if (BusIsServer(pn2->in))
		return ServerDir(dirfunc, v, pn2, flags);

	/* STATISTICS */
	STAT_ADD1(dir_main.calls);

	DirblobInit(&db);			// set up a fresh dirblob

	/* Operate at dev level, not filetype */
	pn2->ft = NULL;
	flags[0] = 0;				/* start out with no flags set */

	BUSLOCK(pn2);

	/* it appears that plugging in a new device sends a "presence pulse" that screws up BUS_first */
	if ((ret = BUS_first(&ds, pn2))) {
		BUSUNLOCK(pn2);
		if (ret == -ENODEV) {
			if (pn2->pathlength == 0)
				pn2->in->last_root_devs = 0;	// root dir estimated length
			return 0;			/* no more devices is ok */
		}
		return -EIO;
	}
	/* BUS still locked */
	if (pn2->pathlength == 0)
		db.allocated = pn2->in->last_root_devs;	// root dir estimated length
	do {
		BUSUNLOCK(pn2);
		if (DirblobPure(&db)) {	/* only add if there is a blob allocated successfully */
			DirblobAdd(ds.sn, &db);
		}
		++devices;

		memcpy(pn2->sn, ds.sn, 8);
		/* Add to Device location cache */
		Cache_Add_Device(pn2->in->index, pn2);
		/* Search for known 1-wire device -- keyed to device name (family code in HEX) */
		FS_devicefindhex(ds.sn[0], pn2);	// lookup ID and set pn2.dev

		DIRLOCK;
		dirfunc(v, pn2);
		flags[0] |= pn2->dev->flags;
		DIRUNLOCK;
		pn2->dev = NULL;		/* clear for the rest of directory listing */

		BUSLOCK(pn2);
	} while ((ret = BUS_next(&ds, pn2)) == 0);
	/* BUS still locked */
	if (pn2->pathlength == 0 && ret == -ENODEV)
		pn2->in->last_root_devs = devices;	// root dir estimated length
	BUSUNLOCK(pn2);

	/* Add to the cache (full list as a single element */
	if (DirblobPure(&db) && ret == -ENODEV) {
		Cache_Add_Dir(&db, pn2);	// end with a null entry
	}
	DirblobClear(&db);

	STATLOCK;
	dir_main.entries += devices;
	STATUNLOCK;
	if (ret == -ENODEV)
		return 0;				// no more devices is ok */
	return ret;
}

/* points "serial number" to directory
   -- 0 for root
   -- DS2409/mai|aux for branch
   -- DS2409 needs only the last element since each DS2409 is unique
   */
void FS_LoadPath(BYTE * sn, const struct parsedname *pn)
{
	if (pn->pathlength == 0) {
		memset(sn, 0, 8);
	} else {
		memcpy(sn, pn->bp[pn->pathlength - 1].sn, 7);
		sn[7] = pn->bp[pn->pathlength - 1].branch;
	}
}

/* A directory of devices -- either main or branch */
/* not within a device, nor alarm state */
/* Also, adapters and stats handled elsewhere */
/* Cache2Real try the cache first, else can directory from bus (and add to cache) */
static int
FS_cache2real(void (*dirfunc) (void *, const struct parsedname *), void *v,
			  struct parsedname *pn2, uint32_t * flags)
{
	size_t dindex;
	struct dirblob db;

	/* Test to see whether we should get the directory "directly" */
	//printf("Pre test cache for dir\n") ;
	if (SpecifiedBus(pn2) || IsUncachedDir(pn2) || Cache_Get_Dir(&db, pn2)) {
		//printf("FS_cache2real: didn't find anything at bus %d\n", pn2->in->index);
		return FS_realdir(dirfunc, v, pn2, flags);
	}
	//printf("Post test cache for dir, snlist=%p, devices=%lu\n",snlist,devices) ;
	/* We have a cached list in snlist. Note that we have to free this memory */
	/* STATISCTICS */
	STAT_ADD1(dir_main.calls);

	/* Get directory from the cache */
	for (dindex = 0; DirblobGet(dindex, pn2->sn, &db) == 0; ++dindex) {
		/* Search for known 1-wire device -- keyed to device name (family code in HEX) */
		FS_devicefindhex(pn2->sn[0], pn2);	// lookup ID and set pn2.dev
		DIRLOCK;
		dirfunc(v, pn2);
		flags[0] |= pn2->dev->flags;
		DIRUNLOCK;
	}
	DirblobClear(&db);			/* allocated in Cache_Get_Dir */
	pn2->dev = NULL;			/* clear for the rest of directory listing */
	STATLOCK;
	dir_main.entries += dindex;
	STATUNLOCK;
	return 0;
}

#if OW_MT
// must lock a global struct for walking through tree -- limitation of "twalk"
pthread_mutex_t Typedirmutex = PTHREAD_MUTEX_INITIALIZER;
#define TYPEDIRMUTEXLOCK  pthread_mutex_lock(&Typedirmutex)
#define TYPEDIRMUTEXUNLOCK  pthread_mutex_unlock(&Typedirmutex)
#else /* OW_MT */
#define TYPEDIRMUTEXLOCK
#define TYPEDIRMUTEXUNLOCK
#endif							/* OW_MT */

// struct for walkking through tree -- cannot send data except globally
struct {
	void (*dirfunc) (void *, const struct parsedname *);
	void *v;
	struct parsedname *pn;
} tda;
static void Typediraction(const void *t, const VISIT which,
						  const int depth)
{
	(void) depth;
	switch (which) {
	case leaf:
	case postorder:
		tda.pn->dev = ((const struct device_opaque *) t)->key;
		(tda.dirfunc) (tda.v, tda.pn);
	default:
		break;
	}
}

/* Show the pn->type (statistics, system, ...) entries */
/* Only the top levels, the rest will be shown by FS_devdir */
static int FS_typedir(void (*dirfunc) (void *, const struct parsedname *),
					  void *v, struct parsedname *pn2)
{
    TYPEDIRMUTEXLOCK ;

    tda.dirfunc = dirfunc;
	tda.v = v;
	tda.pn = pn2;
	twalk(Tree[pn2->type], Typediraction);

    TYPEDIRMUTEXUNLOCK ;

    return 0;
}

/* Show the bus entries */
static int FS_busdir(void (*dirfunc) (void *, const struct parsedname *),
					 void *v, const struct parsedname *pn)
{
	struct parsedname pn2;
	int bus_number;

	memcpy(&pn2, pn, sizeof(struct parsedname));	// shallow copy

	for (bus_number = 0; bus_number < indevices; ++bus_number) {
		SetKnownBus(bus_number, &pn2);
		dirfunc(v, &pn2);
	}

	return 0;
}
