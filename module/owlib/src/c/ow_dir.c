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
#include "ow.h"

static int FS_dir_both(void (*dirfunc) (void *, const struct parsedname *), void *v, const struct parsedname *pn_directory, uint32_t * flags);
static int
FS_dir_all_connections(void (*dirfunc) (void *, const struct parsedname *), void *v, const struct parsedname *pn_directory, uint32_t * flags);
static int FS_dir_all_connections_loop(void (*dirfunc)
									    (void *, const struct parsedname * const),
									   void *v, int bus_number, const struct parsedname *pn_directory, uint32_t * flags);
static int FS_devdir(void (*dirfunc) (void *, const struct parsedname * const), void *v, const struct parsedname *pn2);
static int FS_alarmdir(void (*dirfunc) (void *, const struct parsedname * const), void *v, const struct parsedname *pn2);
static int FS_typedir(void (*dirfunc) (void *, const struct parsedname * const), void *v, const struct parsedname *pn_type_directory);
static int FS_realdir(void (*dirfunc) (void *, const struct parsedname * const), void *v, const struct parsedname *pn2, uint32_t * flags);
static int FS_cache2real(void (*dirfunc) (void *, const struct parsedname * const), void *v, const struct parsedname *pn2, uint32_t * flags);
static int FS_busdir(void (*dirfunc) (void *, const struct parsedname *), void *v, const struct parsedname *pn_directory);

static void FS_stype_dir(void (*dirfunc) (void *, const struct parsedname *), void *v, const struct parsedname *pn_root_directory);
static void FS_interface_dir(void (*dirfunc) (void *, const struct parsedname *), void *v, const struct parsedname *pn_root_directory);
static void FS_alarm_entry(void (*dirfunc) (void *, const struct parsedname *), void *v, const struct parsedname *pn_root_directory);
static void FS_simultaneous_entry(void (*dirfunc) (void *, const struct parsedname *), void *v, const struct parsedname *pn_root_directory);
static void FS_uncached_dir(void (*dirfunc) (void *, const struct parsedname *), void *v, const struct parsedname *pn_root_directory);
static int FS_dir_plus(void (*dirfunc) (void *, const struct parsedname *), void *v, const struct parsedname *pn_directory, const char *file);

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
   FS_dir_all_connections the variable part */
int FS_dir(void (*dirfunc) (void *, const struct parsedname *), void *v, const struct parsedname *pn_directory)
{
	uint32_t flags;
	LEVEL_DEBUG("In FS_dir(%s)\n", pn_directory->path);

	return FS_dir_both(dirfunc, v, pn_directory, &flags);
}

/* path is the path which "pn_directory" parses */
/* FS_dir_remote is the entry into FS_dir_all_connections from ServerDir */
/* More checking is done, and the flags are returned */
int FS_dir_remote(void (*dirfunc) (void *, const struct parsedname *), void *v, const struct parsedname *pn_directory, uint32_t * flags)
{
	LEVEL_DEBUG("In FS_dir_remote(%s)\n", pn_directory->path);
	return FS_dir_both(dirfunc, v, pn_directory, flags);
}


static int FS_dir_both(void (*dirfunc) (void *, const struct parsedname *), void *v, const struct parsedname *pn_raw_directory, uint32_t * flags)
{
	int ret = 0;

	/* initialize flags */
	flags[0] = 0;
	if (pn_raw_directory == NULL || pn_raw_directory->selected_connection == NULL)
		return -ENODEV;
	LEVEL_CALL("DIRECTORY path=%s\n", SAFESTRING(pn_raw_directory->path));

	STATLOCK;
	AVERAGE_IN(&dir_avg);
	AVERAGE_IN(&all_avg);
	STATUNLOCK;

	FSTATLOCK;
	StateInfo.dir_time = time(NULL);	// protected by mutex
	FSTATUNLOCK;

	if (pn_raw_directory->selected_filetype != NULL) {
		// File, not directory
		ret = -ENOTDIR;

	} else if (SpecifiedRemoteBus(pn_raw_directory)) {
		//printf("SPECIFIED_BUS BUS_IS_SERVER\n");
		if (!SpecifiedVeryRemoteBus(pn_raw_directory)) {
			//printf("Add extra INTERFACE\n");
			FS_interface_dir(dirfunc, v, pn_raw_directory);
		}
		// Send remotely only (all evaluation done there)
		ret = ServerDir(dirfunc, v, pn_raw_directory, flags);

	} else if (pn_raw_directory->selected_device != NULL) {
		//printf("NO SELECTED_DEVICE\n");
		// device directory -- not bus-specific
		ret = FS_devdir(dirfunc, v, pn_raw_directory);

	} else if (NotRealDir(pn_raw_directory)) {
		//printf("NOT_REAL_DIR\n");
		// structure, statistics, system or setrings dir -- not bus-specific
		ret = FS_typedir(dirfunc, v, pn_raw_directory);

	} else if (SpecifiedLocalBus(pn_raw_directory)) {
		if (IsAlarmDir(pn_raw_directory)) {	/* root or branch directory -- alarm state */
			LEVEL_DEBUG("ALARM directory\n");
			ret = FS_alarmdir(dirfunc, v, pn_raw_directory);
			LEVEL_DEBUG("Return from ALARM is %d\n", ret);
		} else {
			if (pn_raw_directory->pathlength == 0) {
				// only add funny directories for non-micro hub (DS2409) branches
				FS_interface_dir(dirfunc, v, pn_raw_directory);
			}
			/* Now get the actual devices */
			ret = FS_cache2real(dirfunc, v, pn_raw_directory, flags);
			/* simultaneous directory */
			if (flags[0] & (DEV_temp | DEV_volt)) {
				FS_simultaneous_entry(dirfunc, v, pn_raw_directory);
			}
			if (flags[0] & DEV_alarm) {
				FS_alarm_entry(dirfunc, v, pn_raw_directory);
			}
		}

	} else if (IsAlarmDir(pn_raw_directory)) {	/* root or branch directory -- alarm state */
		//printf("ALARM_DIR\n");
		LEVEL_DEBUG("ALARM directory\n");
		ret = FS_alarmdir(dirfunc, v, pn_raw_directory);
		LEVEL_DEBUG("Return from ALARM is %d\n", ret);

	} else {
		//printf("KNOWN_BUS\n");
		// Not specified bus, so scan through all and print union
		ret = FS_dir_all_connections(dirfunc, v, pn_raw_directory, flags);
		if ((Globals.opt != opt_server)
			|| ShouldReturnBusList(pn_raw_directory)) {
			if (pn_raw_directory->pathlength == 0) {
				// only add funny directories for non-micro hub (DS2409) branches
				FS_busdir(dirfunc, v, pn_raw_directory);
				FS_uncached_dir(dirfunc, v, pn_raw_directory);
				FS_stype_dir(dirfunc, v, pn_raw_directory);
			}
			/* simultaneous directory */
			if (flags[0] & (DEV_temp | DEV_volt)) {
				FS_simultaneous_entry(dirfunc, v, pn_raw_directory);
			}
			if (flags[0] & DEV_alarm) {
				FS_alarm_entry(dirfunc, v, pn_raw_directory);
			}
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
static void FS_stype_dir(void (*dirfunc) (void *, const struct parsedname *), void *v, const struct parsedname *pn_root_directory)
{
	FS_dir_plus(dirfunc, v, pn_root_directory, ePN_name[ePN_settings]);
	FS_dir_plus(dirfunc, v, pn_root_directory, ePN_name[ePN_system]);
	FS_dir_plus(dirfunc, v, pn_root_directory, ePN_name[ePN_statistics]);
	FS_dir_plus(dirfunc, v, pn_root_directory, ePN_name[ePN_structure]);
}

/* interface (only for bus directories) */
static void FS_interface_dir(void (*dirfunc) (void *, const struct parsedname *), void *v, const struct parsedname *pn_root_directory)
{
	FS_dir_plus(dirfunc, v, pn_root_directory, ePN_name[ePN_interface]);
}

static void FS_alarm_entry(void (*dirfunc) (void *, const struct parsedname *), void *v, const struct parsedname *pn_root_directory)
{
	FS_dir_plus(dirfunc, v, pn_root_directory, "alarm");
}

/* status settings sustem */
static void FS_uncached_dir(void (*dirfunc) (void *, const struct parsedname *), void *v, const struct parsedname *pn_root_directory)
{
	if (!IsLocalCacheEnabled(pn_root_directory)) {	/* cached */
		return;
	}

	if (IsUncachedDir(pn_root_directory)) {	/* cached */
		return;
	}

	FS_dir_plus(dirfunc, v, pn_root_directory, "uncached");
}

static void FS_simultaneous_entry(void (*dirfunc) (void *, const struct parsedname *), void *v, const struct parsedname *pn_root_directory)
{
	FS_dir_plus(dirfunc, v, pn_root_directory, "simultaneous");
}

/* path is the path which "pn_directory" parses */
/* FS_dir_all_connections produces the data that can vary: device lists, etc. */
#if OW_MT
struct dir_all_connections_struct {
	int bus_number;
	const struct parsedname *pn_directory;
	void (*dirfunc) (void *, const struct parsedname *);
	void *v;
	uint32_t *flags;
	int ret;
};

/* Embedded function */
static void *FS_dir_all_connections_callback(void *v)
{
	struct dir_all_connections_struct *dacs = v;
	dacs->ret = FS_dir_all_connections_loop(dacs->dirfunc, dacs->v, dacs->bus_number, dacs->pn_directory, dacs->flags);
	pthread_exit(NULL);
	return NULL;
}
static int FS_dir_all_connections_loop(void (*dirfunc)
									    (void *, const struct parsedname *), void *v,
									   int bus_number, const struct parsedname *pn_directory, uint32_t * flags)
{
	int ret = 0;
	struct dir_all_connections_struct dacs = { bus_number + 1, pn_directory, dirfunc, v, flags, 0 };
	struct parsedname s_pn_bus_directory;
	struct parsedname *pn_bus_directory = &s_pn_bus_directory;
	pthread_t thread;
	int threadbad = 1;

	threadbad = (bus_number + 1 >= count_inbound_connections)
		|| pthread_create(&thread, NULL, FS_dir_all_connections_callback, (void *) (&dacs));

	memcpy(pn_bus_directory, pn_directory, sizeof(struct parsedname));	// shallow copy

	SetKnownBus(bus_number, pn_bus_directory);

	if (TestConnection(pn_bus_directory)) {	// reconnect ok?
		ret = -ECONNABORTED;
	} else if (BusIsServer(pn_bus_directory->selected_connection)) {	/* is this a remote bus? */
		//printf("FS_dir_all_connections: Call ServerDir %s\n", pn_directory->path);
		ret = ServerDir(dirfunc, v, pn_bus_directory, flags);
	} else if (IsAlarmDir(pn_bus_directory)) {	/* root or branch directory -- alarm state */
		//printf("FS_dir_all_connections: Call FS_alarmdir %s\n", pn_directory->path);
		ret = FS_alarmdir(dirfunc, v, pn_bus_directory);
	} else {
		ret = FS_cache2real(dirfunc, v, pn_bus_directory, flags);
	}
	//printf("FS_dir_all_connections4 pid=%ld adapter=%d ret=%d\n",pthread_self(), pn_directory->selected_connection->index,ret);
	/* See if next bus was also queried */
	if (threadbad == 0) {		/* was a thread created? */
		void *vo;
		if (pthread_join(thread, &vo)) {
			return ret;			/* wait for it (or return only this result) */
		}
		if (dacs.ret >= 0) {
			return dacs.ret;	/* is it an error return? Then return this one */
		}
	}
	return ret;
}
static int
FS_dir_all_connections(void (*dirfunc) (void *, const struct parsedname *), void *v, const struct parsedname *pn_directory, uint32_t * flags)
{
	return FS_dir_all_connections_loop(dirfunc, v, 0, pn_directory, flags);
}
#else							/* OW_MT */

/* path is the path which "pn_directory" parses */
/* FS_dir_all_connections produces the data that can vary: device lists, etc. */
static int
FS_dir_all_connections(void (*dirfunc) (void *, const struct parsedname *), void *v, const struct parsedname *pn_directory, uint32_t * flags)
{
	int ret = 0;
	int bus_number;
	struct parsedname s_pn_selected_connection;
	struct parsedname *pn_selected_connection = &s_pn_selected_connection;

	memcpy(pn_selected_connection, pn_directory, sizeof(struct parsedname));	//shallow copy

#if 0
	memcpy(pn_bus_directory, pn_directory, sizeof(struct parsedname));	// shallow copy

	pn_bus_directory->state = ePS_bus;	// even stronger statement than SetKnownBus
	pn_bus_directory->type = ePN_real;
#endif
	
	for (bus_number = 0; bus_number < count_inbound_connections; ++bus_number) {
		SetKnownBus(bus_number, pn_selected_connection);

		if (TestConnection(pn_selected_connection)) {
			continue;
		}

		if (BusIsServer(pn_selected_connection->selected_connection)) {	/* is this a remote bus? */
			ret |= ServerDir(dirfunc, v, pn_selected_connection, flags);
		} else {				/* local bus */
			if (IsAlarmDir(pn_selected_connection)) {	/* root or branch directory -- alarm state */
				ret |= FS_alarmdir(dirfunc, v, pn_selected_connection);
			} else {
				ret = FS_cache2real(dirfunc, v, pn_selected_connection, flags);
			}
		}
	}

	return ret;
}
#endif							/* OW_MT */

/* Device directory -- all from memory */
static int FS_devdir(void (*dirfunc) (void *, const struct parsedname *), void *v, const struct parsedname *pn_device_directory)
{
	struct filetype *lastft = &(pn_device_directory->selected_device->filetype_array[pn_device_directory->selected_device->count_of_filetypes]);	/* last filetype struct */
	struct filetype *ft_pointer;	/* first filetype struct */
	char subdir_name[OW_FULLNAME_MAX + 1];
	size_t subdir_len;

	STAT_ADD1(dir_dev.calls);

	// Add subdir to name
	if (pn_device_directory->subdir != NULL) {	/* head_inbound_list subdir, name prepends */
		//printf("DIR device subdirectory\n");
		strncpy(subdir_name, pn_device_directory->subdir->name, OW_FULLNAME_MAX);
		strcat(subdir_name, "/");
		subdir_len = strlen(subdir_name);
		ft_pointer = pn_device_directory->subdir + 1;
	} else {
		//printf("DIR device directory\n");
		subdir_len = 0;
		ft_pointer = pn_device_directory->selected_device->filetype_array;
	}

	for (; ft_pointer < lastft; ++ft_pointer) {	/* loop through filetypes */
		char *slash = strchr(ft_pointer->name, '/');
		char *namepart;
		if (subdir_len > 0) {	/* subdir */
			/* test start of name for directory name */
			if (strncmp(ft_pointer->name, subdir_name, subdir_len) != 0) {
				break;
			}
		} else {				/* primary device directory */
			if (slash != NULL) {
				continue;
			}
		}
		namepart = (slash == NULL) ? ft_pointer->name : slash + 1;
		if (ft_pointer->ag) {
			int extension;
			int first_extension = (ft_pointer->format == ft_bitfield) ? -2 : -1;
			struct parsedname s_pn_file_entry;
			struct parsedname *pn_file_entry = &s_pn_file_entry;
			for (extension = first_extension; extension < ft_pointer->ag->elements; ++extension) {
				if (FS_ParsedNamePlusExt(pn_device_directory->path, namepart, extension, ft_pointer->ag->letters, pn_file_entry) == 0) {
					DIRLOCK;
					dirfunc(v, pn_file_entry);
					DIRUNLOCK;
					FS_ParsedName_destroy(pn_file_entry);
				}
				STAT_ADD1(dir_dev.entries);
			}
		} else {
			FS_dir_plus(dirfunc, v, pn_device_directory, namepart);
			STAT_ADD1(dir_dev.entries);
		}
	}
	return 0;
}

/* Note -- alarm directory is smaller, no adapters or stats or uncached */
static int FS_alarmdir(void (*dirfunc) (void *, const struct parsedname *), void *v, const struct parsedname *pn_alarm_directory)
{
	int ret;
	struct device_search ds;	// holds search state

	/* cache from Server if this is a remote bus */
	if (BusIsServer(pn_alarm_directory->selected_connection)) {
		uint32_t flags;
		return ServerDir(dirfunc, v, pn_alarm_directory, &flags);
	}

	/* STATISCTICS */
	STAT_ADD1(dir_main.calls);
	//printf("DIR alarm directory\n");

	BUSLOCK(pn_alarm_directory);
	ret = BUS_first_alarm(&ds, pn_alarm_directory);
	if (ret) {
		BUSUNLOCK(pn_alarm_directory);
		LEVEL_DEBUG("FS_alarmdir BUS_first_alarm = %d\n", ret);
		if (ret == -ENODEV) {
			return 0;			/* no more alarms is ok */
		}
		return ret;
	}
	while (ret == 0) {
		char dev[OW_FULLNAME_MAX + 1];

		STAT_ADD1(dir_main.entries);
		FS_devicename(dev, OW_FULLNAME_MAX, ds.sn, pn_alarm_directory);

		FS_dir_plus(dirfunc, v, pn_alarm_directory, dev);

		ret = BUS_next(&ds, pn_alarm_directory);
		//printf("ALARM sn: "SNformat" ret=%d\n",SNvar(sn),ret);
	}
	BUSUNLOCK(pn_alarm_directory);
	if (ret == -ENODEV) {
		return 0;				/* no more alarms is ok */
	}
	return ret;
}

/* A directory of devices -- either main or branch */
/* not within a device, nor alarm state */
/* Also, adapters and stats handled elsewhere */
/* Scan the directory from the BUS and add to cache */
static int FS_realdir(void (*dirfunc) (void *, const struct parsedname *), void *v, const struct parsedname *pn_whole_directory, uint32_t * flags)
{
	struct device_search ds;
	size_t devices = 0;
	struct dirblob db;
	int ret;

	/* cache from Server if this is a remote bus */
	if (BusIsServer(pn_whole_directory->selected_connection)) {
		return ServerDir(dirfunc, v, pn_whole_directory, flags);
	}

	/* STATISTICS */
	STAT_ADD1(dir_main.calls);

	DirblobInit(&db);			// set up a fresh dirblob

	flags[0] = 0;				/* start out with no flags set */

	BUSLOCK(pn_whole_directory);

	/* it appears that plugging in a new device sends a "presence pulse" that screws up BUS_first */
	if ((ret = BUS_first(&ds, pn_whole_directory))) {
		BUSUNLOCK(pn_whole_directory);
		if (ret == -ENODEV) {
			if (RootNotBranch(pn_whole_directory)) {
				pn_whole_directory->selected_connection->last_root_devs = 0;	// root dir estimated length
			}
			return 0;			/* no more devices is ok */
		}
		return -EIO;
	}
	/* BUS still locked */
	if (RootNotBranch(pn_whole_directory)) {
		db.allocated = pn_whole_directory->selected_connection->last_root_devs;	// root dir estimated length
	}
	do {
		struct parsedname s_pn_real_device;
		struct parsedname *pn_real_device = &s_pn_real_device;
		char dev[OW_FULLNAME_MAX + 1];

		BUSUNLOCK(pn_whole_directory);

		FS_devicename(dev, OW_FULLNAME_MAX, ds.sn, pn_whole_directory);
		if (DirblobPure(&db)) {	/* only add if there is a blob allocated successfully */
			DirblobAdd(ds.sn, &db);
		}
		++devices;

		if (FS_ParsedNamePlus(pn_whole_directory->path, dev, pn_real_device) == 0) {
			DIRLOCK;
			dirfunc(v, pn_real_device);
			flags[0] |= pn_real_device->selected_device->flags;
			DIRUNLOCK;
			FS_ParsedName_destroy(pn_real_device);
		}

		BUSLOCK(pn_whole_directory);
	} while ((ret = BUS_next(&ds, pn_whole_directory)) == 0);
	/* BUS still locked */
	if (RootNotBranch(pn_whole_directory) && ret == -ENODEV) {
		pn_whole_directory->selected_connection->last_root_devs = devices;	// root dir estimated length
	}
	BUSUNLOCK(pn_whole_directory);

	/* Add to the cache (full list as a single element */
	if (DirblobPure(&db) && ret == -ENODEV) {
		Cache_Add_Dir(&db, pn_whole_directory);
	}
	DirblobClear(&db);

	STATLOCK;
	dir_main.entries += devices;
	STATUNLOCK;
	if (ret == -ENODEV) {
		return 0;				// no more devices is ok */
	}
	return ret;
}

/* points "serial number" to directory
   -- 0 for root
   -- DS2409/main|aux for branch
   -- DS2409 needs only the last element since each DS2409 is unique
   */
void FS_LoadDirectoryOnly(struct parsedname *pn_directory, const struct parsedname *pn_original)
{
	if (pn_directory != pn_original) {
		memcpy(pn_directory, pn_original, sizeof(struct parsedname));	//shallow copy
	}
	if (RootNotBranch(pn_directory)) {
		memset(pn_directory->sn, 0, 8);
	} else {
		--pn_directory->pathlength;
		memcpy(pn_directory->sn, pn_directory->bp[pn_directory->pathlength].sn, 7);
		pn_directory->sn[7] = pn_directory->bp[pn_directory->pathlength].branch;
	}
	pn_directory->selected_device = NULL;
}

/* A directory of devices -- either main or branch */
/* not within a device, nor alarm state */
/* Also, adapters and stats handled elsewhere */
/* Cache2Real try the cache first, else can directory from bus (and add to cache) */
static int FS_cache2real(void (*dirfunc) (void *, const struct parsedname *), void *v, const struct parsedname *pn_real_directory, uint32_t * flags)
{
	size_t dindex;
	struct dirblob db;
	BYTE sn[8];

	/* Test to see whether we should get the directory "directly" */
	//printf("Pre test cache for dir\n") ;
	if (SpecifiedBus(pn_real_directory) || IsUncachedDir(pn_real_directory)
		|| Cache_Get_Dir(&db, pn_real_directory)) {
		//printf("FS_cache2real: didn't find anything at bus %d\n", pn_real_directory->selected_connection->index);
		return FS_realdir(dirfunc, v, pn_real_directory, flags);
	}
	//printf("Post test cache for dir, snlist=%p, devices=%lu\n",snlist,devices) ;
	/* We have a cached list in snlist. Note that we have to free this memory */

	/* STATISCTICS */
	STAT_ADD1(dir_main.calls);

	/* Get directory from the cache */
	for (dindex = 0; DirblobGet(dindex, sn, &db) == 0; ++dindex) {
		struct parsedname s_pn_real_device;
		struct parsedname *pn_real_device = &s_pn_real_device;
		char dev[OW_FULLNAME_MAX + 1];

		FS_devicename(dev, OW_FULLNAME_MAX, sn, pn_real_directory);
		if (FS_ParsedNamePlus(pn_real_directory->path, dev, pn_real_device) == 0) {
			DIRLOCK;
			dirfunc(v, pn_real_device);
			flags[0] |= pn_real_device->selected_device->flags;
			DIRUNLOCK;
			FS_ParsedName_destroy(pn_real_device);
		}
	}
	DirblobClear(&db);			/* allocated in Cache_Get_Dir */

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
#else							/* OW_MT */
#define TYPEDIRMUTEXLOCK
#define TYPEDIRMUTEXUNLOCK
#endif							/* OW_MT */

// struct for walking through tree -- cannot send data except globally
struct {
	void (*dirfunc) (void *, const struct parsedname *);
	void *v;
	struct parsedname *pn_directory;
} typedir_action_struct;

static void Typediraction(const void *t, const VISIT which, const int depth)
{
	(void) depth;
	switch (which) {
	case leaf:
	case postorder:
		FS_dir_plus(typedir_action_struct.dirfunc, typedir_action_struct.v, typedir_action_struct.pn_directory,
					((const struct device_opaque *) t)->key->family_code);
	default:
		break;
	}
}

/* Show the pn_directory->type (statistics, system, ...) entries */
/* Only the top levels, the rest will be shown by FS_devdir */
static int FS_typedir(void (*dirfunc) (void *, const struct parsedname *), void *v, const struct parsedname *pn_type_directory)
{
	struct parsedname s_pn_type_device;
	struct parsedname *pn_type_device = &s_pn_type_device;


	memcpy(pn_type_device, pn_type_directory, sizeof(struct parsedname));	// shallow copy

	LEVEL_DEBUG("FS_typedir called on %s\n", pn_type_directory->path);

	TYPEDIRMUTEXLOCK;

	typedir_action_struct.dirfunc = dirfunc;
	typedir_action_struct.v = v;
	typedir_action_struct.pn_directory = pn_type_device;
	twalk(Tree[pn_type_directory->type], Typediraction);

	TYPEDIRMUTEXUNLOCK;

	return 0;
}

/* Show the bus entries */
static int FS_busdir(void (*dirfunc) (void *, const struct parsedname *), void *v, const struct parsedname *pn_directory)
{
	char bus[OW_FULLNAME_MAX];
	int bus_number;

	if (!RootNotBranch(pn_directory)) {
		return 0;
	}

	for (bus_number = 0; bus_number < count_inbound_connections; ++bus_number) {
		UCLIBCLOCK;
		snprintf(bus, OW_FULLNAME_MAX, "bus.%d", bus_number);
		UCLIBCUNLOCK;
		FS_dir_plus(dirfunc, v, pn_directory, bus);
	}

	return 0;
}

/* Parse and show */
static int FS_dir_plus(void (*dirfunc) (void *, const struct parsedname *), void *v, const struct parsedname *pn_directory, const char *file)
{
	struct parsedname s_pn_plus_directory;
	struct parsedname *pn_plus_directory = &s_pn_plus_directory;

	if (FS_ParsedNamePlus(pn_directory->path, file, pn_plus_directory) == 0) {
		DIRLOCK;
		dirfunc(v, pn_plus_directory);
		DIRUNLOCK;
		FS_ParsedName_destroy(pn_plus_directory);
		return 0;
	}
	return -ENOENT;
}
