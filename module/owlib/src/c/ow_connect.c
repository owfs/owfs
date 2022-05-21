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
#include "ow_connection.h"

static struct connection_in *AllocIn(const struct connection_in *old_in) ;

/* Routines for handling a linked list of connections in*/
/* typical connection in would be the serial port or USB */

/* Globals */
struct inbound_control Inbound_Control = {
	.active = 0,
	.next_index = 0,
	.head_port = NULL,
	.next_fake = 0,
	.next_tester = 0,
	.next_mock = 0,
	.w1_monitor = NO_CONNECTION ,
	.external = NO_CONNECTION ,
};

struct connection_in *find_connection_in(int bus_number)
{
	struct port_in * pin ;
	
	for ( pin = Inbound_Control.head_port ; pin != NULL ; pin = pin->next ) {
		struct connection_in *cin;
		for ( cin = pin->first; cin != NO_CONNECTION; cin = cin->next) {
			if (cin->index == bus_number) {
				return cin;
			}
		}
	}
	LEVEL_DEBUG("Couldn't find bus number %d",bus_number);
	return NO_CONNECTION;
}

int SetKnownBus( int bus_number, struct parsedname * pn)
{
	struct connection_in * found = find_connection_in( bus_number ) ;
	if ( found == NO_CONNECTION ) {
		return 1 ;
	}
	pn->state |= ePS_bus;
	pn->selected_connection=found ;
	pn->known_bus=found;
	return 0 ;
}

enum bus_mode get_busmode(const struct connection_in *in)
{
	if (in == NO_CONNECTION) {
		return bus_unknown;
	}
	return in->pown->busmode;
}

// return true (non-zero) if the connection_in exists, and is remote
int BusIsServer(struct connection_in *in)
{
	if ( in == NO_CONNECTION ) {
		return 0 ;
	}
	switch ( get_busmode(in) ) {
		case bus_server:
		case bus_zero:
			return 1 ;
		default:
			return 0 ;
	}
}

/* Make a new connection_in entry, but DON'T place it in the chain (yet)*/
/* Based on a shallow copy of "in" if not NULL */
static struct connection_in *AllocIn(const struct connection_in *old_in)
{
	size_t len = sizeof(struct connection_in);
	struct connection_in *new_in = (struct connection_in *) owmalloc(len);
	if (new_in != NO_CONNECTION) {
		if (old_in == NO_CONNECTION) {
			// Not a copy
			memset(new_in, 0, len);
			DEVICENAME(new_in) = NULL ; 
			/* Not yet a valid bus */
			new_in->iroutines.flags = ADAP_FLAG_sham ;
		} else {
			// Copy of prior bus
			memcpy(new_in, old_in, len);
			if ( DEVICENAME(new_in) != NULL ) {
				// Don't make the name point to the same string, make a copy
				DEVICENAME(new_in) = owstrdup( DEVICENAME(old_in) ) ;
			}
		}
		
		/* not yet linked */
		new_in->next = NO_CONNECTION ;

		/* Support DS1994/DS2404 which require longer delays, and is automatically
		 * turned on in *_next_both().
		 * If it's turned off, it will result into a faster reset-sequence.
		 */
		new_in->ds2404_found = 0;
		/* Flag first pass as need to clear all branches if DS2409 present */
		new_in->branch.branch = eBranch_bad ;
		/* Arbitrary guess at root directory size for allocating cache blob */
		new_in->last_root_devs = 10;
		new_in->AnyDevices = anydevices_unknown ;

		++Inbound_Control.active ;
		new_in->index = Inbound_Control.next_index++;
		_MUTEX_INIT(new_in->bus_mutex);
		_MUTEX_INIT(new_in->dev_mutex);
		new_in->dev_db = NULL;
	} else {
		LEVEL_DEFAULT("Cannot allocate memory for bus master structure");
	}
	return new_in;
}

struct port_in * AllocPort( const struct port_in * old_pin )
{
	size_t len = sizeof(struct port_in);
	struct port_in *new_pin = (struct port_in *) owmalloc(len);
	if ( new_pin != NULL ) {
		if (old_pin == NULL) {
			// Not a copy
			memset(new_pin, 0, len);
			new_pin->first = AllocIn(NO_CONNECTION) ;
		} else {
			// Copy of prior bus
			memcpy(new_pin, old_pin, len);
			new_pin->first = AllocIn(old_pin->first) ;
			if ( old_pin->init_data != NULL ) {
				new_pin->init_data = owstrdup( old_pin->init_data ) ;
			}		
		}
		
		// never copy file_descriptor (it's unique to port)
		new_pin->file_descriptor = FILE_DESCRIPTOR_BAD ;
		new_pin->type = ct_unknown ;
		new_pin->state = cs_virgin ;

		if ( new_pin->first == NO_CONNECTION ) {
			LEVEL_DEBUG("Port creation incomplete");
			owfree(new_pin) ;
			return NULL ;
		}
		
		new_pin->connections = 1 ;
		new_pin->first->channel = 0 ;
		
		/* port not yet linked */
		new_pin->next = NULL ;
		/* connnection_in needs to be linked */
		new_pin->first->pown = new_pin ;

	} else {
		LEVEL_DEFAULT("Cannot allocate memory for port master structure");
	}
	return new_pin;
}

/* Place a new connection in the chain */
struct port_in *LinkPort(struct port_in *pin)
{
	if (pin != NULL) {
		// Housekeeping to place in linked list
		// Locking done at a higher level
		pin->next = Inbound_Control.head_port;	/* put in linked list at start */
		Inbound_Control.head_port = pin ;

		_MUTEX_INIT(pin->port_mutex);
	}
	return pin;
}

/* Create a new inbound structure and place it in the chain */
struct port_in *NewPort(const struct port_in *pin) 
{
	return LinkPort( AllocPort(pin) ) ;
}

/* Free all connection_in in reverse order (Added to head on creation, head-first deletion) */
void FreeInAll( void )
{
	while ( Inbound_Control.head_port ) {
		RemovePort(Inbound_Control.head_port);
	}
}

void RemoveIn( struct connection_in * conn )
{
	struct port_in * pin ;
	
	/* NULL safe */
	if ( conn == NO_CONNECTION ) {
		return ;
	}

	/* Close master-specific resources. This may do writes
	 * and what-not that expects the conn to be kept setup. */
	BUS_close(conn) ;

	// owning port
	pin = conn->pown ;

	/* First unlink from list */
	if ( pin == NULL ) {
		// free-floating
	} else if ( pin->first == conn ) {
		/* Head of list, easy */
		pin->first = conn->next ;
		-- pin->connections ;
		--Inbound_Control.active ;
	} else {
		struct connection_in * now ;
		/* find in list and splice out */
		for ( now = pin->first ; now != NO_CONNECTION ; now = now->next ) {
			/* Works even if not linked, since won't match and will just fall through */
			if ( now->next == conn ) {
				now->next = conn->next ;
				-- pin->connections ;
				--Inbound_Control.active ;
				break ;
			}
		}
	}

	if ( conn->index == Inbound_Control.next_index-1 ) {
		Inbound_Control.next_index-- ;
	}

	/* Now free up thread-sync resources */
	_MUTEX_DESTROY(conn->bus_mutex);
	_MUTEX_DESTROY(conn->dev_mutex);
	SAFETDESTROY( conn->dev_db, owfree_func);

	/* Free port */
	COM_free( conn ) ;

	/* Next free up internal resources */
	SAFEFREE( DEVICENAME(conn) ) ;
	
	/* Finally delete the structure */
	owfree(conn);
}

void RemovePort( struct port_in * pin )
{
	/* NULL safe */
	if ( pin == NULL ) {
		return ;
	}

	/* First delete connections */
	while ( pin->first != NO_CONNECTION ) {
		RemoveIn( pin->first ) ;
	}

	/* Next unlink from list */
	if ( pin == Inbound_Control.head_port ) {
		/* Head of list, easy */
		Inbound_Control.head_port = pin->next ;
	} else {
		struct port_in * now ;
		/* find in list and splice out */
		for ( now = Inbound_Control.head_port ; now != NULL ; now = now->next ) {
			/* Works even if not linked, since won't match and will just fall through */
			if ( now->next == pin ) {
				now->next = pin->next ;
				break ;
			}
		}
	}

	/* Now free up thread-sync resources */
	/* Only if actually linked in and possibly active */
	_MUTEX_DESTROY(pin->port_mutex);
	SAFEFREE(pin->init_data) ;

	/* Finally delete the structure */
	owfree(pin);
	pin = NULL ;
}

struct connection_in * AddtoPort( struct port_in * pin )
{
	struct connection_in * add_in = AllocIn( pin->first ) ;
	
	if ( add_in == NO_CONNECTION ) {
		// cannot allocate
		return NO_CONNECTION ;
	}

	// Housekeeping to place in linked list
	// Locking done at a higher level
	
	add_in->next = pin->first;	/* put in linked list at start */
	pin->first = add_in;
	add_in->pown = pin ;

	add_in->channel = pin->connections ;
	++pin->connections ;

	return add_in ;
}
