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

static int Test_Add_Alias( char * name, BYTE * sn ) ;

int AliasFile(const ASCII * file)
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
		return 1;
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

		if ( Parse_SerialNumber(sn_char, sn) ) {
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
				Test_Add_Alias( name_char, sn) ;
				break ;
			}
		}
	}
	if ( alias_line != NULL ) {
		free(alias_line) ; // not owfree since allocated by getline
	}
	fclose(alias_file_pointer);
	return 0;
}

static int Test_Add_Alias( char * name, BYTE * sn )
{
	BYTE sn_stored[SERIAL_NUMBER_SIZE] ;
	if ( strlen(name) > PROPERTY_LENGTH_ALIAS ) {
		LEVEL_CALL("Alias too long: sn=" SNformat ", alias=%s max length=%d", SNvar(sn), name,  PROPERTY_LENGTH_ALIAS ) ;
		return 1 ;
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
		return 1 ;
	}
	if ( Cache_Get_SerialNumber( name, sn_stored )==0 && memcmp(sn,sn_stored,SERIAL_NUMBER_SIZE)!=0 ) {
		LEVEL_CALL("Alias redefines a previous alias: %s " SNformat " and " SNformat,name,SNvar(sn),SNvar(sn_stored) ) ;
		return 1 ;
	}
	if ( strchr( name, '/' ) ) {
		LEVEL_CALL("Alias contains confusin path separator \'/\': %s",name ) ;
		return 1 ;
	}
	return Cache_Add_Alias( name, sn) ;
}

void FS_alias_subst(void (*dirfunc) (void *, const struct parsedname *), void *v, const struct parsedname *pn)
{
	if ( pn->control_flags | ALIAS_REQUEST) {
		struct parsedname s_pn_copy ;
		struct parsedname * pn_copy = & s_pn_copy ;
		ASCII path[PATH_MAX+1] ;
		ASCII * pcurrent = pn->path ;
		ASCII * pnext ;
		BYTE sn[SERIAL_NUMBER_SIZE] ;
		ASCII * pathstart ;
		enum alias_parse_state { aps_initial, aps_next, aps_last } aps = aps_initial ;

		// Shallow copy
		memcpy( pn_copy, pn, sizeof(struct parsedname) ) ;
		memset( path, 0, sizeof(path) ) ;

		while ( aps != aps_last ) {
			pnext = strchr(pcurrent,'/') ;
			if ( aps != aps_initial ) {
				strcat( path, "/") ;
			}
			// point to end
			pathstart = & path[strlen(path)] ;
			if ( pnext == NULL ) {
				strcpy(pathstart,pcurrent) ;
				aps = aps_last ;
			} else {
				strncpy(pathstart,pcurrent, pnext-pcurrent) ;
				pcurrent = pnext + 1 ; // past '/'
				aps = aps_next ;
			}
			if ( Parse_SerialNumber(pathstart,sn)==0 ) {
				Cache_Get_Alias(pathstart,PATH_MAX - (pathstart-path),sn) ;
			}
		}

		pn_copy->path = path ;

		DIRLOCK;
		dirfunc(v, pn_copy);
		DIRUNLOCK;
	} else {
		DIRLOCK;
		dirfunc(v, pn);
		DIRUNLOCK;
	}
}
