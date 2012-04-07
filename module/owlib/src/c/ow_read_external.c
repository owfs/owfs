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

/* ow_parse_external -- read the SENSOR and PROPERTY lines */

/* in general the lines are comma separated tokens
 * with backslash escaping and
 * quote and double quote matching
 * */

#include <config.h>
#include "owfs_config.h"
#include "ow.h"
#include "ow_external.h"


/* strategy for external read:
   A single read function for each communication scheme e.g. script
   The actual device record and actual familt record is obtained from the trees.
   The script is called (via popen) or other method for other types
   The device should be locked for the communication
   arguments include fields from the tree records and owq
   the returned value is obvious
*/


READ_FUNCTION( FS_r_external_script ) ;

ZERO_OR_ERROR FS_r_external_script( struct one_wire_query * owq )
{
	return 0 ;
}
