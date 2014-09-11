/*
    OWFS -- One-Wire filesystem
    OWHTTPD -- One-Wire Web Server
    Written 2003 Paul H Alfille
    email: paul.alfille@gmail.com
    Released under the GPL
    See the header file: ow.h for full attribution
    1wire/iButton system from Dallas Semiconductor
*/

/* ow_opt -- owlib specific command line options processing */

#ifndef _GNU_SOURCE_
#define _GNU_SOURCE
#include <stdio.h> // for getline
#undef _GNU_SOURCE
#else
#include <stdio.h> // for getline
#endif

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

	// zero length allowed -- will delete this entry.

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

		ASCII path[PATH_MAX+3] ;
		ASCII * path_pointer = path ; // current location in original path

		// Shallow copy
		memcpy( pn_copy, pn, sizeof(struct parsedname) ) ;
		pn_copy->path[0] = '\0' ;

		// path copy to use for separation
		strcpy( path, pn->path ) ;

		// copy segments of path (delimitted by "/") to copy
		while( path_pointer != NULL ) {
			ASCII * path_segment = strsep( &path_pointer, "/" ) ;
			BYTE sn[SERIAL_NUMBER_SIZE] ;
			
			if ( PATH_MAX < strlen(pn_copy->path) + strlen(path_segment) ) {
				// too long, just use initial copy
				strcpy( pn_copy->path, pn->path ) ;
				break ;
			}
			
			//test this segment for serial number
			if ( Parse_SerialNumber(path_segment,sn) == sn_valid ) {
				//printf("We see serial number in path "SNformat"\n",SNvar(sn)) ;
				// now test for alias
				ASCII * name = Cache_Get_Alias( sn ) ;
				if ( name != NULL ) {
					//printf("It's aliased to %s\n",name);
					// now test for room
					if ( PATH_MAX < strlen(pn_copy->path) + strlen(name) ) {
						// too long, just use initial copy
						strcpy( pn_copy->path, pn->path ) ;
						owfree(name) ;
						break ;
					}
					// overwrite serial number with alias name
					strcat( pn_copy->path, name ) ;
					owfree( name ) ;
				} else {
					strcat( pn_copy->path, path_segment ) ;
				}
			} else {
				strcat( pn_copy->path, path_segment ) ;
			}
			
			if ( path_pointer != NULL ) {
				strcat( pn_copy->path, "/" ) ;
			}
			//LEVEL_DEBUG( "Alias path so far: %s",pn_copy->path ) ;
		}

		if ( dirfunc != NULL ) {
			DIRLOCK;
			dirfunc(v, pn_copy);
			DIRUNLOCK;
		}
	} else {
		// Don't want alias substituted
		if ( dirfunc != NULL ) {
			DIRLOCK;
			dirfunc(v, pn);
			DIRUNLOCK;
		}
	}
}
