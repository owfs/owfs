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

static enum parse_enum set_type( enum ePN_type epntype, struct parsedname * pn ) ;
static enum parse_enum Parse_Unspecified(char *pathnow, enum parse_pass remote_status, struct parsedname *pn);
static enum parse_enum Parse_Branch(char *pathnow, enum parse_pass remote_status, struct parsedname *pn);
static enum parse_enum Parse_Real(char *pathnow, enum parse_pass remote_status, struct parsedname *pn);
static enum parse_enum Parse_NonReal(char *pathnow, struct parsedname *pn);
static enum parse_enum Parse_RealDevice(char *filename, enum parse_pass remote_status, struct parsedname *pn);
static enum parse_enum Parse_Property(char *filename, struct parsedname *pn);

static enum parse_enum Parse_RealDeviceSN(enum parse_pass remote_status, struct parsedname *pn);
static enum parse_enum Parse_NonRealDevice(char *filename, struct parsedname *pn);
static enum parse_enum Parse_Bus(char *pathnow, struct parsedname *pn);
static enum parse_enum Parse_Alias(char *filename, enum parse_pass remote_status, struct parsedname *pn);
static enum parse_enum Parse_Alias_Known( char *filename, enum parse_pass remote_status, struct parsedname *pn);
static void ReplaceAliasInPath( char * filename, struct parsedname * pn);

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
	SAFEFREE(pn->sparse_name);
	SAFEFREE(pn->bp) ;
	SAFEFREE(pn->path) ;
}

// Path is either NULL (in which case a minimal structure is created that doesn't need Destroy -- used for Bus Master setups)
// Or Path is a full "filename" type string of form: 10.1243Ab000 or uncached/bus.0/statistics etc.
// The Path passed in isn't altered, but 2 copies are made -- one with the full path, the other (to_server) has the first bus.n removed.
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

	RETURN_CODE_ERROR_RETURN( FS_ParsedName_setup(pp, path, pn) );

	if (path == NO_PATH) {
		RETURN_CODE_RETURN( 0 ) ; // success (by default)
	}

	while (1) {
		// Check for extreme conditions (done, error)
		switch (pe) {

		case parse_done:		// the only exit!
			//LEVEL_DEBUG("PARSENAME parse_done") ;
			//printf("PARSENAME end parse_error_status=%d\n",parse_error_status) ;
			if (parse_error_status) {
				FS_ParsedName_destroy(pn);
				return parse_error_status ;
			}

			if ( pp->pathnext != NULL ) {
				// extra text -- make this an error
				RETURN_CODE_SET_SCALAR( parse_error_status, 77 ) ; // extra text in path
				pe = parse_done;
				continue;				
			}
			
			//printf("%s: Parse %s before corrections: %.4X -- state = %d\n",(back_from_remote)?"BACK":"FORE",pn->path,pn->state,pn->type) ;
			// Play with remote levels
			switch ( pn->type ) {
				case ePN_interface:
					if ( SpecifiedVeryRemoteBus(pn) ) {
						// veryremote -> remote
						pn->state &= ~ePS_busveryremote ;
					} else if ( SpecifiedRemoteBus(pn) ) {
						// remote -> local
						pn->state &= ~ePS_busremote ;
						pn->state |= ePS_buslocal ;
					}
					break ;
					
				case ePN_root:
					// root buses are considered "real"
					pn->type = ePN_real;	// default state
					break ;
				default:
					// everything else gets promoted so directories aren't added on
					if ( SpecifiedRemoteBus(pn) ) {
						// very remote
						pn->state |= ePS_busveryremote;
					}
					break ;
			}
			//printf("%s: Parse %s after  corrections: %.4X -- state = %d\n\n",(back_from_remote)?"BACK":"FORE",pn->path,pn->state,pn->type) ;
			return 0;

		case parse_error:
			RETURN_CODE_SET_SCALAR( parse_error_status, 27 ) ; // bad path syntax
			pe = parse_done;
			continue;

		default:
			break;
		}

		// break out next name in path, make sure pp->pathnext isn't NULL. (SIGSEGV in uClibc)
		pp->pathnow = (pp->pathnext != NULL) ? strsep(&(pp->pathnext), "/") : NULL;
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
	if (pn == NO_PARSEDNAME) {
		RETURN_CODE_RETURN( 78 ); // unexpected null pointer
	}

	memset(pn, 0, sizeof(struct parsedname));
	pn->known_bus = NULL;		/* all buses */
	pn->sparse_name = NULL ;
	RETURN_CODE_INIT(pn);

	/* Set the persistent state info (temp scale, ...) -- will be overwritten by client settings in the server */
	CONTROLFLAGSLOCK;
	pn->control_flags = LocalControlFlags | SHOULD_RETURN_BUS_LIST;	// initial flag as the bus-returning level, will change if a bus is specified
	CONTROLFLAGSUNLOCK;

	// initialization
	pp->pathnow = NO_PATH;
	pp->pathlast = NO_PATH;
	pp->pathnext = NO_PATH;

	/* Default attributes */
	pn->state = ePS_normal;
	pn->type = ePN_root;

	/* uncached attribute */
	if ( Globals.uncached ) {
		// local settings (--uncached) can set
		pn->state |= ePS_uncached;
	}
	// Also can be set by path ("/uncached")
	// Also in owserver, can be set by client flags
	// sibling inherits parent
	
	/* unaliased attribute */
	if ( Globals.unaliased ) {
		// local settings (--unalaised) can set
		pn->state |= ePS_unaliased;
	}
	// Also can be set by path ("/unaliased")
	// Also in owserver, can be set by client flags
	// sibling inherits parent

	/* No device lock yet assigned */
	pn->lock = NULL ;

	/* minimal structure for initial bus "detect" use -- really has connection and LocalControlFlags only */
	pn->dirlength = -1 ;
	if (path == NO_PATH) {
		return 0; // success
	}

	if (strlen(path) > PATH_MAX) {
		RETURN_CODE_RETURN( 26 ) ; // path too long
	}

	pn->path = (char *) owmalloc(2 * PATH_MAX + 4);
	if (pn->path == NO_PATH) {
		RETURN_CODE_RETURN( 79 ) ; // unable to allocate memory
	}

	/* Have to save pn->path at once */
	strcpy(pn->path, "/"); // initial slash
	strcpy(pn->path+1, path[0]=='/'?path+1:path);
	pn->path_to_server = pn->path + strlen(pn->path) + 1;
	strcpy(pn->path_to_server, pn->path);

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
	pn->selected_connection = NO_CONNECTION ; // Default bus assignment

	return 0 ; // success
}

/* Used for virtual directories like settings and statistics
 * If local, applies to all local (this program) and not a
 * specific local bus.
 * If remote, pass it on for the remote to handle
 * */
static enum parse_enum set_type( enum ePN_type epntype, struct parsedname * pn )
{
	if (SpecifiedLocalBus(pn)) {
		return parse_error;
	} else if ( ! SpecifiedRemoteBus(pn) ) {
		pn->type |= ePS_busanylocal;
	}
	pn->type = epntype;
	return parse_nonreal;
}
	

// Early parsing -- only bus entries, uncached and text may have preceeded
static enum parse_enum Parse_Unspecified(char *pathnow, enum parse_pass remote_status, struct parsedname *pn)
{
	if (strncasecmp(pathnow, "bus.", 4) == 0) {
		return Parse_Bus(pathnow, pn);

	} else if (strcasecmp(pathnow, "settings") == 0) {
		return set_type( ePN_settings, pn ) ;

	} else if (strcasecmp(pathnow, "statistics") == 0) {
		return set_type( ePN_statistics, pn ) ;

	} else if (strcasecmp(pathnow, "structure") == 0) {
		return set_type( ePN_structure, pn ) ;

	} else if (strcasecmp(pathnow, "system") == 0) {
		return set_type( ePN_system, pn ) ;

	} else if (strcasecmp(pathnow, "interface") == 0) {
		if (!SpecifiedBus(pn)) {
			return parse_error;
		}
		pn->type = ePN_interface;
		return parse_nonreal;

	} else if (strcasecmp(pathnow, "text") == 0) {
		pn->state |= ePS_json;
		return parse_first;

	} else if (strcasecmp(pathnow, "json") == 0) {
		pn->state |= ePS_text;
		return parse_first;

	} else if (strcasecmp(pathnow, "uncached") == 0) {
		pn->state |= ePS_uncached;
		return parse_first;

	} else if (strcasecmp(pathnow, "unaliased") == 0) {
		pn->state |= ePS_unaliased;
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
		LEVEL_DEBUG("TEST set simultaneous");
		pn->selected_device = DeviceSimultaneous;
		return parse_prop;

	} else if (strcasecmp(pathnow, "text") == 0) {
		pn->state |= ePS_text;
		return parse_real;

	} else if (strcasecmp(pathnow, "json") == 0) {
		pn->state |= ePS_json;
		return parse_real;

	} else if (strcasecmp(pathnow, "thermostat") == 0) {
		pn->selected_device = DeviceThermostat;
		return parse_prop;

	} else if (strcasecmp(pathnow, "uncached") == 0) {
		pn->state |= ePS_uncached;
		return parse_real;

	} else if (strcasecmp(pathnow, "unaliased") == 0) {
		pn->state |= ePS_unaliased;
		return parse_real;

	} else {
		return Parse_RealDevice(pathnow, remote_status, pn);
	}
}

static enum parse_enum Parse_NonReal(char *pathnow, struct parsedname *pn)
{
	if (strcasecmp(pathnow, "text") == 0) {
		pn->state |= ePS_text;
		return parse_nonreal;

	} else if (strcasecmp(pathnow, "json") == 0) {
		pn->state |= ePS_json;
		return parse_nonreal;

	} else if (strcasecmp(pathnow, "uncached") == 0) {
		pn->state |= ePS_uncached;
		return parse_nonreal;

	} else if (strcasecmp(pathnow, "unaliased") == 0) {
		pn->state |= ePS_unaliased;
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
	INDEX_OR_ERROR bus_number;
	/* Processing for bus.X directories -- eventually will make this more generic */
	if (!isdigit(pathnow[4])) {
		return parse_error;
	}

	bus_number = atoi(&pathnow[4]);
	if ( INDEX_NOT_VALID(bus_number) ) {
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

	/* Create the path without the "bus.x" part in pn->path_to_server */
	if ( (found = strstr(pn->path, "/bus.")) ) {
		int length = found - pn->path;
		if ((found = strchr(found + 1, '/'))) {	// more after bus
			strcpy(&(pn->path_to_server[length]), found);	// copy rest
		} else {
			pn->path_to_server[length] = '\0';	// add final null
		}
	}
	return parse_first;
}

/* replace alias with sn */
static void ReplaceAliasInPath( char * filename, struct parsedname * pn)
{
	int alias_len = strlen(filename) ;

	char alias[alias_len + 2] ;

	char * alias_loc ;
	char * post_alias_loc ;

	// check total length
	if ( strlen(pn->path_to_server) + 14 - alias_len > PATH_MAX ) {
		return ;
	}
	
	strcpy( alias, "/") ;
	strcat( alias, filename ) ; // we include an initial / to make the match more accurate

	for ( alias_loc = pn->path_to_server; alias_loc != NULL ; alias_loc = strstr( alias_loc, alias ) ) {
		++alias_loc ; // point after '/'

		post_alias_loc = alias_loc + alias_len ;
		switch ( post_alias_loc[0] ) {
			case '\0':
			case '/':
				// move rest of path
				memmove( &alias_loc[14], post_alias_loc, strlen(post_alias_loc)+1 ) ;
				//write in serial number for alias
				bytes2string( alias_loc, pn->sn, 7 ) ;
				return ;
			default:
				// alias not really found (only partial match)
				// loop starting one char later
				break ;
		}
	}
}

/* This is when the alias name in mapped to a known serial number
 * behaves much more like the standard handling -- bus from sn */
static enum parse_enum Parse_Alias_Known( char *filename, enum parse_pass remote_status, struct parsedname *pn)
{
	if (remote_status == parse_pass_pre_remote) {
		ReplaceAliasInPath( filename, pn ) ;
	}

	return Parse_RealDeviceSN( remote_status, pn ) ;
}

/* Get a device that isn't a serial number -- see if it's an alias */
static enum parse_enum Parse_Alias(char *filename, enum parse_pass remote_status, struct parsedname *pn)
{
	INDEX_OR_ERROR bus ;
	
	// See if the alias is known in the permanent list. We get the serial number 
	if ( GOOD( Cache_Get_Alias_SN(filename,pn->sn)) ) {
		// Success! The alias is already registered and the serial
		//  number just now loaded in pn->sn
		return Parse_Alias_Known( filename, remote_status, pn ) ;
	}

	// By definition this is a remote device, or non-existent.
	pn->selected_device = &RemoteDevice ;

	// is alias name cached from previous query?
	bus = Cache_Get_Alias_Bus( filename ) ;
	if ( bus != INDEX_BAD ) {
		// This alias is cached in temporary list
		SetKnownBus(bus, pn);
		return parse_prop ;
	}

	// Look for alias in remote buses
	bus = RemoteAlias(pn) ;
	if ( bus == INDEX_BAD ) {
		return parse_error ;
	}
	
	// Found the alias (remotely)
	SetKnownBus(bus, pn);

	if ( pn->sn[0] == 0 && pn->sn[7]==0 ) { // no serial number owserver (older)
		Cache_Add_Alias_Bus(filename,bus) ;
	} else {
		Cache_Add_Alias( filename, pn->sn ) ;
		Cache_Add_Device( bus, pn->sn ) ;
		pn->selected_device = FS_devicefindhex(pn->sn[0], pn);
	}

	return parse_prop ;
}

/* Parse Name (only device name) part of string */
/* Return -ENOENT if not a valid name
   return 0 if good
   *next points to next segment, or NULL if not filetype
 */
static enum parse_enum Parse_RealDevice(char *filename, enum parse_pass remote_status, struct parsedname *pn)
{
	switch ( Parse_SerialNumber(filename,pn->sn) ) {
		case sn_alias:
			return Parse_Alias( filename, remote_status, pn) ;
		case sn_valid:
			return Parse_RealDeviceSN( remote_status, pn ) ;
		case sn_invalid:
		default:
			return parse_error ;
	}
}

/* Device is known with serial number */
static enum parse_enum Parse_RealDeviceSN(enum parse_pass remote_status, struct parsedname *pn)
{
	/* Search for known 1-wire device -- keyed to device name (family code in HEX) */
	pn->selected_device = FS_devicefindhex(pn->sn[0], pn);

	// returning from owserver -- don't need to check presence (it's implied)
	if (remote_status == parse_pass_post_remote) {
		return parse_prop;
	}

	if (Globals.one_device) {
		SetKnownBus(INDEX_DEFAULT, pn);
	} else {
		/* Check the presence, and cache the proper bus number for better performance */
		INDEX_OR_ERROR bus_nr = CheckPresence(pn);
		if ( INDEX_NOT_VALID(bus_nr) ) {
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
	return (pn->selected_device == &UnknownDevice) ? parse_error : parse_prop;
}

static enum parse_enum Parse_Property(char *filename, struct parsedname *pn)
{
	char *dot = filename;

	//printf("FilePart: %s %s\n", filename, pn->path);

	// Special case for remote device. Use distant data
	if ( pn->selected_device == &RemoteDevice ) {
		// remote device, no known sn, can't handle a prolerty
		return parse_error ;
	}

	// separate filename.dot
	filename = strsep(&dot, ".");
	//printf("FP name=%s, dot=%s\n", filename, dot);

	/* Match to known filetypes for this device */
	pn->selected_filetype =
		 bsearch(filename, pn->selected_device->filetype_array,
				 (size_t) pn->selected_device->count_of_filetypes, sizeof(struct filetype), filetype_cmp) ;
				 
	if ( pn->selected_filetype == NO_FILETYPE ) {
		LEVEL_DEBUG("Unknown property fo this device %s",SAFESTRING(filename) ) ;
		return parse_error;			/* filetype not found */
	}
		
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

	} else if (pn->selected_filetype->ag->combined==ag_sparse)  { /* Sparse */
		if (pn->selected_filetype->ag->letters == ag_letters) {	/* text string */
			pn->extension = 0;	/* text extension, not number */
			pn->sparse_name = owstrdup(dot) ;
		} else {			/* Numbers */
			char *p;
			//printf("FP numbers\n") ;
			pn->extension = strtol(dot, &p, 0);	/* Number conversion */
			if ((p == dot) || ((pn->extension == 0) && (errno == -EINVAL))) {
				return parse_error;	/* Bad number */
			}
		}
		
	
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
		if ( pn->type == ePN_structure ) {
			// special case, structure for aux and main
			return parse_done;
		}
		if (BranchAdd(pn) != 0) {
			//printf("PN BranchAdd failed for %s\n", filename);
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
		//printf("PN %s is a subdirectory\n", filename);
		pn->subdir = pn->selected_filetype;
		pn->selected_filetype = NO_FILETYPE;
		return parse_subprop;
	default:
		return parse_done;
	}
}

static ZERO_OR_ERROR BranchAdd(struct parsedname *pn)
{
	//printf("BRANCHADD\n");
	if ((pn->pathlength % BRANCH_INCR) == 0) {
		void *temp = pn->bp;
		if ((pn->bp = owrealloc(temp, (BRANCH_INCR + pn->pathlength) * sizeof(struct buspath))) == NULL) {
			SAFEFREE(temp) ;
			RETURN_CODE_RETURN( 79 ) ; // unable to allocate memory
		}
	}
	memcpy(pn->bp[pn->pathlength].sn, pn->sn, SERIAL_NUMBER_SIZE);	/* copy over DS2409 name */
	pn->bp[pn->pathlength].branch = pn->selected_filetype->data.i;
	++pn->pathlength;
	pn->selected_filetype = NO_FILETYPE;
	pn->selected_device = NO_DEVICE;
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

	if (path == NO_PATH) {
		path = "" ;
	}
	if (file == NULL) {
		file = "" ;
	}

	fullpath = owmalloc(strlen(file) + strlen(path) + 2);
	if (fullpath == NO_PATH) {
		RETURN_CODE_RETURN( 79 ) ; // unable to allocate memory
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
	if (extension == EXTENSION_BYTE ) {
		return FS_ParsedNamePlusText(path, file, "BYTE", pn);
	} else if (extension == EXTENSION_ALL ) {
		return FS_ParsedNamePlusText(path, file, "ALL", pn);
	} else if (alphanumeric == ag_letters) {
		char name[2] = { 'A'+extension, 0x00, } ;
		return FS_ParsedNamePlusText(path, file, name, pn);
	} else {
		char name[OW_FULLNAME_MAX];
		UCLIBCLOCK;
		snprintf(name, OW_FULLNAME_MAX, "%d", extension);
		UCLIBCUNLOCK;
		return FS_ParsedNamePlusText(path, file, name, pn);
	}
}

/* Parse a path/file combination */
ZERO_OR_ERROR FS_ParsedNamePlusText(const char *path, const char *file, const char *extension, struct parsedname *pn)
{
	char name[OW_FULLNAME_MAX];
	UCLIBCLOCK;
		snprintf(name, OW_FULLNAME_MAX, "%s.%s", file, extension );
	UCLIBCUNLOCK;
	return FS_ParsedNamePlus(path, name, pn);
}

void FS_ParsedName_Placeholder( struct parsedname * pn )
{
	FS_ParsedName( NULL, pn ) ; // minimal parsename -- no destroy needed
}
