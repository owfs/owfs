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

static ZERO_OR_ERROR BranchAdd(struct parsedname *pn);

enum parse_pass {
	parse_pass_pre_remote,
	parse_pass_post_remote,
};

struct parsedname_pointers {
	char pathcpy[PATH_MAX+1];
	char *pathnow;
	char *pathnext;
	char *pathlast;
};

static enum parse_enum Parse_Unspecified(char *pathnow, enum parse_pass remote_status, struct parsedname *pn);
static enum parse_enum Parse_Branch(char *pathnow, enum parse_pass remote_status, struct parsedname *pn);
static enum parse_enum Parse_Real(char *pathnow, enum parse_pass remote_status, struct parsedname *pn);
static enum parse_enum Parse_NonReal(char *pathnow, struct parsedname *pn);
static enum parse_enum Parse_RealDevice(char *filename, enum parse_pass remote_status, struct parsedname *pn);
static enum parse_enum Parse_NonRealDevice(char *filename, struct parsedname *pn);
static enum parse_enum Parse_Bus(char *pathnow, struct parsedname *pn);
static ZERO_OR_ERROR FS_ParsedName_anywhere(const char *path, enum parse_pass remote_status, struct parsedname *pn);
static ZERO_OR_ERROR FS_ParsedName_setup(struct parsedname_pointers *pp, const char *path, struct parsedname *pn);

#define BRANCH_INCR (9)

/* ---------------------------------------------- */
/* Filename (path) parsing functions              */
/* ---------------------------------------------- */
void FS_ParsedName_destroy(struct parsedname *pn)
{
	if (!pn) {
		return;
	}
	LEVEL_DEBUG("%s", SAFESTRING(pn->path));
	CONNIN_RUNLOCK ;
	if (pn->bp) {
		owfree(pn->bp);
		pn->bp = NULL;
	}
	if (pn->path) {
		owfree(pn->path);
		pn->path = NULL;
	}
}

// Path is either NULL (in which case a minimal structure is created that doesn't need Destroy -- used for Bus Master setups)
// Or Path is a full "filename" type string of form: 10.1243Ab000 or uncached/bus.0/statistics etc.
// The Path passed in isn't altered, but 2 copies are made -- one with the full path, the other (busless) has the first bus.n removed.
// An initial / is added to the path, and the full length has to be less than MAX_PATH (2048)
// For efficiency, the two path copies are allocated in the same call, and so can be removed together.

/* Parse a path to check it's validity and attach to the propery data structures */
ZERO_OR_ERROR FS_ParsedName(const char *path, struct parsedname *pn)
{
	return FS_ParsedName_anywhere(path, parse_pass_pre_remote, pn);
}

/* Parse a path from a remote source back -- so don't check presence */
ZERO_OR_ERROR FS_ParsedName_BackFromRemote(const char *path, struct parsedname *pn)
{
	return FS_ParsedName_anywhere(path, parse_pass_post_remote, pn);
}

/* Parse off starting "mode" directory (uncached, alarm...) */
static ZERO_OR_ERROR FS_ParsedName_anywhere(const char *path, enum parse_pass remote_status, struct parsedname *pn)
{
	struct parsedname_pointers s_pp;
	struct parsedname_pointers *pp = &s_pp;
	ZERO_OR_ERROR parse_error_status = 0;
	enum parse_enum pe = parse_first;

	// To make the debug output useful it's cleared here.
	// Even on normal glibc, errno isn't cleared on good system calls
	errno = 0;

	LEVEL_CALL("path=[%s]", SAFESTRING(path));

	parse_error_status = FS_ParsedName_setup(pp, path, pn);
	if (parse_error_status) {
		return parse_error_status;
	}
	if (path == NULL) {
		return 0;
	}

	while (1) {
		//printf("PARSENAME parse_enum=%d pathnow=%s\n",pe,SAFESTRING(pp->pathnow));
		// Check for extreme conditions (done, error)
		switch (pe) {

		case parse_done:		// the only exit!
			//LEVEL_DEBUG("PARSENAME parse_done") ;
			//printf("PARSENAME end parse_error_status=%d\n",parse_error_status) ;
			if (parse_error_status) {
				FS_ParsedName_destroy(pn);
				return parse_error_status ;
			}
			//printf("%s: Parse %s before corrections: %.4X -- state = %d\n",(back_from_remote)?"BACK":"FORE",pn->path,pn->state,pn->type) ;
			if (SpecifiedRemoteBus(pn) && !SpecifiedVeryRemoteBus(pn)) {
				// promote interface of remote bus to local
				if (pn->type == ePN_interface) {
					pn->state &= ~ePS_busremote;
					pn->state |= ePS_buslocal;

					// non-root remote bus
				} else if (pn->type != ePN_root) {
					pn->state |= ePS_busveryremote;
				}
			}
			// root buses are considered "real"
			if (pn->type == ePN_root) {
				pn->type = ePN_real;	// default state
			}
			//printf("%s: Parse %s after  corrections: %.4X -- state = %d\n\n",(back_from_remote)?"BACK":"FORE",pn->path,pn->state,pn->type) ;
			return 0;

		case parse_error:
			//LEVEL_DEBUG("PARSENAME parse_error") ;
			parse_error_status = -ENOENT;
			pe = parse_done;
			continue;

		default:
			break;
		}

		// break out next name in path, make sure pp->pathnext isn't NULL. (SIGSEGV in uClibc)
		pp->pathnow = (pp->pathnext) ? strsep(&(pp->pathnext), "/") : NULL;
		//LEVEL_DEBUG("PARSENAME pathnow=[%s] rest=[%s]",pp->pathnow,pp->pathnext) ;
		if (pp->pathnow == NULL || pp->pathnow[0] == '\0') {
			pe = parse_done;
			continue;
		}
		// rest of state machine on parsename
		switch (pe) {

		case parse_first:
			//LEVEL_DEBUG("PARSENAME parse_first") ;
			pe = Parse_Unspecified(pp->pathnow, remote_status, pn);
			break;

		case parse_real:
			//LEVEL_DEBUG("PARSENAME parse_real") ;
			pe = Parse_Real(pp->pathnow, remote_status, pn);
			break;

		case parse_branch:
			//LEVEL_DEBUG("PARSENAME parse_branch") ;
			pe = Parse_Branch(pp->pathnow, remote_status, pn);
			break;

		case parse_nonreal:
			//LEVEL_DEBUG("PARSENAME parse_nonreal\n") ;
			pe = Parse_NonReal(pp->pathnow, pn);
			break;

		case parse_prop:
			//LEVEL_DEBUG("PARSENAME parse_prop") ;
			pn->dirlength = pp->pathnow - pp->pathcpy + 1 ;
			//LEVEL_DEBUG("Dirlength=%d <%*s>",pn->dirlength,pn->dirlength,pn->path) ;
			//printf("dirlength = %d which makes the path <%s> <%.*s>\n",pn->dirlength,pn->path,pn->dirlength,pn->path);
			pp->pathlast = pp->pathnow;	/* Save for concatination if subdirectory later wanted */
			pe = Parse_Property(pp->pathnow, pn);
			break;

		case parse_subprop:
			//LEVEL_DEBUG("PARSENAME parse_subprop") ;
			pp->pathnow[-1] = '/';
			pe = Parse_Property(pp->pathlast, pn);
			break;

		default:
			pe = parse_error;	// unknown state
			break;

		}
		//printf("PARSENAME pe=%d\n",pe) ;
	}
}

/* Initial memory allocation and pn setup */
static ZERO_OR_ERROR FS_ParsedName_setup(struct parsedname_pointers *pp, const char *path, struct parsedname *pn)
{
	if (pn == NULL) {
		return -EINVAL;
	}

	memset(pn, 0, sizeof(struct parsedname));
	pn->known_bus = NULL;		/* all buses */

	/* Set the persistent state info (temp scale, ...) -- will be overwritten by client settings in the server */
	CONTROLFLAGSLOCK;
	pn->control_flags = LocalControlFlags | SHOULD_RETURN_BUS_LIST;	// initial flag as the bus-returning level, will change if a bus is specified
	CONTROLFLAGSUNLOCK;

	// initialization
	pp->pathnow = NULL;
	pp->pathlast = NULL;
	pp->pathnext = NULL;

	/* Default attributes */
	pn->state = ePS_normal;
	pn->type = ePN_root;

	/* No device lock yet assigned */
	pn->lock = NULL ;

	/* minimal structure for initial bus "detect" use -- really has connection and LocalControlFlags only */
	if (path == NULL) {
		return 0;
	}

	if (strlen(path) > PATH_MAX) {
		return -EBADF;
	}

	pn->path = (char *) owmalloc(2 * strlen(path) + 4);
	if (pn->path == NULL) {
		return -ENOMEM;
	}

	/* Have to save pn->path at once */
	strcpy(pn->path, "/"); // initial slash
	strcpy(pn->path+1, path[0]=='/'?path+1:path);
	pn->path_busless = pn->path + strlen(pn->path) + 1;
	strcpy(pn->path_busless, pn->path);

	/* make a copy for destructive parsing  without initial '/'*/
	strcpy(pp->pathcpy,&pn->path[1]);
	/* pointer to rest of path after current token peeled off */
	pp->pathnext = pp->pathcpy;
	pn->dirlength = strlen(pn->path) ;

	/* connection_in list and start */
	/* ---------------------------- */
	/* -- This is important:     -- */
	/* --     Buses can --          */
	/* -- be added by Browse so  -- */
	/* -- a reader/writer lock is - */
	/* -- held until ParsedNameDestroy */
	/* ---------------------------- */

	CONNIN_RLOCK;
	pn->selected_connection = Inbound_Control.head ; // Default bus assignment

	return 0;
}

// Early parsing -- only bus entries, uncached and text may have preceeded
static enum parse_enum Parse_Unspecified(char *pathnow, enum parse_pass remote_status, struct parsedname *pn)
{
	if (strncasecmp(pathnow, "bus.", 4) == 0) {
		return Parse_Bus(pathnow, pn);

	} else if (strcasecmp(pathnow, "settings") == 0) {
		if (SpecifiedLocalBus(pn)) {
			return parse_error;
		}
		pn->type = ePN_settings;
		return parse_nonreal;

	} else if (strcasecmp(pathnow, "statistics") == 0) {
		if (SpecifiedLocalBus(pn)) {
			return parse_error;
		}
		pn->type = ePN_statistics;
		return parse_nonreal;

	} else if (strcasecmp(pathnow, "structure") == 0) {
		if (SpecifiedLocalBus(pn)) {
			return parse_error;
		}
		pn->type = ePN_structure;
		return parse_nonreal;

	} else if (strcasecmp(pathnow, "system") == 0) {
		if (SpecifiedLocalBus(pn)) {
			return parse_error;
		}
		pn->type = ePN_system;
		return parse_nonreal;

	} else if (strcasecmp(pathnow, "interface") == 0) {
		if (!SpecifiedBus(pn)) {
			return parse_error;
		}
		pn->type = ePN_interface;
		return parse_nonreal;

	} else if (strcasecmp(pathnow, "text") == 0) {
		pn->state |= ePS_text;
		return parse_first;

	} else if (strcasecmp(pathnow, "uncached") == 0) {
		pn->state |= ePS_uncached;
		return parse_first;

	}

	pn->type = ePN_real;
	return Parse_Branch(pathnow, remote_status, pn);
}

static enum parse_enum Parse_Branch(char *pathnow, enum parse_pass remote_status, struct parsedname *pn)
{
	if (strcasecmp(pathnow, "alarm") == 0) {
		pn->state |= ePS_alarm;
		pn->type = ePN_real;
		return parse_real;
	}
	return Parse_Real(pathnow, remote_status, pn);
}

static enum parse_enum Parse_Real(char *pathnow, enum parse_pass remote_status, struct parsedname *pn)
{
	if (strcasecmp(pathnow, "simultaneous") == 0) {
		pn->selected_device = DeviceSimultaneous;
		return parse_prop;

	} else if (strcasecmp(pathnow, "text") == 0) {
		pn->state |= ePS_text;
		return parse_real;

	} else if (strcasecmp(pathnow, "thermostat") == 0) {
		pn->selected_device = DeviceThermostat;
		return parse_prop;

	} else if (strcasecmp(pathnow, "uncached") == 0) {
		pn->state |= ePS_uncached;
		return parse_real;

	} else {
		return Parse_RealDevice(pathnow, remote_status, pn);
	}

	return parse_error;
}

static enum parse_enum Parse_NonReal(char *pathnow, struct parsedname *pn)
{
	if (strcasecmp(pathnow, "text") == 0) {
		pn->state |= ePS_text;
		return parse_nonreal;

	} else if (strcasecmp(pathnow, "uncached") == 0) {
		pn->state |= ePS_uncached;
		return parse_nonreal;

	} else {
		return Parse_NonRealDevice(pathnow, pn);
	}

	return parse_error;
}

/* We've reached a /bus.n entry */
static enum parse_enum Parse_Bus(char *pathnow, struct parsedname *pn)
{
	char *found;
	int bus_number;
	/* Processing for bus.X directories -- eventually will make this more generic */
	if (!isdigit(pathnow[4])) {
		return parse_error;
	}

	bus_number = atoi(&pathnow[4]);
	if (bus_number < 0) {
		return parse_error;
	}

	/* Should make a presence check on remote buses here, but
	 * it's not a major problem if people use bad paths since
	 * they will just end up with empty directory listings. */
	if (SpecifiedLocalBus(pn)) {	/* already specified a "bus." */
		/* too many levels of bus for a non-remote adapter */
		return parse_error;
	} else if (SpecifiedRemoteBus(pn)) {	/* already specified a "bus." */
		/* Let the remote bus do the heavy lifting */
		pn->state |= ePS_busveryremote;
		return parse_first;
	}

	/* Since we are going to use a specific in-device now, set
	 * pn->selected_connection to point at that device at once. */
	if ( SetKnownBus(bus_number, pn) ) {
		return parse_error ; // bus doesn't exist
	}
	pn->state |= BusIsServer((pn)->selected_connection) ? ePS_busremote : ePS_buslocal ;

	if (SpecifiedLocalBus(pn)) {
		/* don't return bus-list for local paths. */
		pn->control_flags &= (~SHOULD_RETURN_BUS_LIST);
	}

	/* Create the path without the "bus.x" part in pn->path_busless */
	if ( (found = strstr(pn->path, "/bus.")) ) {
		int length = found - pn->path;
		if ((found = strchr(found + 1, '/'))) {	// more after bus
			strcpy(&(pn->path_busless[length]), found);	// copy rest
		} else {
			pn->path_busless[length] = '\0';	// add final null
		}
	}
	return parse_first;
}

/* Parse Name (only device name) part of string */
/* Return -ENOENT if not a valid name
   return 0 if good
   *next points to next segment, or NULL if not filetype
 */
static enum parse_enum Parse_RealDevice(char *filename, enum parse_pass remote_status, struct parsedname *pn)
{
	int bus_nr ;
	if ( Parse_SerialNumber(filename,pn->sn)  ) {
		// Not a serial number, look for an alias
		if (Cache_Get_SerialNumber(filename,pn->sn)) {
			return parse_error;
		}
	}

	/* Search for known 1-wire device -- keyed to device name (family code in HEX) */
	FS_devicefindhex(pn->sn[0], pn);

	// returning from owserver -- don't need to check presence (it's implied)
	if (remote_status == parse_pass_post_remote) {
		return parse_prop;
	}

	if (Globals.one_device) {
		bus_nr = 0;				// arbitrary assignment
	} else {
		/* Check the presence, and cache the proper bus number for better performance */
		bus_nr = CheckPresence(pn);
		if (bus_nr < 0) {
			return parse_error;	/* CheckPresence failed */
		}
	}

	return parse_prop;
}

/* Parse Name (non-device name) part of string */
/* Return -ENOENT if not a valid name
   return 0 if good
*/
static enum parse_enum Parse_NonRealDevice(char *filename, struct parsedname *pn)
{
	//printf("Parse_NonRealDevice: [%s] [%s]\n", filename, pn->path);
	FS_devicefind(filename, pn);
	return (pn->selected_device == &NoDevice) ? parse_error : parse_prop;
}

enum parse_enum Parse_Property(char *filename, struct parsedname *pn)
{
	char *dot = filename;

	//printf("FilePart: %s %s\n", filename, pn->path);

	filename = strsep(&dot, ".");
	//printf("FP name=%s, dot=%s\n", filename, dot);
	/* Match to known filetypes for this device */
	if ((pn->selected_filetype =
		 bsearch(filename, pn->selected_device->filetype_array,
				 (size_t) pn->selected_device->count_of_filetypes, sizeof(struct filetype), filetype_cmp))) {
		//printf("FP known filetype %s\n",pn->selected_filetype->name) ;
		/* Filetype found, now process extension */
		if (dot == NULL || dot[0] == '\0') {	/* no extension */
			if (pn->selected_filetype->ag != NON_AGGREGATE) {
				return parse_error;	/* aggregate filetypes need an extension */
			}
			pn->extension = 0;	/* default when no aggregate */

		} else if (pn->selected_filetype->ag == NON_AGGREGATE) {
			return parse_error;	/* An extension not allowed when non-aggregate */

		} else if (strcasecmp(dot, "ALL") == 0) {
			//printf("FP ALL\n");
			pn->extension = EXTENSION_ALL;	/* ALL */

		} else if (pn->selected_filetype->format == ft_bitfield && strcasecmp(dot, "BYTE") == 0) {
			pn->extension = EXTENSION_BYTE;	/* BYTE */
			//printf("FP BYTE\n") ;

		} else {				/* specific extension */
			if (pn->selected_filetype->ag->letters == ag_letters) {	/* Letters */
				//printf("FP letters\n") ;
				if ((strlen(dot) != 1) || !isupper(dot[0])) {
					return parse_error;
				}
				pn->extension = dot[0] - 'A';	/* Letter extension */
			} else {			/* Numbers */
				char *p;
				//printf("FP numbers\n") ;
				pn->extension = strtol(dot, &p, 0);	/* Number conversion */
				if ((p == dot) || ((pn->extension == 0) && (errno == -EINVAL))) {
					return parse_error;	/* Bad number */
				}
			}
			//printf("FP ext=%d nr_elements=%d\n", pn->extension, pn->selected_filetype->ag->elements) ;
			/* Now check range */
			if ((pn->extension < 0)
				|| (pn->extension >= pn->selected_filetype->ag->elements)) {
				//printf("FP Extension out of range %d %d %s\n", pn->extension, pn->selected_filetype->ag->elements, pn->path);
				return parse_error;	/* Extension out of range */
			}
			//printf("FP in range\n") ;
		}

		//printf("FP Good\n") ;
		switch (pn->selected_filetype->format) {
		case ft_directory:		// aux or main
			if (BranchAdd(pn) != 0) {
				//printf("PN BranchAdd failed for %s\n", pn->path);
				return parse_error;
			}
			/* STATISTICS */
			STATLOCK;
			if (pn->pathlength > dir_depth) {
				dir_depth = pn->pathlength;
			}
			STATUNLOCK;
			return parse_branch;
		case ft_subdir:
			pn->subdir = pn->selected_filetype;
			pn->selected_filetype = NULL;
			return parse_subprop;
		default:
			return parse_done;
		}
	}
	//printf("FP not found\n") ;
	return parse_error;			/* filetype not found */
}

static ZERO_OR_ERROR BranchAdd(struct parsedname *pn)
{
	//printf("BRANCHADD\n");
	if ((pn->pathlength % BRANCH_INCR) == 0) {
		void *temp = pn->bp;
		if ((pn->bp = owrealloc(temp, (BRANCH_INCR + pn->pathlength) * sizeof(struct buspath))) == NULL) {
			if (temp) {
				owfree(temp);
			}
			return -ENOMEM;
		}
	}
	memcpy(pn->bp[pn->pathlength].sn, pn->sn, SERIAL_NUMBER_SIZE);	/* copy over DS2409 name */
	pn->bp[pn->pathlength].branch = pn->selected_filetype->data.i;
	++pn->pathlength;
	pn->selected_filetype = NULL;
	pn->selected_device = NULL;
	return 0;
}

int filetype_cmp(const void *name, const void *ex)
{
	return strcmp((const char *) name, ((const struct filetype *) ex)->name);
}

/* Parse a path/file combination */
ZERO_OR_ERROR FS_ParsedNamePlus(const char *path, const char *file, struct parsedname *pn)
{
	ZERO_OR_ERROR ret = 0;
	char *fullpath;

	if (path == NULL) {
		path = "" ;
	}
	if (file == NULL) {
		file = "" ;
	}

	fullpath = owmalloc(strlen(file) + strlen(path) + 2);
	if (fullpath == NULL) {
		return -ENOMEM;
	}
	strcpy(fullpath, path);
	if (fullpath[strlen(fullpath) - 1] != '/') {
		strcat(fullpath, "/");
	}
	strcat(fullpath, file);
	//printf("PARSENAMEPLUS path=%s pre\n",fullpath) ;
	ret = FS_ParsedName(fullpath, pn);
	//printf("PARSENAMEPLUS path=%s post\n",fullpath) ;
	owfree(fullpath);
	//printf("PARSENAMEPLUS free\n") ;
	return ret;
}

/* Parse a path/file combination */
ZERO_OR_ERROR FS_ParsedNamePlusExt(const char *path, const char *file, int extension, enum ag_index alphanumeric, struct parsedname *pn)
{
	char name[OW_FULLNAME_MAX];
	UCLIBCLOCK;
	if (extension == -2) {
		snprintf(name, OW_FULLNAME_MAX, "%s.BYTE", file);
	} else if (extension == -1) {
		snprintf(name, OW_FULLNAME_MAX, "%s.ALL", file);
	} else if (alphanumeric == ag_letters) {
		snprintf(name, OW_FULLNAME_MAX, "%s.%c", file, extension + 'A');
	} else {
		snprintf(name, OW_FULLNAME_MAX, "%s.%d", file, extension);
	}
	UCLIBCUNLOCK;
	return FS_ParsedNamePlus(path, name, pn);
}

void FS_ParsedName_Placeholder( struct parsedname * pn )
{
	FS_ParsedName( NULL, pn ) ; // minimal parsename -- no destroy needed
}

/* For read and write sibling -- much simpler requirements */
int FS_ParseProperty_for_sibling(char *filename, struct parsedname *pn)
{
	// need to make a copy of filename since a static string can't be modified by strsep
	char *filename_copy = owstrdup(filename);
	int ret;
	if (filename_copy == NULL) {
		return 1;
	}
	ret = (Parse_Property(filename_copy, pn) == parse_done) ? 0 : 1;
	owfree(filename_copy);
	return ret;
}
