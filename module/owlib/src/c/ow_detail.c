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
#include "ow.h"

// Switch to debug mode when this device is referenced
// Unfortunately, this only occurs after parsing.

struct debug_detail { // global
	int details ; // number of details to match
	int lock_depth ; // number of detail (mutex locked)
	struct dirblob sn ; // to match
	struct dirblob length ; // use only first byte
} DD ;

void Detail_Init( void ) 
{
	DD.details = 0 ;
	DD.lock_depth = 0 ;
	DirblobInit( & (DD.sn) ) ;
	DirblobInit( & (DD.length) ) ;
}
	
void Detail_Close( void ) 
{
	DirblobClear( & (DD.sn) ) ;
	DirblobClear( & (DD.length) ) ;
}

void Detail_Test( struct parsedname * pn )
{
	int test_index ;
	
	for ( test_index = 0 ; test_index < DD.details ; ++ test_index ) {
		BYTE sn[SERIAL_NUMBER_SIZE] ;
		BYTE length[SERIAL_NUMBER_SIZE] ;
		DirblobGet( test_index, sn, &(DD.sn) ) ;
		DirblobGet( test_index, length, &(DD.length) ) ;
		if ( memcmp( pn->sn, sn, length[0] ) == 0 ) {
			pn->detail_flag = 1 ;
			DETAILLOCK ;
			++DD.lock_depth ;
			Globals.error_level = 9 ;
			DETAILUNLOCK ;
			break ;
		}
	}
}

void Detail_Free( struct parsedname * pn )
{
	if ( pn->detail_flag == 1 ) {
		DETAILLOCK ;
		--DD.lock_depth ;
		if ( DD.lock_depth == 0 ) {
			Globals.error_level = Globals.error_level_restore ;
		}
		DETAILUNLOCK ;
	}
}
	 			
GOOD_OR_BAD Detail_Add( const char *arg ) 
{
	char * arg_copy = owstrdup( arg ) ;
	char * next_p = arg_copy ;
	
	while ( next_p != NULL ) {
		BYTE sn[SERIAL_NUMBER_SIZE] ;
		BYTE length[SERIAL_NUMBER_SIZE] ;
		char * this_p = strsep( &next_p, " ," ) ;
		length[0] = SerialNumber_length( this_p, sn ) ;
		if ( length[0] > 0 ) {
			++ DD.details ;
			Globals.want_background = 0 ; //foreground
			DirblobAdd( sn, &(DD.sn) ) ;
			DirblobAdd( length, &(DD.length) ) ;
		}
	}
	return gbGOOD ;
}

