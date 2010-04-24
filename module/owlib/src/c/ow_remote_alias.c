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

/* General Device File format:
    This device file corresponds to a specific 1wire/iButton chip type
    ( or a closely related family of chips )

    The connection to the larger program is through the "device" data structure,
      which must be declared in the acompanying header file.

    The device structure holds the
      family code,
      name,
      device type (chip, interface or pseudo)
      number of properties,
      list of property structures, called "filetype".

    Each filetype structure holds the
      name,
      estimated length (in bytes),
      aggregate structure pointer,
      data format,
      read function,
      write funtion,
      generic data pointer

    The aggregate structure, is present for properties that several members
    (e.g. pages of memory or entries in a temperature log. It holds:
      number of elements
      whether the members are lettered or numbered
      whether the elements are stored together and split, or separately and joined
*/

#include <config.h>
#include "owfs_config.h"
#include "ow_standard.h"
#include "ow_counters.h"
#include "ow_connection.h"

/* ------- Prototypes ------------ */
static struct connection_in * NextServer( struct connection_in * in ) ;
static INDEX_OR_ERROR ServerAlias( BYTE * sn, struct connection_in * in, struct parsedname * pn ) ;

/* ------- Functions ------------ */

static struct connection_in * NextServer( struct connection_in * in ) 
{
	do {
		if ( in == NULL ) {
			return NULL ;
		}
		if ( BusIsServer(in) ) {
			return in ;
		}
		in = in->next ;
	} while (1) ;
}

static INDEX_OR_ERROR ServerAlias( BYTE * sn, struct connection_in * in, struct parsedname * pn ) 
{
	struct parsedname s_pn_copy;
	struct parsedname * pn_copy = &s_pn_copy ;
	INDEX_OR_ERROR bus_nr ;

	memcpy(pn_copy, pn, sizeof(struct parsedname));	// shallow copy
	pn_copy->selected_connection = in;
	bus_nr = ServerPresence( pn_copy) ;
	memcpy( sn, pn_copy->sn, SERIAL_NUMBER_SIZE );
	return bus_nr ;
}	

#if OW_MT

struct remotealias_struct {
	struct connection_in *in;
	struct parsedname *pn;
	BYTE sn[SERIAL_NUMBER_SIZE] ;
	INDEX_OR_ERROR bus_nr;
};

static void * RemoteAlias_callback(void * v)
{
	pthread_t thread;
	int threadbad = 1;
	struct remotealias_struct * ras = (struct remotealias_struct *) v ;
	struct remotealias_struct next_ras ;
	
	next_ras.in = NextServer(ras->in->next) ;
	next_ras.pn = ras->pn ;
	next_ras.bus_nr = INDEX_BAD ;

	memset( next_ras.sn, 0, SERIAL_NUMBER_SIZE) ;
		
	threadbad = (next_ras.in == NULL)
	|| pthread_create(&thread, NULL, RemoteAlias_callback, (void *) (&next_ras));
	
	ras->bus_nr = ServerAlias( ras->sn, ras->in, ras->pn ) ;
	
	if (threadbad == 0) {		/* was a thread created? */
		void *vv;
		if (pthread_join(thread, &vv)==0) {
			if ( INDEX_VALID(next_ras.bus_nr) ) {
				ras->bus_nr = next_ras.bus_nr ;
				memcpy( ras->sn, next_ras.sn, SERIAL_NUMBER_SIZE ) ;
			}
		}
	}
	return NULL ;
}

INDEX_OR_ERROR RemoteAlias(struct parsedname *pn)
{
	struct remotealias_struct ras ; 
	
	ras.in = NextServer(Inbound_Control.head) ;
	ras.pn = pn ;
	ras.bus_nr = INDEX_BAD ;

	memset( ras.sn, 0, SERIAL_NUMBER_SIZE) ;
	
	if ( ras.in != NULL ) {
		RemoteAlias_callback( (void *) (&ras) ) ;
	}
	
	if ( INDEX_VALID( ras.bus_nr ) ) {
		memcpy( pn->sn, ras.sn, SERIAL_NUMBER_SIZE ) ;
		Cache_Add_Device( ras.bus_nr, ras.sn ) ;
	}
	LEVEL_DEBUG("Remote alias for %s bus=%d "SNformat,pn->path_to_server,ras.bus_nr,SNvar(ras.sn));
	return ras.bus_nr;
}

#else							/* OW_MT */

INDEX_OR_ERROR RemoteAlias(struct parsedname *pn)
{
	struct connection_in * in ;
	
	for ( in=NextServer(Inbound_Control.head) ; in ; in=NextServer(in->next) ) {
		BYTE sn[SERIAL_NUMBER_SIZE] ;
		INDEX_OR_ERROR bus_nr = ServerAlias( sn, in, pn ) ;
		if ( INDEX_VALID(bus_nr) ) {
			memcpy( pn->sn, sn, SERIAL_NUMBER_SIZE ) ;
			Cache_Add_Device( bus_nr, sn ) ;
			LEVEL_DEBUG("Remote alias for %s bus=%d "SNformat,pn->path_to_server,bus_nr,SNvar(sn));
			return bus_nr ;
		}
	}
	return INDEX_BAD;				// no success
}
#endif							/* OW_MT */
