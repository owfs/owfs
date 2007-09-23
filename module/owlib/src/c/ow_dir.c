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
					   void *v, const struct parsedname *pn_directory,
					   uint32_t * flags);
static int
FS_dir_seek(void (*dirfunc) (void *, const struct parsedname * const),
			void *v, struct connection_in *in, const struct parsedname *pn_directory,
			uint32_t * flags);
static int
FS_devdir(void (*dirfunc) (void *, const struct parsedname * const),
		  void *v, struct parsedname *pn2);
static int
FS_alarmdir(void (*dirfunc) (void *, const struct parsedname * const),
			void *v, struct parsedname *pn2);
static int
FS_typedir(void (*dirfunc) (void *, const struct parsedname * const),
		   void *v, struct parsedname *pn_type_directory);
static int
FS_realdir(void (*dirfunc) (void *, const struct parsedname * const),
		   void *v, struct parsedname *pn2, uint32_t * flags);
static int
FS_cache2real(void (*dirfunc) (void *, const struct parsedname * const),
			  void *v, struct parsedname *pn2, uint32_t * flags);
static int FS_busdir(void (*dirfunc) (void *, const struct parsedname *),
					 void *v, const struct parsedname *pn_directory);

static void FS_stype_dir(void (*dirfunc) (void *, const struct parsedname *),
                      void *v, struct parsedname *pn_root_directory) ;
static void FS_structure_dir(void (*dirfunc) (void *, const struct parsedname *),
                      void *v, struct parsedname *pn_root_directory);
static void FS_uncached_dir(void (*dirfunc) (void *, const struct parsedname *),
                      void *v, struct parsedname *pn_root_directory) ;

/* Calls dirfunc() for each element in directory */
/* void * data is arbitrary user data passed along -- e.g. output file descriptor */
/* pn_directory -- input:
    pn_directory->selected_device == NULL -- root directory, give list of all devices
    pn_directory->selected_device non-null, -- device directory, give all properties
    branch aware
    cache aware

   pn_directory -- output (with each call to dirfunc)
    ROOT
    pn_directory->selected_device set
    pn_directory->sn set appropriately
    pn_directory->selected_filetype not set

    DEVICE
    pn_directory->selected_device and pn_directory->sn still set
    pn_directory->selected_filetype loops through
*/
/* path is the path which "pn_directory" parses */
/* FS_dir produces the "invariant" portion of the directory, passing on to
   FS_dir_seek the variable part */
int FS_dir(void (*dirfunc) (void *, const struct parsedname *),
		   void *v, const struct parsedname *pn_directory)
{
	uint32_t flags;
	LEVEL_DEBUG("In FS_dir(%s)\n",pn_directory->path) ;

	return FS_dir_both(dirfunc, v, pn_directory, &flags);
}

/* path is the path which "pn_directory" parses */
/* FS_dir_remote is the entry into FS_dir_seek from ServerDir */
/* More checking is done, and the flags are returned */
int FS_dir_remote(void (*dirfunc) (void *, const struct parsedname *),
				  void *v, const struct parsedname *pn_directory, uint32_t * flags)
{
	LEVEL_DEBUG("In FS_dir_remote(%s)\n",pn_directory->path) ;
	return FS_dir_both(dirfunc, v, pn_directory, flags);
}


static int FS_dir_both(void (*dirfunc) (void *, const struct parsedname *),
					   void *v, const struct parsedname *pn_raw_directory,
					   uint32_t * flags)
{
	int ret = 0;
	struct parsedname pn2;

	/* initialize flags */
	flags[0] = 0;
	if (pn_raw_directory == NULL || pn_raw_directory->in == NULL)
		return -ENODEV;
	LEVEL_CALL("DIRECTORY path=%s\n", SAFESTRING(pn_raw_directory->path));

	STATLOCK;
	AVERAGE_IN(&dir_avg);
	AVERAGE_IN(&all_avg);
	STATUNLOCK;
	FSTATLOCK;
	dir_time = time(NULL);		// protected by mutex
	FSTATUNLOCK;

	/* Make a copy (shallow) of pn_raw_directory to modify for directory entries */
	memcpy(&pn2, pn_raw_directory, sizeof(struct parsedname));	/*shallow copy */

	if (pn_raw_directory->selected_filetype) {
		ret = -ENOTDIR;
	} else if (pn_raw_directory->selected_device) {		/* device directory */
		if (pn_raw_directory->type == pn_structure) {
			/* Device structure is always known for ordinary devices, so don't
			 * bother calling ServerDir() */
			ret = FS_devdir(dirfunc, v, &pn2);
		} else if (SpecifiedBus(pn_raw_directory) && BusIsServer(pn_raw_directory->in)) {
			ret = ServerDir(dirfunc, v, &pn2, flags);
		} else {
			ret = FS_devdir(dirfunc, v, &pn2);
		}
	} else if (IsAlarmDir(pn_raw_directory)) {	/* root or branch directory -- alarm state */
		LEVEL_DEBUG("ALARM directory\n");
		ret = SpecifiedBus(pn_raw_directory) ? FS_alarmdir(dirfunc, v, &pn2)
			: FS_dir_seek(dirfunc, v, pn2.in, &pn2, flags);
		LEVEL_DEBUG("Return from ALARM is %d\n",ret) ;
	} else if (NotRealDir(pn_raw_directory)) {	/* stat, sys or set dir */
		/* there are some files with variable sizes, and /system/adapter have variable
		 * number of entries and we have to call ServerDir() */
		ret = (SpecifiedBus(pn_raw_directory) && BusIsServer(pn_raw_directory->in))
			? ServerDir(dirfunc, v, &pn2, flags)
			: FS_typedir(dirfunc, v, &pn2);
	} else {					/* Directory of some kind */
		if (RootNotBranch(pn_raw_directory)) {	/* root directory */
            FS_structure_dir(dirfunc,v,&pn2) ;
			if (!SpecifiedBus(pn_raw_directory) && ShouldReturnBusList(pn_raw_directory)) {
				/* restore state */
				pn2.type = pn_real;
				FS_busdir(dirfunc, v, pn_raw_directory);
				if (NotUncachedDir(pn_raw_directory)) {
					FS_uncached_dir(dirfunc,v,&pn2) ;
					FS_stype_dir(dirfunc,v,&pn2) ;
				}
				pn2.type = pn_raw_directory->type;
			}
		}
		/* Now get the actual devices */
		ret = SpecifiedBus(pn_raw_directory) ? FS_cache2real(dirfunc, v, &pn2, flags)
			: FS_dir_seek(dirfunc, v, pn2.in, &pn2, flags);
	}
	if (Global.opt != opt_server) {
		if (NotAlarmDir(pn_raw_directory)) {
			/* don't show alarm directory in alarm directory */
			/* alarm directory */
			if (flags[0] & DEV_alarm) {
				pn2.state = (pn_alarm | (pn_raw_directory->state & pn_text));
				dirfunc(v, &pn2);
				pn2.state = pn_raw_directory->state;
			}
		}
		/* simultaneous directory */
		if (flags[0] & (DEV_temp | DEV_volt)) {
			pn2.selected_device = DeviceSimultaneous;
			dirfunc(v, &pn2);
		}
	}
	STATLOCK;
	AVERAGE_OUT(&dir_avg);
	AVERAGE_OUT(&all_avg);
	STATUNLOCK;
	LEVEL_DEBUG("FS_dir_both out ret=%d\n", ret);
	return ret;
}

/* status settings sustem */
static void FS_stype_dir(void (*dirfunc) (void *, const struct parsedname *),
                      void *v, struct parsedname *pn_root_directory)
{
    struct parsedname s_pn_s_directory ;
    struct parsedname * pn_s_directory = &s_pn_s_directory ;

    /* Shallow copy */
    memcpy( pn_s_directory, pn_root_directory, sizeof(struct parsedname)) ;

    pn_s_directory->type = pn_settings;
    dirfunc(v, pn_s_directory);
    pn_s_directory->type = pn_system;
    dirfunc(v, pn_s_directory);
    pn_s_directory->type = pn_statistics;
    dirfunc(v, pn_s_directory);
}

/* status settings sustem */
static void FS_uncached_dir(void (*dirfunc) (void *, const struct parsedname *),
                      void *v, struct parsedname *pn_root_directory)
{
    struct parsedname s_pn_uncached_directory ;
    struct parsedname * pn_uncached_directory = &s_pn_uncached_directory ;

	if ( ! IsLocalCacheEnabled(pn_root_directory)) {	/* cached */
		return ;
	}

	/* Shallow copy */
	memcpy( pn_uncached_directory, pn_root_directory, sizeof(struct parsedname)) ;

	pn_uncached_directory->state = pn_root_directory->state | pn_uncached;
	dirfunc(v, pn_uncached_directory);
}
/* status settings sustem */
static void FS_structure_dir(void (*dirfunc) (void *, const struct parsedname *),
                      void *v, struct parsedname *pn_root_directory)
{
    struct parsedname s_pn_structure_directory ;
    struct parsedname * pn_structure_directory = &s_pn_structure_directory ;

    /* Shallow copy */
    memcpy( pn_structure_directory, pn_root_directory, sizeof(struct parsedname)) ;

    if (Global.opt == opt_server ) return ;
    if ( SpecifiedBus(pn_root_directory) ) return ;
    if ( IsUncachedDir(pn_root_directory) ) return ;

    pn_structure_directory->type = pn_structure;
    dirfunc(v, pn_structure_directory);
}

/* path is the path which "pn_directory" parses */
/* FS_dir_seek produces the data that can vary: device lists, etc. */
#if OW_MT
struct dir_seek_struct {
	struct connection_in *in;
	const struct parsedname *pn_directory;
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
		FS_dir_seek(dss->dirfunc, dss->v, dss->in, dss->pn_directory, dss->flags);
	pthread_exit(NULL);
	return NULL;
}
static int FS_dir_seek(void (*dirfunc) (void *, const struct parsedname *),
					   void *v, struct connection_in *in,
					   const struct parsedname *pn_directory, uint32_t * flags)
{
	int ret = 0;
	struct dir_seek_struct dss = { in->next, pn_directory, dirfunc, v, flags, 0 };
	struct parsedname pn2;
	pthread_t thread;
	int threadbad = 1;

	if (!KnownBus(pn_directory)) {
		threadbad = in->next == NULL
			|| pthread_create(&thread, NULL, FS_dir_seek_callback,
							  (void *) (&dss));
	}

	memcpy(&pn2, pn_directory, sizeof(struct parsedname));	// shallow copy
	pn2.in = in;

	if (TestConnection(&pn2)) {	// reconnect ok?
		ret = -ECONNABORTED;
	} else if (KnownBus(pn_directory) && BusIsServer(in)) {	/* is this a remote bus? */
		//printf("FS_dir_seek: Call ServerDir %s\n", pn_directory->path);
		ret = ServerDir(dirfunc, v, &pn2, flags);
	} else {					/* local bus */
		if (IsAlarmDir(pn_directory)) {	/* root or branch directory -- alarm state */
			//printf("FS_dir_seek: Call FS_alarmdir %s\n", pn_directory->path);
			ret = FS_alarmdir(dirfunc, v, &pn2);
		} else {
			//printf("FS_dir_seek: call FS_cache2real bus %d\n", pn_directory->in->index);
			ret = FS_cache2real(dirfunc, v, &pn2, flags);
		}
	}
	//printf("FS_dir_seek4 pid=%ld adapter=%d ret=%d\n",pthread_self(), pn_directory->in->index,ret);
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

/* path is the path which "pn_directory" parses */
/* FS_dir_seek produces the data that can vary: device lists, etc. */
static int FS_dir_seek(void (*dirfunc) (void *, const struct parsedname *),
					   void *v, struct connection_in *in,
					   const struct parsedname *pn_directory, uint32_t * flags)
{
	int ret = 0;
	struct parsedname pn2;

	memcpy(&pn2, pn_directory, sizeof(struct parsedname));	//shallow copy
	pn2.in = in;
	if (TestConnection(&pn2)) {	// reconnect ok?
		ret = -ECONNABORTED;
	} else if (KnownBus(pn_directory) && BusIsServer(in)) {	/* is this a remote bus? */
		//printf("FS_dir_seek: Call ServerDir %s\n", pn_directory->path);
		ret = ServerDir(dirfunc, v, &pn2, flags);
	} else {					/* local bus */
		if (IsAlarmDir(&pn2)) {	/* root or branch directory -- alarm state */
			//printf("FS_dir_seek: Call FS_alarmdir %s\n", pn_directory->path);
			ret = FS_alarmdir(dirfunc, v, &pn2);
		} else {
			//printf("FS_dir_seek: call FS_cache2real bus %d\n", pn_directory->in->index);
			ret = FS_cache2real(dirfunc, v, &pn2, flags);
		}
	}
	//printf("FS_dir_seek4 pid=%ld adapter=%d ret=%d\n",pthread_self(), pn_directory->in->index,ret);
	if (in->next && ret <= 0)
		return FS_dir_seek(dirfunc, v, in->next, pn_directory, flags);
	return ret;
}
#endif							/* OW_MT */

/* Device directory -- all from memory */
static int FS_devdir(void (*dirfunc) (void *, const struct parsedname *),
					 void *v, struct parsedname *pn_device_directory)
{
	struct filetype *lastft = &(pn_device_directory->selected_device->filetype_array[pn_device_directory->selected_device->count_of_filetypes]);	/* last filetype struct */
	struct filetype *firstft;	/* first filetype struct */
	char s[33];
	size_t len;

	struct parsedname s_pn_file_entry ;
	struct parsedname * pn_file_entry = &s_pn_file_entry ;

	STAT_ADD1(dir_dev.calls);
	if (pn_device_directory->subdir != NULL) {			/* head_inbound_list subdir, name prepends */
		//printf("DIR device subdirectory\n");
		len = snprintf(s, 32, "%s/", pn_device_directory->subdir->name);
		firstft = pn_device_directory->subdir + 1;
	} else {
		//printf("DIR device directory\n");
		len = 0;
		firstft = pn_device_directory->selected_device->filetype_array;
	}

	/* Shallow copy */
	memcpy( pn_file_entry, pn_device_directory, sizeof(struct parsedname)) ;

	for (pn_file_entry->selected_filetype = firstft; pn_file_entry->selected_filetype < lastft; ++pn_file_entry->selected_filetype) {	/* loop through filetypes */
		if (len) {				/* subdir */
			/* test start of name for directory name */
			if (strncmp(pn_file_entry->selected_filetype->name, s, len))
				break;
		} else {				/* primary device directory */
			if (strchr(pn_file_entry->selected_filetype->name, '/'))
				continue;
		}
		if (pn_file_entry->selected_filetype->ag) {
			for (pn_file_entry->extension =
				 (pn_file_entry->selected_filetype->format == ft_bitfield) ? -2 : -1;
				 pn_file_entry->extension < pn_file_entry->selected_filetype->ag->elements;
				 ++pn_file_entry->extension) {
				dirfunc(v, pn_file_entry);
				STAT_ADD1(dir_dev.entries);
			}
		} else {
			pn_file_entry->extension = 0;
			dirfunc(v, pn_file_entry);
			STAT_ADD1(dir_dev.entries);
		}
	}
	return 0;
}

/* Note -- alarm directory is smaller, no adapters or stats or uncached */
static int FS_alarmdir(void (*dirfunc) (void *, const struct parsedname *),
					   void *v, struct parsedname *pn_alarm_directory)
{
	int ret;
	uint32_t flags;
	struct device_search ds;	// holds search state
	struct parsedname s_pn_alarm_device ;
	struct parsedname * pn_alarm_device = &s_pn_alarm_device ;

	/* cache from Server if this is a remote bus */
	if (BusIsServer(pn_alarm_directory->in))
		return ServerDir(dirfunc, v, pn_alarm_directory, &flags);

	/* Shallow copy */
	memcpy( pn_alarm_device, pn_alarm_directory, sizeof(struct parsedname)) ;

	/* STATISCTICS */
	STAT_ADD1(dir_main.calls);
//printf("DIR alarm directory\n");

	BUSLOCK(pn_alarm_directory);
	pn_alarm_directory->selected_filetype = NULL;				/* just in case not properly set */
	ret = BUS_first_alarm(&ds, pn_alarm_directory);
	if (ret) {
		BUSUNLOCK(pn_alarm_directory);
		LEVEL_DEBUG("FS_alarmdir BUS_first_alarm = %d\n",ret) ;
		if (ret == -ENODEV)
			return 0;			/* no more alarms is ok */
		return ret;
	}
	while (ret == 0) {
		STAT_ADD1(dir_main.entries);
		memcpy(pn_alarm_device->sn, ds.sn, 8);
		/* Search for known 1-wire device -- keyed to device name (family code in HEX) */
		FS_devicefindhex(ds.sn[0], pn_alarm_device);	// lookup ID and set pn2.selected_device
		DIRLOCK;
		dirfunc(v, pn_alarm_device);
		DIRUNLOCK;
		ret = BUS_next(&ds, pn_alarm_directory);
//printf("ALARM sn: "SNformat" ret=%d\n",SNvar(sn),ret);
	}
	BUSUNLOCK(pn_alarm_directory);
	if (ret == -ENODEV)
		return 0;				/* no more alarms is ok */
	return ret;
}

/* A directory of devices -- either main or branch */
/* not within a device, nor alarm state */
/* Also, adapters and stats handled elsewhere */
/* Scan the directory from the BUS and add to cache */
static int FS_realdir(void (*dirfunc) (void *, const struct parsedname *),
					  void *v, struct parsedname *pn_whole_directory, uint32_t * flags)
{
	struct device_search ds;
	size_t devices = 0;
	struct dirblob db;
	struct parsedname s_pn_real_device ;
	struct parsedname * pn_real_device = &s_pn_real_device ;
	int ret;

	/* cache from Server if this is a remote bus */
	if (BusIsServer(pn_whole_directory->in))
		return ServerDir(dirfunc, v, pn_whole_directory, flags);

	/* Shallow copy */
	memcpy( pn_real_device, pn_whole_directory, sizeof(struct parsedname)) ;

	/* STATISTICS */
	STAT_ADD1(dir_main.calls);

	DirblobInit(&db);			// set up a fresh dirblob

	/* Operate at dev level, not filetype */
	pn_real_device->selected_filetype = NULL;
	flags[0] = 0;				/* start out with no flags set */

	BUSLOCK(pn_real_device);

	/* it appears that plugging in a new device sends a "presence pulse" that screws up BUS_first */
	if ((ret = BUS_first(&ds, pn_whole_directory))) {
		BUSUNLOCK(pn_whole_directory);
		if (ret == -ENODEV) {
			if (RootNotBranch(pn_real_device))
				pn_real_device->in->last_root_devs = 0;	// root dir estimated length
			return 0;			/* no more devices is ok */
		}
		return -EIO;
	}
	/* BUS still locked */
	if (RootNotBranch(pn_real_device))
		db.allocated = pn_real_device->in->last_root_devs;	// root dir estimated length
	do {
		BUSUNLOCK(pn_whole_directory);
		if (DirblobPure(&db)) {	/* only add if there is a blob allocated successfully */
			DirblobAdd(ds.sn, &db);
		}
		++devices;

		memcpy(pn_real_device->sn, ds.sn, 8);
		/* Add to Device location cache */
		Cache_Add_Device(pn_real_device->in->index, pn_real_device);
		/* Search for known 1-wire device -- keyed to device name (family code in HEX) */
		FS_devicefindhex(ds.sn[0], pn_real_device);	// lookup ID and set pn_real_device.selected_device

		DIRLOCK;
		dirfunc(v, pn_real_device);
		flags[0] |= pn_real_device->selected_device->flags;
		DIRUNLOCK;

		BUSLOCK(pn_whole_directory);
	} while ((ret = BUS_next(&ds, pn_whole_directory)) == 0);
	/* BUS still locked */
	if (RootNotBranch(pn_real_device) && ret == -ENODEV)
		pn_real_device->in->last_root_devs = devices;	// root dir estimated length
	BUSUNLOCK(pn_whole_directory);

	/* Add to the cache (full list as a single element */
	if (DirblobPure(&db) && ret == -ENODEV) {
		Cache_Add_Dir(&db, pn_real_device);	// end with a null entry
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
   -- DS2409/main|aux for branch
   -- DS2409 needs only the last element since each DS2409 is unique
   */
void FS_LoadPath(BYTE * sn, const struct parsedname *pn_branch_directory)
{
	if (RootNotBranch(pn_branch_directory)) {
		memset(sn, 0, 8);
	} else {
		memcpy(sn, pn_branch_directory->bp[pn_branch_directory->pathlength - 1].sn, 7);
		sn[7] = pn_branch_directory->bp[pn_branch_directory->pathlength - 1].branch;
	}
}

/* A directory of devices -- either main or branch */
/* not within a device, nor alarm state */
/* Also, adapters and stats handled elsewhere */
/* Cache2Real try the cache first, else can directory from bus (and add to cache) */
static int FS_cache2real(void (*dirfunc) (void *, const struct parsedname *), void *v,
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
		FS_devicefindhex(pn2->sn[0], pn2);	// lookup ID and set pn2.selected_device
		DIRLOCK;
		dirfunc(v, pn2);
		flags[0] |= pn2->selected_device->flags;
		DIRUNLOCK;
	}
	DirblobClear(&db);			/* allocated in Cache_Get_Dir */
	pn2->selected_device = NULL;			/* clear for the rest of directory listing */
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
	struct parsedname *pn_directory;
} typedir_action_struct;
static void Typediraction(const void *t, const VISIT which,
						  const int depth)
{
	(void) depth;
	switch (which) {
	case leaf:
	case postorder:
		typedir_action_struct.pn_directory->selected_device = ((const struct device_opaque *) t)->key;
		(typedir_action_struct.dirfunc) (typedir_action_struct.v, typedir_action_struct.pn_directory);
	default:
		break;
	}
}

/* Show the pn_directory->type (statistics, system, ...) entries */
/* Only the top levels, the rest will be shown by FS_devdir */
static int FS_typedir(void (*dirfunc) (void *, const struct parsedname *),
					  void *v, struct parsedname *pn_type_directory)
{
    TYPEDIRMUTEXLOCK ;

    typedir_action_struct.dirfunc = dirfunc;
	typedir_action_struct.v = v;
	typedir_action_struct.pn_directory = pn_type_directory;
	twalk(Tree[pn_type_directory->type], Typediraction);

    TYPEDIRMUTEXUNLOCK ;

    return 0;
}

/* Show the bus entries */
static int FS_busdir(void (*dirfunc) (void *, const struct parsedname *),
					 void *v, const struct parsedname *pn_directory)
{
	struct parsedname pn_bus_directory;
	int bus_number;

	memcpy(&pn_bus_directory, pn_directory, sizeof(struct parsedname));	// shallow copy
	pn_bus_directory.state = pn_bus ; // even stronger statement than SetKnownBus

	for (bus_number = 0; bus_number < count_inbound_connections; ++bus_number) {
		SetKnownBus(bus_number, &pn_bus_directory);
		//printf("Called FS_busdir on %s bus number %d\n",pn_directory->path,bus_number) ;
		dirfunc(v, &pn_bus_directory);
	}

	return 0;
}
