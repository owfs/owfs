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
#include "ow.h"

#include "ow_connection.h"

#if OW_W1

static void W1Scan( const char * directory ) ;
static struct connection_in * CreateIn(const char * name ) ;
static struct connection_in *FindIn(const char * name ) ;
static void W1_bus( const char * directory ) ;

static void W1Scan( const char * directory )
{
	struct connection_in * in ;

	CONNIN_WLOCK ;

	++Inbound_Control.w1_seq ;

	// read current w1 bus masters from sysfs
	W1_bus(directory) ;

	// check for w1 bus masters that weren't found
	for ( in = Inbound_Control.head ; in ; in = in->next ) {
		if ( in->busmode == bus_w1
			&& in->connin.w1.seq  != Inbound_Control.w1_seq
			) {
			LEVEL_DEBUG("w1 bus <%s> no longer found\n",in->name) ;
			RemoveIn( in ) ;
		}
	}
		
	CONNIN_WUNLOCK ;
}

static struct connection_in * CreateIn(const char * name )
{
	struct connection_in * in ;

	in = NewIn(NULL) ;
	if ( in != NULL ) {
		in->name = strdup(name) ;
		W1_detect(in) ;
		LEVEL_DEBUG("Created a new bus.%d\n",in->index) ;
	}

	return in ;
}

// Finds matching connection
// returns it if found,
// else NULL
static struct connection_in *FindIn(const char * name)
{
	struct connection_in *now ;
	for ( now = Inbound_Control.head ; now != NULL ; now = now->next ) {
		//printf("Matching %d/%s/%s/%s/ to bus.%d %d/%s/%s/%s/\n",bus_zero,name,type,domain,now->index,now->busmode,now->connin.tcp.name,now->connin.tcp.type,now->connin.tcp.domain);
		if ( now->busmode == bus_w1
			&& now->name   != NULL
			&& strcasecmp( now->name  , name  ) == 0
			) {
			return now ;
		}
	}
	return NULL;
}

static void W1_bus( const char * directory )
{
	DIR * sys_w1  = opendir(directory) ;

	if ( sys_w1 != NULL ) {
		struct dirent * dent ;

		while ( (dent = readdir( sys_w1)) != NULL ) {
			if ( strncasecmp( "w1", dent->d_name, 2 ) == 0 ) {
				struct connection_in * in =  FindIn( dent->d_name ) ;
				if ( in != NULL ) {
					LEVEL_DEBUG("w1 bus <%s> already known\n",dent->d_name) ;
					in->connin.w1.seq = Inbound_Control.w1_seq ;
					continue ;
				}
				in = CreateIn(dent->d_name) ;
				if ( in != NULL ) {
					LEVEL_DEBUG("w1 bus <%s> to be added\n",dent->d_name) ;
					in->connin.w1.seq = Inbound_Control.w1_seq ;
					continue ;
				}
				LEVEL_DEBUG("w1 bus <%s> couldn't be added\n",dent->d_name) ;
			}
		}
		
		closedir( sys_w1 ) ;
	}
}

#if OW_MT

int W1_Browse( void )
{
	LEVEL_CONNECT("Dynamic w1 support is not supported on this platform\n");
	// Initial setup
	W1Scan("/sys/bus/w1/devices") ;
	return 0;
}

#else /* OW_MT */
int W1_Browse( void )
{
	LEVEL_CONNECT("Dynamic w1 support requires multithreading (a compile-time option\n");
	// Initial setup
	W1Scan("/sys/bus/w1/devices") ;
}
#endif /* OW_MT */

#endif /* OW_W1 */
