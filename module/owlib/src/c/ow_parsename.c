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

static int BranchAdd(struct parsedname *pn);

enum parse_enum { parse_first, parse_done, parse_error, parse_real,
	parse_nonreal, parse_prop, parse_subprop
};

struct parsedname_pointers {
	char * pathcpy ;
	char * pathnow ;
	char * pathnext ;
	char * pathlast ;
} ;


static enum parse_enum Parse_Unspecified(char *pathnow, int back_from_remote, struct parsedname *pn);
static enum parse_enum Parse_Real(char *pathnow, int back_from_remote, struct parsedname *pn);
static enum parse_enum Parse_NonReal(char *pathnow, struct parsedname *pn);
static enum parse_enum Parse_RealDevice(char *filename, int back_from_remote, struct parsedname *pn);
static enum parse_enum Parse_NonRealDevice(char *filename, struct parsedname *pn);
static enum parse_enum Parse_Property(char *filename, struct parsedname *pn);
static enum parse_enum Parse_Bus(char *pathnow, int back_from_remote, struct parsedname *pn);
static int FS_ParsedName_anywhere(const char *path, int back_from_remote, struct parsedname *pn);
static int FS_ParsedName_setup(struct parsedname_pointers * pp, const char *path, struct parsedname *pn) ;

#define BRANCH_INCR (9)

/* ---------------------------------------------- */
/* Filename (path) parsing functions              */
/* ---------------------------------------------- */
void FS_ParsedName_destroy(struct parsedname *pn)
{
	if (!pn)
		return;
	LEVEL_DEBUG("ParsedName_destroy %s\n", SAFESTRING(pn->path));
	//printf("PNDestroy bp (%d)\n",BusIsServer(pn->selected_connection)) ;
	if (pn->bp) {
		free(pn->bp);
		pn->bp = NULL;
	}
	//printf("PNDestroy path (%d)\n",BusIsServer(pn->selected_connection)) ;
	if (pn->path) {
		free(pn->path);
		pn->path = NULL;
	}
	//printf("PNDestroy lock (%d)\n",BusIsServer(pn->selected_connection)) ;
	if (pn->lock) {
		free(pn->lock);
		pn->lock = NULL;
	}
	//printf("PNDestroy done (%d)\n",BusIsServer(pn->selected_connection)) ;
	// Tokenstring is part of a larger allocation and destroyed separately 
}

/* Parse a path to check it's validity and attach to the propery data structures */
int FS_ParsedName(const char *path, struct parsedname *pn)
{
	return FS_ParsedName_anywhere(path, 0, pn);
}

/* Parse a path from a remote source back -- so don't check presence */
int FS_ParsedName_BackFromRemote(const char *path, struct parsedname *pn)
{
	return FS_ParsedName_anywhere(path, 1, pn);
}

/* Parse off starting "mode" directory (uncached, alarm...) */
static int FS_ParsedName_anywhere(const char *path, int back_from_remote,
								  struct parsedname *pn)
{
	struct parsedname_pointers s_pp ;
	struct parsedname_pointers * pp = &s_pp ;
	int ret = 0;
	enum parse_enum pe = parse_first;

	// To make the debug output useful it's cleared here.
	// Even on normal glibc, errno isn't cleared on good system calls
	errno = 0;

	LEVEL_CALL("PARSENAME path=[%s]\n", SAFESTRING(path));
    
	ret = FS_ParsedName_setup(pp,path,pn) ;
	if ( ret ) return ret ;
	//printf("1pathnow=[%s] pathnext=[%s] pn->type=%d\n", pathnow, pathnext, pn->type);

	if(path == NULL) return 0;

	while (1) {
	//printf("PARSENAME parse_enum=%d pathnow=%s\n",pe,SAFESTRING(pp->pathnow));
        // Check for extreme conditions (done, error)
		switch (pe) {
        
		case parse_done: // the only exit!
			//LEVEL_DEBUG("PARSENAME parse_done\n") ;
			//printf("PARSENAME end ret=%d\n",ret) ;
			if ( pp->pathcpy) free(pp->pathcpy);
			if (ret) {
				FS_ParsedName_destroy(pn);
			} else {
				//printf("%s: Parse %s before corrections: %.4X -- state = %d\n",(back_from_remote)?"BACK":"FORE",pn->path,pn->state,pn->type) ;
				if ( SpecifiedRemoteBus(pn) && ! SpecifiedVeryRemoteBus(pn) ) {
					// promote interface of remote bus to local
					if ( pn->type == ePN_interface ) {
						pn->state &= ~ePS_busremote ;
						pn->state |= ePS_buslocal ;
	
					// non-root remote bus
					} else if ( pn->type != ePN_root ) {
						pn->state |= ePS_busveryremote ;
					}
				}
		
				// root busses are considered "real"
				if ( pn->type == ePN_root ) {
					pn->type = ePN_real ; // default state
				}
			}
			//printf("%s: Parse %s after  corrections: %.4X -- state = %d\n\n",(back_from_remote)?"BACK":"FORE",pn->path,pn->state,pn->type) ;
			return ret ;
		
		case parse_error:
			//LEVEL_DEBUG("PARSENAME parse_error\n") ;
			ret = -ENOENT;
			pe = parse_done ;
			continue ;
		
		default:
			break;
		}

		// break out next name in path, make sure pp->pathnext isn't NULL. (SIGSEGV in uClibc)
		if(pp->pathnext)
		  pp->pathnow = strsep(&(pp->pathnext), "/");
		else
		  pp->pathnow = NULL;
		//LEVEL_DEBUG("PARSENAME pathnow=[%s] rest=[%s]\n",pathnow,pathnext) ;
		if (pp->pathnow == NULL || pp->pathnow[0] == '\0') {
			pe = parse_done ;
			continue ;
		}
		
		// rest of state machine on parsename
		switch (pe) {
		
		case parse_first:
			//LEVEL_DEBUG("PARSENAME parse_first\n") ;
			pe = Parse_Unspecified(pp->pathnow, back_from_remote, pn);
			break;
		
		case parse_real:
			//LEVEL_DEBUG("PARSENAME parse_real\n") ;
			pe = Parse_Real(pp->pathnow, back_from_remote, pn);
			break;
		
		case parse_nonreal:
			//LEVEL_DEBUG("PARSENAME parse_nonreal\n") ;
			pe = Parse_NonReal(pp->pathnow, pn);
			break;
		
		case parse_prop:
			//LEVEL_DEBUG("PARSENAME parse_prop\n") ;
			pp->pathlast = pp->pathnow;	/* Save for concatination if subdirectory later wanted */
			pe = Parse_Property(pp->pathnow, pn);
			break;
		
		case parse_subprop:
			//LEVEL_DEBUG("PARSENAME parse_subprop\n") ;
			pp->pathnow[-1] = '/';
			pe = Parse_Property(pp->pathlast, pn);
			break;
		
		default:
		pe = parse_error ; // unknown state
		break;

		}
	//printf("PARSENAME pe=%d\n",pe) ;
	}
}

/* Initial memory allocation and pn setup */
static int FS_ParsedName_setup(struct parsedname_pointers *pp, const char *path, struct parsedname *pn)
{
	if (pn == NULL)
		return -EINVAL;

	memset(pn, 0, sizeof(struct parsedname));
	pn->known_bus = NULL;			/* all busses */

	/* Set the persistent state info (temp scale, ...) -- will be overwritten by client settings in the server */
	pn->sg = SemiGlobal | (1 << BUSRET_BIT);	// initial flag as the bus-returning level, will change if a bus is specified

	// initialization
	pp->pathnow = NULL ;
	pp->pathlast = NULL ;
	pp->pathcpy = NULL ;
	pp->pathnext = NULL ;

	/* minimal structure for setup use */
	if (path == NULL)
		return 0;

	/* Default attributes */
	pn->state = ePS_normal;
	pn->type = ePN_root;

	/* make a copy for destructive parsing */
	pp->pathcpy = strdup(path);
	if ( pp->pathcpy==NULL ) {
		return -ENOMEM ;
	}

	pn->path = (char *) malloc(2 * strlen(path) + 2);
	if ( pn->path == NULL ) {
		free( pp->pathcpy ) ;
		return -ENOMEM ;
	}

	/* connection_in list and start */
	CONNINLOCK;
	pn->head_inbound_list = head_inbound_list;
	pn->lock = calloc(count_inbound_connections, sizeof(struct devlock *));
	CONNINUNLOCK;
	
	if ( pn->head_inbound_list == NULL ) {
		free( pp->pathcpy ) ;
		free( pn->path ) ;
		return -ENOENT ;
	}

	if ( pn->lock == NULL ) {
		free( pp->pathcpy ) ;
		free( pn->path ) ;
		return -ENOMEM ;
	}

	pn->selected_connection = pn->head_inbound_list;

	/* Have to save pn->path at once */
	strcpy(pn->path, path);
	pn->path_busless = pn->path + strlen(path) + 1;
	strcpy(pn->path_busless, path);

	/* pointer to rest of path after current token peeled off */
	pp->pathnext = pp->pathcpy;

	/* remove initial "/" */
	if (pp->pathnext[0] == '/')
		++pp->pathnext;

    pn->terminal_bus_number = -1; 
            
	return 0 ;
}

// Early parsing -- only bus entries, uncached and text may have preceeded
static enum parse_enum Parse_Unspecified(char *pathnow,
                                         int back_from_remote,
                                         struct parsedname *pn)
{
    if (strcasecmp(pathnow, "alarm") == 0) {
        pn->state |= ePS_alarm;
        pn->type = ePN_real;
        return parse_real;

    } else if (strncasecmp(pathnow, "bus.", 4) == 0) {
        return Parse_Bus(pathnow, back_from_remote, pn);

    } else if (strcasecmp(pathnow, "settings") == 0) {
        if ( SpecifiedLocalBus(pn) ) return parse_error ;
        pn->type = ePN_settings;
        return parse_nonreal;

    } else if (strcasecmp(pathnow, "simultaneous") == 0) {
        pn->selected_device = DeviceSimultaneous;
        return parse_prop;

    } else if (strcasecmp(pathnow, "statistics") == 0) {
        if ( SpecifiedLocalBus(pn) ) return parse_error ;
        pn->type = ePN_statistics;
        return parse_nonreal;

    } else if (strcasecmp(pathnow, "structure") == 0) {
        if ( SpecifiedLocalBus(pn) ) return parse_error ;
        pn->type = ePN_structure;
        return parse_nonreal;

    } else if (strcasecmp(pathnow, "system") == 0) {
        if ( SpecifiedLocalBus(pn) ) return parse_error ;
        pn->type = ePN_system;
        return parse_nonreal;

    } else if (strcasecmp(pathnow, "interface") == 0) {
        if ( ! SpecifiedBus(pn) ) return parse_error ;
        pn->type = ePN_interface;
        return parse_nonreal;

    } else if (strcasecmp(pathnow, "text") == 0) {
        pn->state |= ePS_text;
        return parse_first;

    } else if (strcasecmp(pathnow, "thermostat") == 0) {
        pn->selected_device = DeviceThermostat;
        pn->type = ePN_real;
        return parse_prop;

    } else if (strcasecmp(pathnow, "uncached") == 0) {
        pn->state |= ePS_uncached;
        return parse_first;

    } else {
        pn->type = ePN_real;
        return Parse_RealDevice(pathnow, back_from_remote, pn);
    }
}

static enum parse_enum Parse_Real(char *pathnow, int back_from_remote,
								  struct parsedname *pn)
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
		return Parse_RealDevice(pathnow, back_from_remote, pn);
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
static enum parse_enum Parse_Bus(char *pathnow, int back_from_remote, struct parsedname *pn)
{
    char *found;
    int bus_number;

    /* Processing for bus.X directories -- eventually will make this more generic */
    if (!isdigit(pathnow[4])) {
        return parse_error;
    }

    bus_number = atoi(&pathnow[4]);
    if (bus_number < 0) return parse_error ;
    pn->terminal_bus_number = bus_number ;

    /* Should make a presence check on remote busses here, but
    * it's not a major problem if people use bad paths since
    * they will just end up with empty directory listings. */
    if (SpecifiedLocalBus(pn)) {     /* already specified a "bus." */
        /* too many levels of bus for a non-remote adapter */
        return parse_error;
    } else if (SpecifiedRemoteBus(pn)) {     /* already specified a "bus." */
        /* Let the remote bus do the heavy listing */
		pn->state |= ePS_busveryremote ;
        return parse_first ;
    }

    /* on return from remote directory ow_server.c:ServerDir
    the SetKnownBus will be be performed else where since the sending bus number is used */
    /* this will only be reached once, because a local bus.x triggers "SpecifiedBus" */
    //printf("SPECIFIED BUS for ParsedName PRE (%d):\n\tpath=%s\n\tpath_busless=%s\n\tKnnownBus=%d\tSpecifiedBus=%d\n",bus_number,   SAFESTRING(pn->path),SAFESTRING(pn->path_busless),KnownBus(pn),SpecifiedBus(pn));
    CONNINLOCK;
    if (count_inbound_connections <= bus_number) bus_number = -1 ;
    CONNINUNLOCK;
    if (bus_number < 0) return parse_error ;

    /* Since we are going to use a specific in-device now, set
    * pn->selected_connection to point at that device at once. */
    SetSpecifiedBus(bus_number, pn);

    // return trip, so bus_pathless not needed.
    if (back_from_remote) {
        return parse_first ;
    }

    if (SpecifiedLocalBus(pn)) {
        /* don't return bus-list for local paths. */
        pn->sg &= (~BUSRET_MASK);
    }

    /* Create the path without the "bus.x" part in pn->path_busless */
    if (!(found = strstr(pn->path, "/bus."))) { // no bus
        int length = pn->path_busless - pn->path - 1;
        strncpy(pn->path_busless, pn->path, length); // just copy path
    } else {
        int length = found - pn->path;
        strncpy(pn->path_busless, pn->path, length);
        if ((found = strchr(found + 1, '/'))) { // more after bus
            strcpy(&(pn->path_busless[length]), found); // copy rest
        } else {
            pn->path_busless[length] = '\0'; // add final null
        }
    }
    //printf("SPECIFIED BUS for ParsedName POST (%d):\n\tpath=%s\n\tpath_busless=%s\n\tKnnownBus=%d\tSpecifiedBus=%d\n",bus_number,SAFESTRING(pn->path),SAFESTRING(pn->path_busless),KnownBus(pn),SpecifiedBus(pn));
    //LEVEL_DEBUG("PARSENAME test path=%s, path_busless=%s\n",pn->path, pn->path_busless ) ;
    return parse_first;
}

/* Parse Name (only device name) part of string */
/* Return -ENOENT if not a valid name
   return 0 if good
   *next points to next segment, or NULL if not filetype
 */
static enum parse_enum Parse_RealDevice(char *filename,
										int back_from_remote,
										struct parsedname *pn)
{
	//printf("DevicePart: %s %s\n", filename, pn->path);

	ASCII ID[14];
    int bus_nr ;
    int i;
	//printf("NP hex = %s\n",filename ) ;
	//printf("NP cmp'ed %s\n",ID ) ;
	for (i = 0; i < 14; ++i, ++filename) {	/* get ID number */
		if (*filename == '.')
			++filename;
		if (isxdigit(*filename)) {
			ID[i] = *filename;
		} else {
			return parse_error;
		}
	}
	//printf("NP0\n");
	pn->sn[0] = string2num(&ID[0]);
	pn->sn[1] = string2num(&ID[2]);
	pn->sn[2] = string2num(&ID[4]);
	pn->sn[3] = string2num(&ID[6]);
	pn->sn[4] = string2num(&ID[8]);
	pn->sn[5] = string2num(&ID[10]);
	pn->sn[6] = string2num(&ID[12]);
	pn->sn[7] = CRC8compute(pn->sn, 7, 0);
	if (*filename == '.')
		++filename;
	//printf("NP1\n");
	if (isxdigit(filename[0]) && isxdigit(filename[1])) {
		char crc[2];
		num2string(crc, pn->sn[7]);
		if (strncasecmp(crc, filename, 2))
			return parse_error;
	}
	/* Search for known 1-wire device -- keyed to device name (family code in HEX) */
	FS_devicefindhex(pn->sn[0], pn);

    if ( back_from_remote ) return parse_prop ;

    if( Global.one_device ) {
        bus_nr = 0 ; // arbitrary assignment
    } else {
        /* Check the presence, and cache the proper bus number for better performance */
        bus_nr = CheckPresence(pn) ;
        if (bus_nr < 0) {
            return parse_error; /* CheckPresence failed */
        }
    }
	
    return parse_prop;
}

/* Parse Name (non-device name) part of string */
/* Return -ENOENT if not a valid name
   return 0 if good
*/
static enum parse_enum Parse_NonRealDevice(char *filename,
										   struct parsedname *pn)
{
	//printf("Parse_NonRealDevice: [%s] [%s]\n", filename, pn->path);
	FS_devicefind(filename, pn);
	return (pn->selected_device == &NoDevice) ? parse_error : parse_prop;
}

static enum parse_enum Parse_Property(char *filename,
									  struct parsedname *pn)
{
	char *dot = filename;

	//printf("FilePart: %s %s\n", filename, pn->path);

	filename = strsep(&dot, ".");
	//printf("FP name=%s, dot=%s\n", filename, dot);
	/* Match to known filetypes for this device */
	if ((pn->selected_filetype =
		 bsearch(filename, pn->selected_device->filetype_array, (size_t) pn->selected_device->count_of_filetypes,
				 sizeof(struct filetype), filecmp))) {
		//printf("FP known filetype %s\n",pn->selected_filetype->name) ;
		/* Filetype found, now process extension */
		if (dot == NULL || dot[0] == '\0') {	/* no extension */
			if (pn->selected_filetype->ag)
				return parse_error;	/* aggregate filetypes need an extension */
			pn->extension = 0;	/* default when no aggregate */

		} else if (pn->selected_filetype->ag == NULL) {
			return parse_error;	/* An extension not allowed when non-aggregate */

		} else if (strcasecmp(dot, "ALL") == 0) {
			//printf("FP ALL\n");
			pn->extension = EXTENSION_ALL;	/* ALL */

		} else if (pn->selected_filetype->format == ft_bitfield
				   && strcasecmp(dot, "BYTE") == 0) {
			pn->extension = EXTENSION_BYTE;	/* BYTE */
			//printf("FP BYTE\n") ;

		} else { /* specific extension */
			if (pn->selected_filetype->ag->letters == ag_letters) { /* Letters */
				//printf("FP letters\n") ;
				if ((strlen(dot) != 1) || !isupper(dot[0]))
					return parse_error;
				pn->extension = dot[0] - 'A';	/* Letter extension */
			} else {			/* Numbers */
				char *p;
				//printf("FP numbers\n") ;
				pn->extension = strtol(dot, &p, 0);	/* Number conversion */
				if ((p == dot)
					|| ((pn->extension == 0) && (errno == -EINVAL)))
					return parse_error;	/* Bad number */
			}
			//printf("FP ext=%d nr_elements=%d\n", pn->extension, pn->selected_filetype->ag->elements) ;
			/* Now check range */
			if ((pn->extension < 0) || (pn->extension >= pn->selected_filetype->ag->elements)) {
				//printf("FP Extension out of range %d %d %s\n", pn->extension, pn->selected_filetype->ag->elements, pn->path);
				return parse_error;	/* Extension out of range */
			}
			//printf("FP in range\n") ;
		}

		//printf("FP Good\n") ;
		switch (pn->selected_filetype->format) {
            case ft_directory: // aux or main
			if (BranchAdd(pn)) {
				//printf("PN BranchAdd failed for %s\n", pn->path);
				return parse_error;
			}
			/* STATISTICS */
			STATLOCK;
			if (pn->pathlength > dir_depth)
				dir_depth = pn->pathlength;
			STATUNLOCK;
			return parse_real;
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

static int BranchAdd(struct parsedname *pn)
{
	//printf("BRANCHADD\n");
	if ((pn->pathlength % BRANCH_INCR) == 0) {
		void *temp = pn->bp;
		if ((pn->bp =
			 realloc(temp,
					 (BRANCH_INCR +
					  pn->pathlength) * sizeof(struct buspath))) == NULL) {
			if (temp)
				free(temp);
			return -ENOMEM;
		}
	}
	memcpy(pn->bp[pn->pathlength].sn, pn->sn, 8);	/* copy over DS2409 name */
	pn->bp[pn->pathlength].branch = pn->selected_filetype->data.i;
	++pn->pathlength;
	pn->selected_filetype = NULL;
	pn->selected_device = NULL;
	return 0;
}

int filecmp(const void *name, const void *ex)
{
	return strcmp((const char *) name,
				  ((const struct filetype *) ex)->name);
}

/* Parse a path/file combination */
int FS_ParsedNamePlus(const char *path, const char *file,
					  struct parsedname *pn)
{
	if (path == NULL || path[0] == '\0') {
		return FS_ParsedName(file, pn);
	} else if (file == NULL || file[0] == '\0') {
		return FS_ParsedName(path, pn);
	} else {
		int ret = 0;
		char *fullpath;
		fullpath = malloc(strlen(file) + strlen(path) + 2);
		if (fullpath == NULL)
			return -ENOMEM;
		strcpy(fullpath, path);
		if (fullpath[strlen(fullpath) - 1] != '/')
			strcat(fullpath, "/");
		strcat(fullpath, file);
		//printf("PARSENAMEPLUS path=%s pre\n",fullpath) ;
		ret = FS_ParsedName(fullpath, pn);
		//printf("PARSENAMEPLUS path=%s post\n",fullpath) ;
		free(fullpath);
		//printf("PARSENAMEPLUS free\n") ;
		return ret;
	}
}

/* For read and write sibling -- much simpler requirements */
int FS_ParseProperty_for_sibling( char *filename, struct parsedname *pn)
{
    // need to make a copy of filename since a static string can't be modified by strsep
    char * filename_copy = strdup( filename ) ;
    int ret ;
    if ( filename_copy == NULL ) return 1 ;
    ret = ( Parse_Property( filename_copy, pn ) == parse_done ) ? 0 : 1 ;
    free( filename_copy ) ;
    return ret ;
}
