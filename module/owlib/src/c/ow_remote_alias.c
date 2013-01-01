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
		if ( in == NO_CONNECTION ) {
			return NO_CONNECTION ;
		}
		if ( BusIsServer(in) ) {
			return in ;
		}
		in = in->next ;
	} while (1) ;
}

static INDEX_OR_ERROR ServerAlias( BYTE * sn, struct connection_in * in, struct parsedname * pn ) 
{
	if ( in != NO_CONNECTION ) {
		struct parsedname s_pn_copy;
		struct parsedname * pn_copy = &s_pn_copy ;
		INDEX_OR_ERROR bus_nr ;
		
		memcpy(pn_copy, pn, sizeof(struct parsedname));	// shallow copy
		pn_copy->selected_connection = in;
		bus_nr = ServerPresence( pn_copy) ;
		memcpy( sn, pn_copy->sn, SERIAL_NUMBER_SIZE );
		return bus_nr ;
	}
	return INDEX_BAD ;
}	

struct remotealias_struct {
	struct port_in * pin;
	struct connection_in *cin;
	struct parsedname *pn;
	BYTE sn[SERIAL_NUMBER_SIZE] ;
	INDEX_OR_ERROR bus_nr;
};

static void * RemoteAlias_callback_conn(void * v)
{
	struct remotealias_struct * ras = (struct remotealias_struct *) v ;
	struct remotealias_struct ras_next ;
	pthread_t thread;
	int threadbad = 0;
	
	// set up for next server connection
	ras_next.cin = NextServer(ras->cin->next) ;
	if ( ras_next.cin == NO_CONNECTION ) {
		threadbad = 1 ;
	} else {
		ras_next.pin = ras->pin ;
		ras_next.pn = ras->pn ;
		ras_next.bus_nr = INDEX_BAD ;
		memset( ras_next.sn, 0, SERIAL_NUMBER_SIZE) ;
		threadbad = pthread_create(&thread, DEFAULT_THREAD_ATTR, RemoteAlias_callback_conn, (void *) (&ras_next)) ;
	}
	
	// Result for this server connection (while next one is processing)
	ras->bus_nr = ServerAlias( ras->sn, ras->cin, ras->pn ) ;
	
	if (threadbad == 0) {		/* was a thread created? */
		if (pthread_join(thread, NULL)==0) {
			// Set answer to the next bus if it's found
			// else use current answer
			if ( INDEX_VALID(ras_next.bus_nr) ) {
				ras->bus_nr = ras_next.bus_nr ;
				memcpy( ras->sn, ras_next.sn, SERIAL_NUMBER_SIZE ) ;
			}
		}
	}
	return VOID_RETURN ;
}

static void * RemoteAlias_callback_port(void * v)
{
	struct remotealias_struct * ras = (struct remotealias_struct *) v ;
	struct remotealias_struct ras_next ;
	pthread_t thread;
	int threadbad = 0;
	
	// set up for next server connection
	ras_next.pin = ras->pin->next ;
	if ( ras_next.pin == NULL ) {
		threadbad = 1 ;
	} else {
		ras_next.pn = ras->pn ;
		ras_next.bus_nr = INDEX_BAD ;
		memset( ras_next.sn, 0, SERIAL_NUMBER_SIZE) ;
		threadbad = pthread_create(&thread, DEFAULT_THREAD_ATTR, RemoteAlias_callback_port, (void *) (&ras_next)) ;
	}
	
	ras->cin = NextServer( ras->pin->first ) ;
	if ( ras->cin != NO_CONNECTION ) {
		RemoteAlias_callback_conn(v) ;
	}
	
	if (threadbad == 0) {		/* was a thread created? */
		if (pthread_join(thread, NULL)==0) {
			// Set answer to the next bus if it's found
			// else use current answer
			if ( INDEX_VALID(ras_next.bus_nr) ) {
				ras->bus_nr = ras_next.bus_nr ;
				memcpy( ras->sn, ras_next.sn, SERIAL_NUMBER_SIZE ) ;
			}
		}
	}
	return VOID_RETURN ;
}

INDEX_OR_ERROR RemoteAlias(struct parsedname *pn)
{
	struct remotealias_struct ras ; 
	
	ras.pin = Inbound_Control.head_port ;
	ras.pn = pn ;
	ras.bus_nr = INDEX_BAD ;
	memset( ras.sn, 0, SERIAL_NUMBER_SIZE) ;

	if ( ras.pin != NULL ) {
		RemoteAlias_callback_port( (void *) (&ras) ) ;
	}
	
	memcpy( pn->sn, ras.sn, SERIAL_NUMBER_SIZE ) ;

	if ( INDEX_VALID( ras.bus_nr ) ) {
		LEVEL_DEBUG("Remote alias for %s bus=%d "SNformat,pn->path_to_server,ras.bus_nr,SNvar(ras.sn));
	} else {
		LEVEL_DEBUG("Remote alias for %s not found",pn->path_to_server);
	}
	return ras.bus_nr;		
}
