/*
    OWFS -- One-Wire filesystem
    OWHTTPD -- One-Wire Web Server
    Written 2003 Paul H Alfille
    email: paul.alfille@gmail.com
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
#include "ow_external.h"

static enum search_status PossiblyLockedBusCall( enum search_status (* first_next)(struct device_search *, const struct parsedname *), struct device_search * ds, const struct parsedname * pn ) ;

static ZERO_OR_ERROR FS_dir_both(void (*dirfunc) (void *, const struct parsedname *), void *v, const struct parsedname *pn_directory, uint32_t * flags);
static ZERO_OR_ERROR FS_dir_all_connections(void (*dirfunc) (void *, const struct parsedname *), void *v, const struct parsedname *pn_directory, uint32_t * flags);
static ZERO_OR_ERROR FS_devdir(void (*dirfunc) (void *, const struct parsedname * const), void *v, const struct parsedname *pn2);
static ZERO_OR_ERROR FS_structdevdir(void (*dirfunc) (void *, const struct parsedname *), void *v, const struct parsedname *pn_device_directory);
static ZERO_OR_ERROR FS_alarmdir(void (*dirfunc) (void *, const struct parsedname * const), void *v, const struct parsedname *pn2);
static ZERO_OR_ERROR FS_typedir(void (*dirfunc) (void *, const struct parsedname * const), void *v, const struct parsedname *pn_type_directory);
static ZERO_OR_ERROR FS_realdir(void (*dirfunc) (void *, const struct parsedname * const), void *v, const struct parsedname *pn2, uint32_t * flags);
static ZERO_OR_ERROR FS_cache_or_real(void (*dirfunc) (void *, const struct parsedname * const), void *v, const struct parsedname *pn2, uint32_t * flags);
static ZERO_OR_ERROR FS_busdir(void (*dirfunc) (void *, const struct parsedname *), void *v, const struct parsedname *pn_directory);

static void FS_stype_dir(void (*dirfunc) (void *, const struct parsedname *), void *v, const struct parsedname *pn_root_directory);
static ZERO_OR_ERROR FS_externaldir(void (*dirfunc) (void *, const struct parsedname *), void *v, const struct parsedname *pn_external_directory);
static void FS_interface_dir(void (*dirfunc) (void *, const struct parsedname *), void *v, const struct parsedname *pn_root_directory);
static void FS_alarm_entry(void (*dirfunc) (void *, const struct parsedname *), void *v, const struct parsedname *pn_root_directory);
static void FS_simultaneous_entry(void (*dirfunc) (void *, const struct parsedname *), void *v, const struct parsedname *pn_root_directory);
static void FS_uncached_dir(void (*dirfunc) (void *, const struct parsedname *), void *v, const struct parsedname *pn_root_directory);
static ZERO_OR_ERROR FS_dir_plus(void (*dirfunc) (void *, const struct parsedname *), void *v, uint32_t * flags, const struct parsedname *pn_directory, const char *file) ;

/* Calls dirfunc() for each element in directory */
/* void * data is arbitrary user data passed along -- e.g. output file descriptor */
/* pn_directory -- input:
    pn_directory->selected_device == NO_DEVICE -- root directory, give list of all devices
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

/* FS_dir produces the "invariant" portion of the directory, passing on to
   FS_dir_all_connections the variable part */
ZERO_OR_ERROR FS_dir(void (*dirfunc) (void *, const struct parsedname *), void *v, struct parsedname *pn_directory)
{
	/* applies 'dirfunc' to each directory element of pn_directory */
	/* void * v is extra information passed along */
	
	uint32_t flags;
	LEVEL_DEBUG("path=%s", pn_directory->path);
	pn_directory->control_flags |= ALIAS_REQUEST ; // All local directory queries want alias translation

	return FS_dir_both(dirfunc, v, pn_directory, &flags);
}

/* path is the path which "pn_directory" parses */
/* FS_dir_remote is the entry into FS_dir_all_connections from ServerDir */
/* More checking is done, and the flags are returned */
ZERO_OR_ERROR FS_dir_remote(void (*dirfunc) (void *, const struct parsedname *), void *v, const struct parsedname *pn_directory, uint32_t * flags)
{
	LEVEL_DEBUG("path=%s", pn_directory->path);
	return FS_dir_both(dirfunc, v, pn_directory, flags);
}


static ZERO_OR_ERROR FS_dir_both(void (*dirfunc) (void *, const struct parsedname *), void *v, const struct parsedname *pn_raw_directory, uint32_t * flags)
{
	ZERO_OR_ERROR ret = 0;

	/* initialize flags */
	flags[0] = 0;

	/* pn_raw_directory->selected_connection Could be NULL here...
	 * It will then return a root-directory containing
	 * /uncached,/settings,/system,/statistics,/structure
	 * instead of an empty directory.
	 */
	if (pn_raw_directory == NO_PARSEDNAME) {
		LEVEL_CALL("return ENODEV pn_raw_directory=%p selected_connection=%p",
			pn_raw_directory,
			(pn_raw_directory ? pn_raw_directory->selected_connection : NO_CONNECTION));
		return -ENODEV;
	}
	
	LEVEL_CALL("path=%s", SAFESTRING(pn_raw_directory->path));

	STATLOCK;
	AVERAGE_IN(&dir_avg);
	AVERAGE_IN(&all_avg);
	STATUNLOCK;

	FSTATLOCK;
	StateInfo.dir_time = NOW_TIME;	// protected by mutex
	FSTATUNLOCK;

	if (pn_raw_directory->selected_filetype != NO_FILETYPE) {
		// File, not directory
		ret = -ENOTDIR;

	} else if (SpecifiedVeryRemoteBus(pn_raw_directory)) {
		//printf("SPECIFIED_BUS BUS_IS_SERVER (very remote)\n");
		// Send remotely only (all evaluation done there)
		ret = ServerDir(dirfunc, v, pn_raw_directory, flags);

	} else if (SpecifiedRemoteBus(pn_raw_directory)) {
		//printf("SPECIFIED_BUS BUS_IS_SERVER (just remote)\n");
		//printf("Add extra INTERFACE\n");
		FS_interface_dir(dirfunc, v, pn_raw_directory);
		// Send remotely only (all evaluation done there)
		ret = ServerDir(dirfunc, v, pn_raw_directory, flags);

	} else if (pn_raw_directory->selected_device != NO_DEVICE) {
		//printf("YES SELECTED_DEVICE\n");
		// device directory -- not bus-specific
		if ( IsInterfaceDir(pn_raw_directory) ) {
			ret = FS_devdir(dirfunc, v, pn_raw_directory);
		} else if ( BusIsServer( pn_raw_directory->selected_connection) ) {
			ret = ServerDir(dirfunc, v, pn_raw_directory, flags);
		} else if ( IsStructureDir( pn_raw_directory ) ) {
			ret = FS_structdevdir( dirfunc, v, pn_raw_directory ) ;
		} else {
			ret = FS_devdir(dirfunc, v, pn_raw_directory);
		}

	} else if (NotRealDir(pn_raw_directory)) {
		//printf("NOT_REAL_DIR\n");
		// structure, statistics, system or settings dir -- not bus-specific
		ret = FS_typedir(dirfunc, v, pn_raw_directory);

	} else if (SpecifiedLocalBus(pn_raw_directory)) {
		if (IsAlarmDir(pn_raw_directory)) {	/* root or branch directory -- alarm state */
			ret = FS_alarmdir(dirfunc, v, pn_raw_directory);
		} else {
			if (pn_raw_directory->ds2409_depth == 0) {
				// only add funny directories for non-micro hub (DS2409) branches
				FS_interface_dir(dirfunc, v, pn_raw_directory);
			}
			/* Now get the actual devices */
			ret = FS_cache_or_real(dirfunc, v, pn_raw_directory, flags);
			/* simultaneous directory */
			if (flags[0] & (DEV_temp | DEV_volt)) {
				FS_simultaneous_entry(dirfunc, v, pn_raw_directory);
			}
			if (flags[0] & DEV_alarm) {
				FS_alarm_entry(dirfunc, v, pn_raw_directory);
			}
		}

	} else if ( IsAlarmDir(pn_raw_directory)) { // alarm for all busses
		// Not specified bus, so scan through all and print union
		ret = FS_dir_all_connections(dirfunc, v, pn_raw_directory, flags);
		// add no chaff to alarm directory -- no "uncached", "bus.x" etc
	} else { // standard directory search -- all busses
		// Not specified bus, so scan through all and print union
		int server_type = Globals.program_type==program_type_server || Globals.program_type==program_type_external ;
		ret = FS_dir_all_connections(dirfunc, v, pn_raw_directory, flags);
		if ( !server_type || ShouldReturnBusList(pn_raw_directory)) {
			if (pn_raw_directory->ds2409_depth == 0) {
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

	LEVEL_DEBUG("ret=%d", ret);
	return ret;
}

/* directories about the internal pogram state and configuration rather than device data */
static void FS_stype_dir(void (*dirfunc) (void *, const struct parsedname *), void *v, const struct parsedname *pn_root_directory)
{
	uint32_t ignoreflag ;
	FS_dir_plus(dirfunc, v, &ignoreflag, pn_root_directory, ePN_name[ePN_settings]);
	FS_dir_plus(dirfunc, v, &ignoreflag, pn_root_directory, ePN_name[ePN_system]);
	FS_dir_plus(dirfunc, v, &ignoreflag, pn_root_directory, ePN_name[ePN_statistics]);
	FS_dir_plus(dirfunc, v, &ignoreflag, pn_root_directory, ePN_name[ePN_structure]);
}

/* interface (only for bus directories) -- actually generated by the layer ABOVE that bus. */
static void FS_interface_dir(void (*dirfunc) (void *, const struct parsedname *), void *v, const struct parsedname *pn_root_directory)
{
	uint32_t ignoreflag ;
	FS_dir_plus(dirfunc, v, &ignoreflag, pn_root_directory, ePN_name[ePN_interface]);
}

/* Some devices have a special state when certain events are found called the ALARM state */
static void FS_alarm_entry(void (*dirfunc) (void *, const struct parsedname *), void *v, const struct parsedname *pn_root_directory)
{
	uint32_t ignoreflag ;
	FS_dir_plus(dirfunc, v, &ignoreflag, pn_root_directory, "alarm");
}

/* Add the "uncached" directory as a mnenomic aid to top level directory listings. */
static void FS_uncached_dir(void (*dirfunc) (void *, const struct parsedname *), void *v, const struct parsedname *pn_root_directory)
{
	uint32_t ignoreflag ;

	if (IsUncachedDir(pn_root_directory)) {	/* already uncached */
		return;
	}

	FS_dir_plus(dirfunc, v, &ignoreflag, pn_root_directory, "uncached");
}

/* Some temperature and voltage measurements can be triggered globally for considerable speed improvements */
static void FS_simultaneous_entry(void (*dirfunc) (void *, const struct parsedname *), void *v, const struct parsedname *pn_root_directory)
{
	uint32_t ignoreflag ;
	FS_dir_plus(dirfunc, v, &ignoreflag, pn_root_directory, "simultaneous");
}

/* path is the path which "pn_directory" parses */
/* FS_dir_all_connections produces the data that can vary: device lists, etc. */

struct dir_all_connections_struct {
	struct port_in * pin ;
	struct connection_in * cin ;
	struct parsedname pn_directory;
	void (*dirfunc) (void *, const struct parsedname *);
	void *v;
	uint32_t flags;
	ZERO_OR_ERROR ret;
};

/* Embedded function */
static void *FS_dir_all_connections_callback_conn(void *v)
{
	struct dir_all_connections_struct *dacs = v;
	struct dir_all_connections_struct dacs_next ;
	pthread_t thread;
	int threadbad = 0;
	
	// set up structure
	dacs_next.cin = dacs->cin->next ;

	if ( dacs_next.cin == NO_CONNECTION ) {
		threadbad = 1 ;
	} else {
		dacs_next.pin = dacs->pin ;
		dacs_next.dirfunc = dacs->dirfunc ;
		memcpy( &(dacs_next.pn_directory), &(dacs->pn_directory), sizeof(struct parsedname));	// shallow copy
		dacs_next.v = dacs->v ;
		dacs_next.flags = dacs->flags ;
		dacs_next.ret = dacs->ret ;
		threadbad = pthread_create(&thread, DEFAULT_THREAD_ATTR, FS_dir_all_connections_callback_conn, (void *) (&dacs_next));
	}

	SetKnownBus(dacs->cin->index, &(dacs->pn_directory) );

	if ( BAD(TestConnection( &(dacs->pn_directory) )) ) {	// reconnect ok?
		dacs->ret = -ECONNABORTED;
	} else if (BusIsServer(dacs->pn_directory.selected_connection)) {	/* is this a remote bus? */
		//printf("FS_dir_all_connections: Call ServerDir %s\n", dacs->pn_directory->path);
		dacs->ret = ServerDir(dacs->dirfunc, dacs->v, &(dacs->pn_directory), &(dacs->flags));
	} else if (IsAlarmDir( &(dacs->pn_directory) ) ) {	/* root or branch directory -- alarm state */
		//printf("FS_dir_all_connections: Call FS_alarmdir %s\n", dacs->pn_directory->path);
		dacs->ret = FS_alarmdir(dacs->dirfunc, dacs->v, &(dacs->pn_directory) );
	} else {
		dacs->ret = FS_cache_or_real(dacs->dirfunc, dacs->v, &(dacs->pn_directory), &(dacs->flags));
	}
	//printf("FS_dir_all_connections4 pid=%ld adapter=%d ret=%d\n",pthread_self(), dacs->pn_directory->selected_connection->index,ret);
	/* See if next bus was also queried */
	if (threadbad == 0) {		/* was a thread created? */
		if (pthread_join(thread, NULL)!= 0) {
			return VOID_RETURN ;			/* cannot join, so return only this result */
		}
		if (dacs_next.ret >= 0) {
			dacs->ret = dacs_next.ret;	/* is it an error return? Then return this one */
		} else {
			dacs->flags |= dacs_next.flags ;
		}		
	}
	return VOID_RETURN;
}

static void *FS_dir_all_connections_callback_port(void *v)
{
	struct dir_all_connections_struct *dacs = v;
	struct dir_all_connections_struct dacs_next ;
	pthread_t thread;
	int threadbad = 0;
	
	if ( dacs->pin == NULL ) {
		return VOID_RETURN;
	}

	// set up structure
	dacs_next.pin = dacs->pin->next ;

	if ( dacs_next.pin == NULL ) {
		threadbad = 1 ;
	} else {
		dacs_next.dirfunc = dacs->dirfunc ;
		memcpy( &(dacs_next.pn_directory), &(dacs->pn_directory), sizeof(struct parsedname));	// shallow copy
		dacs_next.v = dacs->v ;
		dacs_next.flags = dacs->flags ;
		dacs_next.ret = dacs->ret ;
		threadbad = pthread_create(&thread, DEFAULT_THREAD_ATTR, FS_dir_all_connections_callback_port, (void *) (&dacs_next));
	}

	dacs->cin = dacs->pin->first ;
	if ( dacs->cin != NO_CONNECTION ) {
		FS_dir_all_connections_callback_conn( v ) ;
	}
		
	//printf("FS_dir_all_connections4 pid=%ld adapter=%d ret=%d\n",pthread_self(), dacs->pn_directory->selected_connection->index,ret);
	/* See if next bus was also queried */
	if (threadbad == 0) {		/* was a thread created? */
		if (pthread_join(thread, NULL)!= 0) {
			return VOID_RETURN ;			/* cannot join, so return only this result */
		}
		if (dacs_next.ret >= 0) {
			dacs->ret = dacs_next.ret;	/* is it an error return? Then return this one */
		} else {
			dacs->flags |= dacs_next.flags ;
		}		
	}
	return VOID_RETURN;
}

static ZERO_OR_ERROR
FS_dir_all_connections(void (*dirfunc) (void *, const struct parsedname *), void *v, const struct parsedname *pn_directory, uint32_t * flags)
{
	struct dir_all_connections_struct dacs ;
		
	// set up structure
	dacs.pin = Inbound_Control.head_port ; 
	dacs.dirfunc = dirfunc ;
	memcpy( &(dacs.pn_directory), pn_directory, sizeof(struct parsedname));	// shallow copy
	dacs.v = v ;
	dacs.flags = 0 ;
	dacs.ret = 0 ;
	
	// Start iterating through buses
	FS_dir_all_connections_callback_port(  (void *) (& dacs) ) ;
	
	*flags = dacs.flags ;	
	return dacs.ret ;
}

/* Device directory (i.e. show the properties) -- all from memory */
/* Respect the Visibility status and also show only the correct subdir level */
static ZERO_OR_ERROR FS_devdir(void (*dirfunc) (void *, const struct parsedname *), void *v, const struct parsedname *pn_device_directory)
{
	struct device * dev = pn_device_directory->selected_device ;
	struct filetype *lastft = &(dev->filetype_array[dev->count_of_filetypes]);	/* last filetype struct */
	struct filetype *ft_pointer;	/* first filetype struct */
	char subdir_name[OW_FULLNAME_MAX + 1];
	size_t subdir_len;
	uint32_t ignoreflag ;

	STAT_ADD1(dir_dev.calls);

	// Add subdir to name (SubDirectory is within a device, but an extra layer of grouping of properties)
	if (pn_device_directory->subdir == NO_SUBDIR) {
		// not sub directory
		subdir_name[0] = '\0' ;
		subdir_len = 0;
		ft_pointer = dev->filetype_array;
	} else {
		// device subdirectory -- so use the sorted list to find all entries with the same prefix
		strncpy(subdir_name, pn_device_directory->subdir->name, OW_FULLNAME_MAX);
		strcat(subdir_name, "/");
		subdir_len = strlen(subdir_name);
		ft_pointer = pn_device_directory->subdir + 1; // next element (first one truly in the subdir)
	}

	for (; ft_pointer < lastft; ++ft_pointer) {	/* loop through filetypes */
		char *namepart ;

		/* test that start of name matches directory name */
		if (strncmp(ft_pointer->name, subdir_name, subdir_len) != 0) {
			// end of subdir
			break;
		}

		namepart = &ft_pointer->name[subdir_len]; // point after subdir name
		if ( strchr( namepart, '/') != NULL) {
			// subdir elements (and we're not in this subdir!)
			continue;
		}

		if (ft_pointer->ag==NON_AGGREGATE) {
			FS_dir_plus(dirfunc, v, &ignoreflag, pn_device_directory, namepart);
			STAT_ADD1(dir_dev.entries);
		} else if (ft_pointer->ag->combined==ag_sparse) {
			struct parsedname s_pn_file_entry;
			struct parsedname *pn_file_entry = &s_pn_file_entry;
			if (ft_pointer->ag->letters==ag_letters) {
				if (FS_ParsedNamePlusText(pn_device_directory->path, namepart, "xxx", pn_file_entry) == 0) {
					switch ( FS_visible(pn_file_entry) ) { // hide hidden properties
						case visible_now :
						case visible_always:
							FS_dir_entry_aliased( dirfunc, v, pn_file_entry) ;
							STAT_ADD1(dir_dev.entries);
							break ;
						default:
							break ;
					}
					FS_ParsedName_destroy(pn_file_entry);
				}
			} else {
				if (FS_ParsedNamePlusText(pn_device_directory->path, namepart, "000", pn_file_entry) == 0) {
					switch ( FS_visible(pn_file_entry) ) { // hide hidden properties
						case visible_now :
						case visible_always:
							FS_dir_entry_aliased( dirfunc, v, pn_file_entry) ;
							STAT_ADD1(dir_dev.entries);
							break ;
						default:
							break ;
					}
					FS_ParsedName_destroy(pn_file_entry);
				}
			}
		} else {
			int extension;
			int first_extension = (ft_pointer->format == ft_bitfield) ? EXTENSION_BYTE : EXTENSION_ALL;
			struct parsedname s_pn_file_entry;
			struct parsedname *pn_file_entry = &s_pn_file_entry;
			for (extension = first_extension; extension < ft_pointer->ag->elements; ++extension) {
				if (FS_ParsedNamePlusExt(pn_device_directory->path, namepart, extension, ft_pointer->ag->letters, pn_file_entry) == 0) {
					switch ( FS_visible(pn_file_entry) ) { // hide hidden properties
						case visible_now :
						case visible_always:
							FS_dir_entry_aliased( dirfunc, v, pn_file_entry) ;
							STAT_ADD1(dir_dev.entries);
							break ;
						default:
							break ;
					}
					FS_ParsedName_destroy(pn_file_entry);
				}
			}
		}
	}
	return 0;
}

/* Device directory -- all from memory  -- for structure */
static ZERO_OR_ERROR FS_structdevdir(void (*dirfunc) (void *, const struct parsedname *), void *v, const struct parsedname *pn_device_directory)
{
	struct filetype *lastft = &(pn_device_directory->selected_device->filetype_array[pn_device_directory->selected_device->count_of_filetypes]);	/* last filetype struct */
	struct filetype *ft_pointer;	/* first filetype struct */
	char subdir_name[OW_FULLNAME_MAX + 1];
	size_t subdir_len;

	// Add subdir to name (SubDirectory is within a device, but an extra layer of grouping of properties)
	if (pn_device_directory->subdir == NO_SUBDIR) {
		// not sub directory
		subdir_name[0] = '\0' ;
		subdir_len = 0;
		ft_pointer = pn_device_directory->selected_device->filetype_array;
	} else {
		// device subdirectory -- so use the sorted list to find all entries with the same prefix
		strncpy(subdir_name, pn_device_directory->subdir->name, OW_FULLNAME_MAX);
		strcat(subdir_name, "/");
		subdir_len = strlen(subdir_name);
		ft_pointer = pn_device_directory->subdir + 1; // next element (first one truly in the subdir)
	}


	for (; ft_pointer < lastft; ++ft_pointer) {	/* loop through filetypes */
		char *namepart ;

 		if ( ft_pointer->visible == INVISIBLE ) { // hide always invisible
			// done without actually calling the visibility function since
			// the device is generic for structure listings and may not
			// be an actual device.
			continue ;
		}

		/* test that start of name matches directory name */
		if (strncmp(ft_pointer->name, subdir_name, subdir_len) != 0) {
			// end of subdir
			break;
		}

		namepart = &ft_pointer->name[subdir_len]; // point after subdir name
		if ( strchr( namepart, '/') != NULL) {
			// subdir elements (and we're not in this subdir!)
			continue;
		}

		if (ft_pointer->ag==NON_AGGREGATE) {
			struct parsedname s_pn_file_entry;
			struct parsedname *pn_file_entry = &s_pn_file_entry;
			if (FS_ParsedNamePlus(pn_device_directory->path, namepart, pn_file_entry) == 0) {
				FS_dir_entry_aliased( dirfunc, v, pn_file_entry) ;
				FS_ParsedName_destroy(pn_file_entry);
			}
		} else if (ft_pointer->ag->combined==ag_sparse) {
			// Aggregate property
			struct parsedname s_pn_file_entry;
			struct parsedname *pn_file_entry = &s_pn_file_entry;

			if ( ft_pointer->ag->letters == ag_letters ) {
				if (FS_ParsedNamePlusText(pn_device_directory->path, namepart, "xxx", pn_file_entry) == 0) {
					FS_dir_entry_aliased( dirfunc, v, pn_file_entry) ;
					FS_ParsedName_destroy(pn_file_entry);
				}
			} else {
				if (FS_ParsedNamePlusText(pn_device_directory->path, namepart, "000", pn_file_entry) == 0) {
					FS_dir_entry_aliased( dirfunc, v, pn_file_entry) ;
					FS_ParsedName_destroy(pn_file_entry);
				}
			}
		} else {
			// Aggregate property
			struct parsedname s_pn_file_entry;
			struct parsedname *pn_file_entry = &s_pn_file_entry;

			if ( ft_pointer->format == ft_bitfield ) {
				if (FS_ParsedNamePlusExt(pn_device_directory->path, namepart, EXTENSION_BYTE, ft_pointer->ag->letters, pn_file_entry) == 0) {
					FS_dir_entry_aliased( dirfunc, v, pn_file_entry) ;
					FS_ParsedName_destroy(pn_file_entry);
				}
			}
			if (FS_ParsedNamePlusExt(pn_device_directory->path, namepart, EXTENSION_ALL, ft_pointer->ag->letters, pn_file_entry) == 0) {
				FS_dir_entry_aliased( dirfunc, v, pn_file_entry) ;
				FS_ParsedName_destroy(pn_file_entry);
			}
			// unlike real directory, only show the first array element since the data is redundant
			if (FS_ParsedNamePlusExt(pn_device_directory->path, namepart, 0, ft_pointer->ag->letters, pn_file_entry) == 0) {
				FS_dir_entry_aliased( dirfunc, v, pn_file_entry) ;
				FS_ParsedName_destroy(pn_file_entry);
			}
		}
	}
	return 0;
}

/* Note -- alarm directory is smaller, no adapters or stats or uncached */
static ZERO_OR_ERROR FS_alarmdir(void (*dirfunc) (void *, const struct parsedname *), void *v, const struct parsedname *pn_alarm_directory)
{
	enum search_status ret;
	struct device_search ds;	// holds search state
	uint32_t ignoreflag ;

	/* Special handling for External directory -- no alarm */
	if ( get_busmode(pn_alarm_directory->selected_connection) == bus_external ) {
		return 0 ;
	}

	/* cache from Server if this is a remote bus */
	if (BusIsServer(pn_alarm_directory->selected_connection)) {
		return ServerDir(dirfunc, v, pn_alarm_directory, &ignoreflag);
	}

	/* Certain buses are not real bus masters (like usb-monitor and w1-monitor) */
	if ( pn_alarm_directory->selected_connection->iroutines.flags & ADAP_FLAG_sham ) {
		return 0 ;
	}

	/* STATISCTICS */
	STAT_ADD1(dir_main.calls);

	ret = PossiblyLockedBusCall( BUS_first_alarm, &ds, pn_alarm_directory) ;

	while ( ret == search_good ) {
		char dev[PROPERTY_LENGTH_ALIAS + 1];
		STAT_ADD1(dir_main.entries);
		FS_devicename(dev, PROPERTY_LENGTH_ALIAS, ds.sn, pn_alarm_directory);
		FS_dir_plus(dirfunc, v, &ignoreflag, pn_alarm_directory, dev);

		ret = PossiblyLockedBusCall( BUS_next, &ds, pn_alarm_directory) ;
	}

	switch ( ret ) {
		case search_good:
		case search_done:
			return 0 ;
		case search_error:
		default:
			return -EIO;
	}	
}

static enum search_status PossiblyLockedBusCall( enum search_status (* first_next)(struct device_search *, const struct parsedname *), struct device_search * ds, const struct parsedname * pn )
{
	enum search_status ret;
	
	// This is also called from the reconnection routine -- we use a flag to avoid mutex doubling and deadlock
	if ( NotReconnect(pn) ) {
		BUSLOCK(pn);
		ret = first_next(ds, pn) ;
		BUSUNLOCK(pn);
	} else {
		ret = first_next(ds, pn) ;
	}
	return ret ;
}

/* A directory of devices -- either main or branch */
/* not within a device, nor alarm state */
/* Also, adapters and stats handled elsewhere */
/* Scan the directory from the BUS and add to cache */
static ZERO_OR_ERROR FS_realdir(void (*dirfunc) (void *, const struct parsedname *), void *v, const struct parsedname *pn_whole_directory, uint32_t * flags)
{
	struct device_search ds;
	size_t devices = 0;
	struct dirblob db;
	enum search_status ret;

	/* cache from Server if this is a remote bus */
	if (BusIsServer(pn_whole_directory->selected_connection)) {
		return ServerDir(dirfunc, v, pn_whole_directory, flags);
	}

	/* Certain buses are not real bus masters (like usb-monitor and w1-monitor) */
	if ( pn_whole_directory->selected_connection->iroutines.flags & ADAP_FLAG_sham ) {
		return 0 ;
	}

	/* STATISTICS */
	STAT_ADD1(dir_main.calls);

	DirblobInit(&db);			// set up a fresh dirblob

	ret = PossiblyLockedBusCall( BUS_first, &ds, pn_whole_directory) ;
	
	if (RootNotBranch(pn_whole_directory)) {
		db.allocated = pn_whole_directory->selected_connection->last_root_devs;	// root dir estimated length
	}
	while ( ret == search_good ) {
		char dev[PROPERTY_LENGTH_ALIAS + 1];

		/* Add to device cache */
		Cache_Add_Device(pn_whole_directory->selected_connection->index,ds.sn) ;
		
		/* Get proper device name (including alias subst) */
		FS_devicename(dev, PROPERTY_LENGTH_ALIAS, ds.sn, pn_whole_directory);
		
		/* Execute callback function */
		if ( FS_dir_plus(dirfunc, v, flags, pn_whole_directory, dev) != 0 ) {
			DirblobPoison(&db);
			break ;
		}
		DirblobAdd(ds.sn, &db);
		++devices;

		ret = PossiblyLockedBusCall( BUS_next, &ds, pn_whole_directory) ;
	} 

	STATLOCK;
	dir_main.entries += devices;
	STATUNLOCK;

	switch ( ret ) {
		case search_done:
			if ( RootNotBranch(pn_whole_directory) ) {
				pn_whole_directory->selected_connection->last_root_devs = devices;	// root dir estimated length
			}
			/* Add to the cache (full list as a single element */
			if (DirblobPure(&db) && (ret == search_done) ) {
				Cache_Add_Dir(&db, pn_whole_directory);
			}
			DirblobClear(&db);
			return 0 ;
		case search_good:
		case search_error:
		default:
			DirblobClear(&db);
			return -EIO ;
	}
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
		memset(pn_directory->sn, 0, SERIAL_NUMBER_SIZE);
	} else {
		// Stuff the branch into the checksum slot
		// Makes the branch address unique (DS2409 id + branch)
		--pn_directory->ds2409_depth;
		memcpy(pn_directory->sn, pn_directory->bp[pn_directory->ds2409_depth].sn, SERIAL_NUMBER_SIZE-1);
		pn_directory->sn[SERIAL_NUMBER_SIZE-1] = pn_directory->bp[pn_directory->ds2409_depth].branch;
	}
	pn_directory->selected_device = NO_DEVICE;
}

/* A directory of devices -- either main or branch */
/* not within a device, nor alarm state */
/* Also, adapters and stats handled elsewhere */
/* Cache2Real try the cache first, else get directory from bus (and add to cache) */
static ZERO_OR_ERROR FS_cache_or_real(void (*dirfunc) (void *, const struct parsedname *), void *v, const struct parsedname *pn_real_directory, uint32_t * flags)
{
	size_t dindex;
	struct dirblob db;
	BYTE sn[8];

	/* Special handling for External directory -- just walk tree */
	if ( get_busmode(pn_real_directory->selected_connection) == bus_external ) {
		return FS_externaldir(dirfunc, v, pn_real_directory) ;
	}
	
	/* Test to see whether we should get the directory "directly" */
	if (
		SpecifiedBus(pn_real_directory) 
		|| IsUncachedDir(pn_real_directory) // asking for uncached
		|| BAD( Cache_Get_Dir(&db, pn_real_directory)) // cache'd version isn't available (or old)
		) {
		// directly
		return FS_realdir(dirfunc, v, pn_real_directory, flags);
	}

	// Use cached version of directory

	/* STATISTICS */
	STAT_ADD1(dir_main.calls);

	/* Get directory from the cache */
	for (dindex = 0; DirblobGet(dindex, sn, &db) == 0; ++dindex) {
		char dev[PROPERTY_LENGTH_ALIAS + 1];

		FS_devicename(dev, PROPERTY_LENGTH_ALIAS, sn, pn_real_directory);
		FS_dir_plus(dirfunc, v, flags, pn_real_directory, dev);
	}
	DirblobClear(&db);			/* allocated in Cache_Get_Dir */

	STATLOCK;

	dir_main.entries += dindex;

	STATUNLOCK;
	return 0;
}

// must lock a global struct for walking through tree -- limitation of "twalk"
// struct for walking through tree -- cannot send data except globally
struct {
	void (*dirfunc) (void *, const struct parsedname *);
	void *v;
	struct parsedname *pn_directory;
} typedir_action_struct;

static void Typediraction(const void *t, const VISIT which, const int depth)
{
	uint32_t ignoreflag ;
	(void) depth;
	switch (which) {
	case leaf:
	case postorder:
		FS_dir_plus(typedir_action_struct.dirfunc, typedir_action_struct.v, &ignoreflag, typedir_action_struct.pn_directory,
					((const struct device_opaque *) t)->key->family_code);
	default:
		break;
	}
}

/* Show the pn_directory->type (statistics, system, ...) entries */
/* Only the top levels, the rest will be shown by FS_devdir */
static ZERO_OR_ERROR FS_typedir(void (*dirfunc) (void *, const struct parsedname *), void *v, const struct parsedname *pn_type_directory)
{
	struct parsedname s_pn_type_device;
	struct parsedname *pn_type_device = &s_pn_type_device;


	memcpy(pn_type_device, pn_type_directory, sizeof(struct parsedname));	// shallow copy

	LEVEL_DEBUG("called on %s", pn_type_directory->path);

	TYPEDIRLOCK;

	typedir_action_struct.dirfunc = dirfunc;
	typedir_action_struct.v = v;
	typedir_action_struct.pn_directory = pn_type_device;
	twalk(Tree[pn_type_directory->type], Typediraction);

	TYPEDIRUNLOCK;

	return 0;
}

// must lock a global struct for walking through tree -- limitation of "twalk"
// struct for walking through tree -- cannot send data except globally
struct {
	void (*dirfunc) (void *, const struct parsedname *);
	void *v;
	struct parsedname *pn_directory;
} externaldir_action_struct;

static void Externaldiraction(const void *nodep, const VISIT which, const int depth)
{
	const struct sensor_node *p = *(struct sensor_node * const *) nodep;
	uint32_t ignoreflag ;
	(void) depth;

	switch (which) {
	case leaf:
	case postorder:
		FS_dir_plus(externaldir_action_struct.dirfunc, externaldir_action_struct.v, &ignoreflag, externaldir_action_struct.pn_directory,p->name);
	default:
		break;
	}
}

/* Show the external entries */
/* Only the sensors */
static ZERO_OR_ERROR FS_externaldir(void (*dirfunc) (void *, const struct parsedname *), void *v, const struct parsedname *pn_external_directory)
{
	struct parsedname s_pn_external_device;
	struct parsedname *pn_external_device = &s_pn_external_device;


	memcpy(pn_external_device, pn_external_directory, sizeof(struct parsedname));	// shallow copy

	LEVEL_DEBUG("called on %s", pn_external_directory->path);

	EXTERNALDIRLOCK;

	externaldir_action_struct.dirfunc = dirfunc;
	externaldir_action_struct.v = v;
	externaldir_action_struct.pn_directory = pn_external_device;
	twalk( sensor_tree, Externaldiraction);

	EXTERNALDIRUNLOCK;

	return 0;
}

/* Show the bus entries */
static ZERO_OR_ERROR FS_busdir(void (*dirfunc) (void *, const struct parsedname *), void *v, const struct parsedname *pn_directory)
{
	char bus[OW_FULLNAME_MAX];
	struct port_in * pin ;
	uint32_t ignoreflag ;

	if (!RootNotBranch(pn_directory)) {
		return 0;
	}

	for ( pin = Inbound_Control.head_port ; pin != NULL ; pin = pin->next ) {
		struct connection_in * cin ;
		for ( cin = pin->first ; cin != NO_CONNECTION ; cin = cin->next ) {
			UCLIBCLOCK;
			snprintf(bus, OW_FULLNAME_MAX, "bus.%d", cin->index);
			UCLIBCUNLOCK;
			FS_dir_plus(dirfunc, v, &ignoreflag, pn_directory, bus);
		}
	}

	return 0;
}

/* Parse and show */
static ZERO_OR_ERROR FS_dir_plus(void (*dirfunc) (void *, const struct parsedname *), void *v, uint32_t * flags, const struct parsedname *pn_directory, const char *file)
{
	struct parsedname s_pn_plus_directory;
	struct parsedname *pn_plus_directory = &s_pn_plus_directory;

	if (FS_ParsedNamePlus(pn_directory->path, file, pn_plus_directory) == 0) {
 		switch ( FS_visible(pn_plus_directory) ) { // hide hidden properties
			case visible_now :
			case visible_always:
				FS_dir_entry_aliased( dirfunc, v, pn_plus_directory) ;
				if ( pn_plus_directory->selected_device ){
					flags[0] |= pn_plus_directory->selected_device->flags;
				}
				break ;
			default:
				break ;
		}
		FS_ParsedName_destroy(pn_plus_directory) ;
		return 0;
	}
	return -ENOENT;
}
