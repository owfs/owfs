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

static GOOD_OR_BAD Test_and_Add_Alias( char * name, BYTE * sn ) ;

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

static GOOD_OR_BAD Test_and_Add_Alias( char * name, BYTE * sn )
{
	BYTE sn_stored[SERIAL_NUMBER_SIZE] ;
	if ( strlen(name) > PROPERTY_LENGTH_ALIAS ) {
		LEVEL_CALL("Alias too long: sn=" SNformat ", alias=%s max length=%d", SNvar(sn), name,  PROPERTY_LENGTH_ALIAS ) ;
		return gbBAD ;
	}

	if ( strcmp( name, "interface" )==0
	|| strcmp( name, "settings" )==0
	|| strcmp( name, "uncached" )==0
	|| strcmp( name, "text" )==0
	|| strcmp( name, "alarm" )==0
	|| strcmp( name, "statistics" )==0
	|| strcmp( name, "simultaneous" )==0
	|| strcmp( name, "structure" )==0
	|| strncmp( name, "bus.", 4 )==0
	) {
		LEVEL_CALL("Alias copies intrinsic filename: %s",name ) ;
		return gbBAD ;
	}
	if ( GOOD( Cache_Get_SerialNumber( name, sn_stored )) && memcmp(sn,sn_stored,SERIAL_NUMBER_SIZE)!=0 ) {
		LEVEL_CALL("Alias redefines a previous alias: %s " SNformat " and " SNformat,name,SNvar(sn),SNvar(sn_stored) ) ;
		return gbBAD ;
	}
	if ( strchr( name, '/' ) ) {
		LEVEL_CALL("Alias contains confusing path separator \'/\': %s",name ) ;
		return gbBAD ;
	}
	return Cache_Add_Alias( name, sn) ;
}

void FS_dir_entry_aliased(void (*dirfunc) (void *, const struct parsedname *), void *v, const struct parsedname *pn)
{
	if ( pn->control_flags | ALIAS_REQUEST) {
		// Want alias substituted
		struct parsedname s_pn_copy ;
		struct parsedname * pn_copy = & s_pn_copy ;
		ASCII path_copy[PATH_MAX+1] ;
		ASCII * path_pointer = pn->path ;
		ASCII * path_slash ;
		BYTE sn[SERIAL_NUMBER_SIZE] ;
		ASCII * path_copy_pointer ;
		enum alias_parse_state { aps_initial, aps_next, aps_last } aps = aps_initial ;

		// Shallow copy
		memcpy( pn_copy, pn, sizeof(struct parsedname) ) ;
		memset( path_copy, 0, sizeof(path_copy) ) ;

		while ( aps != aps_last ) {
			path_slash = strchr(path_pointer,'/') ;
			if ( aps != aps_initial ) {
				strcat( path_copy, "/") ;
			}
			// point to end
			path_copy_pointer = & path_copy[strlen(path)] ;
			if ( path_slash == NULL ) {
				strcpy(path_copy_pointer,path_pointer) ;
				aps = aps_last ;
			} else {
				strncpy(path_copy_pointer, path_pointer, path_slash-path_pointer) ;
				path_pointer = path_slash + 1 ; // past '/'
				aps = aps_next ;
			}
			if ( Parse_SerialNumber(path_copy_pointer,sn) == sn_valid ) {
				Cache_Get_Alias(path_copy_pointer,PATH_MAX - (path_copy_pointer-path_copy),sn) ;
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
