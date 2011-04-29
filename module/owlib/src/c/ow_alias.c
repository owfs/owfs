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

/* ow_opt -- owlib specific command line options processing */

#define _GNU_SOURCE
#include <stdio.h> // for getline
#undef _GNU_SOURCE

#include <config.h>
#include "owfs_config.h"
#include "ow.h"

GOOD_OR_BAD ReadAliasFile(const ASCII * file)
{
	FILE *alias_file_pointer ;

	/* getline parameters. allocated and reallocated by getline. use free rather than owfree */
	char * alias_line = NULL ;
	size_t alias_line_length ;
	int line_number = 0;

	/* try to open file for reading */
	alias_file_pointer = fopen(file, "r");
	if ( alias_file_pointer == NULL ) {
		ERROR_DEFAULT("Cannot process alias file %s", file);
		return gbBAD;
	}

	/* read each line and parse */
	while (getline(&alias_line, &alias_line_length, alias_file_pointer) >= 0) {
		++line_number;
		BYTE sn[SERIAL_NUMBER_SIZE] ;
		char * a_line = alias_line ; // pointer within the line
		char * sn_char = NULL ; // pointer to serial number
		char * name_char = NULL ; // pointer to alias name

		while ( a_line ) {
			sn_char = strsep( &a_line, "/ \t=\n");
			if ( strlen(sn_char)>0 ) {
				// non delim char
				break ;
			}
		}

		if ( Parse_SerialNumber(sn_char, sn) != sn_valid ) {
			LEVEL_CALL("Problem parsing device name in alias file %s:%d",file,line_number) ;
			continue ;
		}

		if ( a_line ) {
			a_line += strspn(a_line," \t=") ;
		}
		while ( a_line ) {
			name_char = strsep( &a_line, "\n");
			size_t len = strlen(name_char) ;
			if ( len > 0 ) {
				while ( len>0 ) {
					if ( name_char[len-1] != ' ' && name_char[len-1] != '\t' ) {
						break ;
					}
					name_char[--len] = '\0'  ;
				}
				Test_and_Add_Alias( name_char, sn) ;
				break ;
			}
		}
	}
	if ( alias_line != NULL ) {
		free(alias_line) ; // not owfree since allocated by getline
	}
	fclose(alias_file_pointer);
	return gbGOOD;
}

/* Name is a null-terminated string */
/* sn is an 8-byte serial number */
/* 1. Trims name
 * 2. Checks name length
 * 3. Refuses reserved words
 * 4. Refuses path separator (/)
 * 5. Removes any old assignments to name or serial number
 * 6. Add new alias
 * */
GOOD_OR_BAD Test_and_Add_Alias( char * name, BYTE * sn )
{
	BYTE sn_stored[SERIAL_NUMBER_SIZE] ;
	size_t len ;

	// Parse off initial spaces
	while ( name[0] == ' ' ) {
		++name ;
	}

	// parse off trailing spaces
	for ( len = strlen(name) ; len > 0 && name[len-1]==' ' ; ) {
		-- len ;
		name[len] = '\0' ;
	}

	// Anything left of the name?
	if ( len == 0 ) {
		LEVEL_DEBUG("Empty alias name") ;
		return gbBAD ;
	}

	// Check length
	if ( len > PROPERTY_LENGTH_ALIAS ) {
		LEVEL_CALL("Alias too long: sn=" SNformat ", Alias=%s, Length=%d, Max length=%d", SNvar(sn), name,  (int) len, PROPERTY_LENGTH_ALIAS ) ;
		return gbBAD ;
	}

	// Reserved word?
	if ( strcmp( name, "interface" )==0
	|| strcmp( name, "settings" )==0
	|| strcmp( name, "uncached" )==0
	|| strcmp( name, "unaliased" )==0
	|| strcmp( name, "text" )==0
	|| strcmp( name, "alarm" )==0
	|| strcmp( name, "statistics" )==0
	|| strcmp( name, "simultaneous" )==0
	|| strcmp( name, "structure" )==0
	|| strncmp( name, "bus.", 4 )==0
	) {
		LEVEL_CALL("Alias attempts to redefine reserved filename: %s",name ) ;
		return gbBAD ;
	}

	// No path separator allowed in name
	if ( strchr( name, '/' ) ) {
		LEVEL_CALL("Alias contains confusing path separator \'/\': %s",name ) ;
		return gbBAD ;
	}

	// Is there another assignment for this alias name already?
	if ( GOOD( Cache_Get_Alias_SN( name, sn_stored ) ) ) {
		if ( memcmp( sn, sn_stored, SERIAL_NUMBER_SIZE ) == 0 ) {
			// repeat assignment
			return gbGOOD ;
		}
		// delete old serial number
		LEVEL_CALL("Alias %s reassigned from "SNformat" to "SNformat,name,SNvar(sn_stored),SNvar(sn)) ;
		Cache_Del_Alias(sn_stored) ;
	}

	// Delete any prior assignments for this serial number
	Cache_Del_Alias(sn) ;

	// Now add
	return Cache_Add_Alias( name, sn) ;
}

void FS_dir_entry_aliased(void (*dirfunc) (void *, const struct parsedname *), void *v, const struct parsedname *pn)
{
	if ( ( pn->state & ePS_unaliased ) == 0 ) {
		// Want alias substituted
		struct parsedname s_pn_copy ;
		struct parsedname * pn_copy = & s_pn_copy ;
		ASCII path_copy[PATH_MAX+1] ;
		ASCII * path_pointer = pn->path ; // current location in original path
		enum alias_parse_state { aps_initial, aps_next, aps_last } aps = aps_initial ;

		// Shallow copy
		memcpy( pn_copy, pn, sizeof(struct parsedname) ) ;
		memset( path_copy, 0, sizeof(path_copy) ) ;
		//printf("About the text alias on %s\n",pn->path);

		while ( aps != aps_last ) {
			ASCII * path_copy_pointer =  & path_copy[strlen(path_copy)] ; // point to end of copy

			ASCII * path_slash = strchr(path_pointer,'/') ;
			BYTE sn[SERIAL_NUMBER_SIZE] ;
			
			if ( aps == aps_initial ) {
				aps = aps_next ;
			} else {
				// add the slash for the next section
				path_copy_pointer[0] = '/' ;
				++path_copy_pointer ;
			}
				
			if ( path_slash == NULL ) {
				// last part of path
				strcpy( path_copy_pointer, path_pointer ) ;
				aps = aps_last ;
			} else {
				// copy this segment
				strncpy( path_copy_pointer, path_pointer, path_slash-path_pointer) ;
				path_pointer = path_slash + 1 ; // past '/'
			}
			
			//test this segment for serial number
			if ( Parse_SerialNumber(path_copy_pointer,sn) == sn_valid ) {
				//printf("We see serial number in path "SNformat"\n",SNvar(sn)) ;
				// now test for alias
				ASCII * name = Cache_Get_Alias( sn ) ;
				if ( name != NULL ) {
					//printf("It's aliased to %s\n",name);
					// now test for room
					if ( path_copy + PATH_MAX > path_copy_pointer + strlen(name) ) {
						// overwrite serial number with alias name
						strcpy( path_copy_pointer, name ) ;
					}
					owfree( name ) ;
				}
			}
		}

		pn_copy->path = path_copy ;

		DIRLOCK;
		dirfunc(v, pn_copy);
		DIRUNLOCK;
	} else {
		// Don't want alias substituted
		DIRLOCK;
		dirfunc(v, pn);
		DIRUNLOCK;
	}
}
