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

#include <config.h>
#include "owfs_config.h"
#include "ow.h"

int AliasFile(const ASCII * file)
{
	FILE *alias_file_pointer = fopen(file, "r");

	if (alias_file_pointer) {
		int ret = 0;
		ASCII alias_line[260] ;
		int line_number = 0;

		while (fgets(alias_line, 258, alias_file_pointer)) {
			++line_number;
			BYTE sn[8] ;
			// check line length
			if (strlen(alias_line) > 255) {
				LEVEL_DEFAULT("Alias file (%s:%d) Line too long (>255 characters).\n", SAFESTRING(file), line_number);
				ret = 1;
				break;
			} else {
				char * a_line = alias_line ;
				char * sn_char = NULL ;
				char * name_char = NULL ;
				while ( a_line ) {
					sn_char = strsep( &a_line, " \t=\n");
					if ( strlen(sn_char) ) {
						break ;
					}
				}
				if ( Parse_SerialNumber(sn_char, sn) ) {
					LEVEL_CALL("Problem parsing device name in alias file %s:%d\n",file,line_number) ;
					continue ;
				}
				if ( a_line ) {
					a_line += strspn(a_line," \t=") ;
				}
				while ( a_line ) {
					name_char = strsep( &a_line, "/\n");
					if ( strlen(name_char) ) {
						size_t len = strlen(name_char) ;
						while ( len>0 ) {
							if ( name_char[len-1] != ' ' && name_char[len-1] != '\t' ) {
								break ;
							}
							name_char[--len] = '\0'  ;
						}
						Cache_Add_Alias( name_char, sn) ;
						break ;
					}
				}
			}
		}
		fclose(alias_file_pointer);
		return ret;
	} else {
		ERROR_DEFAULT("Cannot process alias file %s\n", file);
		return 1;
	}
	return 1 ;
}
